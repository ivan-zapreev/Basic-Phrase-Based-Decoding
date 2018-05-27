/* 
 * File:   balancer_server.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef BALANCER_SERVER_HPP
#define BALANCER_SERVER_HPP

#include "common/messaging/websocket/websocket_server.hpp"

#include "balancer/balancer_parameters.hpp"
#include "balancer/adapters_manager.hpp"
#include "balancer/balancer_manager.hpp"
#include "balancer/bl_cmd_line_client.hpp"

using namespace std;
using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the balancer server class.
                 * Responsibilities:
                 *      Receives the supported languages requests 
                 *      Sends the supported languages responses.
                 *      Receives the translation requests
                 *      Places the received requests into dispatching queue 
                 *      Sends the translation responses.
                 * 
                 * @param TLS_CLASS the TLS class which defines the server type and mode
                 */
                template<typename TLS_CLASS>
                class balancer_server :
                public websocket_server<TLS_CLASS>,
                public bl_cmd_line_client {
                public:

                    /**
                     * The basic constructor
                     * @param params the balancer parameters
                     */
                    balancer_server(const balancer_parameters & params)
                    : websocket_server<TLS_CLASS>(params),
                    m_manager(params.m_num_req_threads, params.m_num_resp_threads),
                    m_adapters(params,
                    bind(&balancer_manager::notify_translation_response, &m_manager, _1, _2),
                    bind(&balancer_manager::notify_adapter_disconnect, &m_manager, _1)) {
                        //Provide the manager with the functional for sending
                        //the translation response and getting the adapters
                        m_manager.set_response_sender(
                                bind(&balancer_server::send_response, this, _1, _2));
                        m_manager.set_adapter_chooser(
                                bind(&adapters_manager::get_translator_adapter, &m_adapters, _1));
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void report_run_time_info() override {
                        //Report the translation server adapters info
                        m_adapters.report_run_time_info();

                        //Report the translation manager info
                        m_manager.report_run_time_info();
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void request_stop() override {
                        this->stop();
                    }

                    /**
                     * @see bl_cmd_line_client
                     */
                    virtual void set_num_inc_threads(const int32_t num_threads) override {
                        m_manager.set_num_inc_threads(num_threads);
                    }

                    /**
                     * @see bl_cmd_line_client
                     */
                    virtual void set_num_out_threads(const int32_t num_threads) override {
                        m_manager.set_num_out_threads(num_threads);
                    }

                protected:

                    /**
                     * @see websocket_server
                     */
                    virtual void before_start_listening() override {
                        //Start the translation server clients
                        m_adapters.enable();
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void after_stop_listening() override {
                        //Stop the translation servers manager
                        //Do not wait until all the running job
                        //replies arrive.
                        m_adapters.disable();

                        //Stop the translation manager.
                        m_manager.stop();
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void open_session(websocketpp::connection_hdl hdl) override {
                        m_manager.open_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void close_session(websocketpp::connection_hdl hdl) override {
                        m_manager.close_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void language_request(
                            websocketpp::connection_hdl hdl, supp_lang_req_in * msg) override {
                        //Send the response supported languages response
                        websocket_server<TLS_CLASS>::send_response(
                                hdl, m_adapters.get_supported_lang_resp_data());

                        //Destroy the message
                        delete msg;
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void translation_request(
                            websocketpp::connection_hdl hdl, trans_job_req_in * msg) override {
                        //Register the translation request, the request message
                        //is to be deleted/handled by the translation manager
                        m_manager.translate(hdl, msg);
                    }

                private:
                    //Stores the translation manager
                    balancer_manager m_manager;
                    //Stores the to the adapters manager
                    adapters_manager m_adapters;

                };

#if IS_TLS_SUPPORT
                //Add the TLS servers as an option in case the TLS support is enabled
                typedef balancer_server<server_hs_with_tls_old> balancer_server_tls_old;
                typedef balancer_server<server_hs_with_tls_int> balancer_server_tls_int;
                typedef balancer_server<server_hs_with_tls_mod> balancer_server_tls_mod;
#endif
                //Add the no-TLS server for when TLS is not enabled or needed
                typedef balancer_server<server_hs_without_tls> balancer_server_no_tls;

            }
        }
    }
}

#endif /* BALANCER_SERVER_HPP */

