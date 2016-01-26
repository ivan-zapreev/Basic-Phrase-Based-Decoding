/* 
 * File:   translation_client.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on January 14, 2016, 2:24 PM
 */

#ifndef TRANSLATION_CLIENT_HPP
#define TRANSLATION_CLIENT_HPP

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"
#include "common/messaging/trans_job_response.hpp"
#include "common/messaging/trans_job_request.hpp"

using namespace std;
using namespace std::placeholders;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                /**
                 * This class is responsible for sending the translation
                 * job request to the server and receiving the result.
                 */
                class translation_client {
                public:
                    typedef websocketpp::client<websocketpp::config::asio_client> client;
                    typedef lock_guard<mutex> scoped_lock;
                    //Define the unique lock needed for wait/notify
                    typedef unique_lock<mutex> unique_lock;

                    translation_client(const string & host, const uint16_t port) : m_open(false), m_done(false) {
                        //Initialize the URI to connect to
                        m_uri = string("ws://") + host + string(":") + to_string(port);

                        // Set up access channels to only log interesting things
                        m_client.clear_access_channels(websocketpp::log::alevel::all);
                        m_client.set_access_channels(websocketpp::log::alevel::connect);
                        m_client.set_access_channels(websocketpp::log::alevel::disconnect);
                        m_client.set_access_channels(websocketpp::log::alevel::app);

                        // Initialize the Asio transport policy
                        m_client.init_asio();

                        // Bind the handlers we are using
                        m_client.set_open_handler(bind(&translation_client::on_open, this, _1));
                        m_client.set_close_handler(bind(&translation_client::on_close, this, _1));
                        m_client.set_fail_handler(bind(&translation_client::on_fail, this, _1));
                        m_client.set_message_handler(bind(&translation_client::on_message, this, _1, _2));
                    }

                    /**
                     * The basic destructor that also stops the client
                     */
                    ~translation_client() {
                        //Stope the client
                        disconnect();
                    }

                    /**
                     * This method will block until the connection is complete
                     * @param uri the uri to connect to
                     * @return true if the connection has been established
                     * 
                     */
                    bool connect() {
                        // Create a new connection to the given URI
                        websocketpp::lib::error_code ec;
                        client::connection_ptr con = m_client.get_connection(m_uri, ec);
                        ASSERT_CONDITION_THROW(ec, string("Get Connection (") + m_uri + string(") Error: ") + ec.message());

                        // Grab a handle for this connection so we can talk to it in a thread
                        // safe manor after the event loop starts.
                        m_hdl = con->get_handle();

                        // Queue the connection. No DNS queries or network connections will be
                        // made until the io_service event loop is run.
                        m_client.connect(con);

                        // Create a thread to run the ASIO io_service event loop
                        m_asio_thread = thread(&client::run, &m_client);

                        return wait_connect();
                    }

                    /**
                     * Allows to close the connection and stop the io service thread
                     */
                    void disconnect() {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                string("Stopping the client: m_open = ") + to_string(m_open) +
                                string(", m_done = ") + to_string(m_done));
                        if (m_open && !m_done) {
                            //Close the connection to the server
                            m_client.close(m_hdl, websocketpp::close::status::normal, "The needed translations are finished.");
                        }
                        //Stop the io service thread
                        m_client.stop();
                        //Wait for the thread to exit.
                        m_asio_thread.join();
                    }

                    /**
                     * Attempts to send the translation job request
                     * @param request thge translation job request
                     * @result the translation job id
                     */
                    job_id_type send(const trans_job_request & request) {
                        //Declare the error code
                        websocketpp::lib::error_code ec;

                        //Try to send the translation job request
                        m_client.send(m_hdl, request.serialize(), websocketpp::frame::opcode::text, ec);

                        // The most likely error that we will get is that the connection is
                        // not in the right state. Usually this means we tried to send a
                        // message to a connection that was closed or in the process of
                        // closing.
                        ASSERT_CONDITION_THROW(ec, string("Send Error: ") + ec.message());

                        //Return the job id and increment to the next one
                        return request.get_job_id();
                    }

                    /**
                     * This method is used to receive the job translation messages
                     * @param hdl the connection handler
                     * @param msg the message
                     */
                    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
                        unique_lock guard(m_lock_msg);

                        //Parse the message into the translation job reply
                        trans_job_response response(msg->get_payload());

                        LOG_RESULT << "RECEIVED!" << END_LOG;

                        //Notify that the message has been received
                        m_is_msg_received.notify_one();
                    }

                    /**
                     * Allows to wait (synchronously) for a translation job reply. The method gets a time-out parameter
                     * @param job_id the translation job id to wait for
                     * @param target_text the variable to write the translated text into 
                     * @param timeout_millisec the time out in milliseconds, the default is 0 that means to time out
                     */
                    void receive(const job_id_type job_id, string & target_text, const size_t timeout_millisec = 0) {
                        //Make sure that message related activity is synchronized
                        unique_lock guard(m_lock_msg);

                        //Wait for the reply message
                        m_is_msg_received.wait(guard);
                    }

                    /**
                     * The open handler will signal that we are ready to start sending translation job(s)
                     * @param the connection handler
                     */
                    void on_open(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection opened!");

                        scoped_lock guard(m_lock_con);
                        m_open = true;
                    }

                    /**
                     * The close handler will signal that we should stop sending translation job(s)
                     * @param the connection handler
                     */
                    void on_close(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection closed!");

                        scoped_lock guard(m_lock_con);
                        m_done = true;
                    }

                    /**
                     * The fail handler will signal that we should stop sending translation job(s)
                     * @param the connection handler
                     */
                    void on_fail(websocketpp::connection_hdl hdl) {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Connection failed!");

                        scoped_lock guard(m_lock_con);
                        m_done = true;
                    }

                    /**
                     * Allows to get the connection URI
                     * @return the connection URI
                     */
                    const string get_uri() {
                        return m_uri;
                    }

                protected:

                    /**
                     * Allows to wait until the connection to the server is established.
                     * @return true if the connection is successfully established
                     */
                    bool wait_connect() {
                        //Declare the variable to store the local connection status
                        bool is_connecting = false;

                        //Wait until the connection is established
                        while (1) {
                            //Check the connection status
                            {
                                scoped_lock guard(m_lock_con);
                                is_connecting = !m_open && !m_done;
                            }

                            //If we we are still connecting then sleep, otherwise move on
                            if (is_connecting) {
                                m_client.get_alog().write(websocketpp::log::alevel::app,
                                        string("Going to sleep, m_open = ") + to_string(m_open) +
                                        string(", m_done = ") + to_string(m_done));
                                sleep(1);
                                m_client.get_alog().write(websocketpp::log::alevel::app, "Done sleeping!");
                            } else {
                                break;
                            }
                        }

                        //If the connection is open and is not done then we are nicely connected
                        const bool result = m_open && !m_done;
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                string("Is connection open: ") + to_string(result) +
                                string(" (m_open = ") + to_string(m_open) +
                                string(", m_done = ") + to_string(m_done) + string(")"));
                        return result;
                    }

                private:
                    //Stores the client
                    client m_client;
                    //Stores the io thread
                    thread m_asio_thread;
                    //Stores the connection handler
                    websocketpp::connection_hdl m_hdl;

                    //Stores the synchronization mutex for connection
                    mutex m_lock_con;

                    //Stores the synchronization mutex for messaging
                    mutex m_lock_msg;
                    //The conditional variable for tracking the reply message
                    condition_variable m_is_msg_received;

                    //Stores the open and done flags for connection
                    atomic<bool> m_open;
                    atomic<bool> m_done;

                    //Stores the server URI
                    string m_uri;

                    //Stores the mapping from the translation job request id to
                    //the resulting translation job result, if already received.
                    //The translation jobs without a reply are mapped to NULL
                    unordered_map<job_id_type, trans_job_response *> m_jobs;
                };
            }
        }
    }
}

#endif /* TRANSLATION_CLIENT_HPP */

