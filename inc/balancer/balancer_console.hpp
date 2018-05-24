/* 
 * File:   balancer_console.hpp
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
 * Created on July 7, 2016, 12:08 PM
 */

#ifndef BALANCER_CONSOLE_HPP
#define BALANCER_CONSOLE_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/cmd/cmd_line_base.hpp"

#include "balancer/balancer_server.hpp"
#include "balancer/balancer_manager.hpp"
#include "balancer/bl_cmd_line_client.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::cmd;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                //The number of worker threads - for the incoming messages pool
                static const string PROGRAM_SET_INT_CMD = "set int ";
                //The number of worker threads - for the outgoing messages pool
                static const string PROGRAM_SET_ONT_CMD = "set ont ";

                /**
                 * This is the load balancer console class:
                 * Responsibilities:
                 *      Provides the balancer console
                 *      Allows to execute commands
                 *      Allows to get run time information
                 *      Allows to change run time settings
                 */
                class balancer_console : public cmd_line_base {
                public:

                    /**
                     * The basic constructor
                     * @param params the balancer parameters
                     * @param client the command line client
                     * @param balancer_thread the balancer server main thread
                     */
                    balancer_console(balancer_parameters & params, bl_cmd_line_client &client, thread &balancer_thread)
                    : cmd_line_base(), m_params(params), m_client(client), m_balancer_thread(balancer_thread) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~balancer_console() {
                        //If the user did not do stop, i,e, we are likely to exit because of an exception
                        if (!m_is_stopped) {
                            //Stop the thing
                            stop();
                        }
                    }

                protected:

                    /**
                     * @see cmd_line_base
                     */
                    virtual void print_specific_commands() {
                        print_command_help(PROGRAM_SET_INT_CMD, "<positive integer>", "set the number of incoming pool threads");
                        print_command_help(PROGRAM_SET_ONT_CMD, "<positive integer>", "set the number of outgoing pool threads");
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_run_time_info() {
                        //Report the run time info from the server
                        m_client.report_run_time_info();
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual bool process_specific_cmd(const string & cmd) {
                        //Set the number of incoming pool threads
                        if (begins_with(cmd, PROGRAM_SET_INT_CMD)) {
                            set_num_threads(cmd, PROGRAM_SET_INT_CMD,
                                    [&](int32_t num_threads)->void {
                                        //Set the number of threads
                                        m_client.set_num_inc_threads(num_threads);
                                        //Remember the new number of threads
                                        m_params.m_num_req_threads = num_threads;
                                    });
                            return false;
                        } else {
                            //Set the number of outgoing pool threads
                            if (begins_with(cmd, PROGRAM_SET_ONT_CMD)) {
                                set_num_threads(cmd, PROGRAM_SET_ONT_CMD,
                                        [&](int32_t num_threads)->void {
                                            //Set the number of threads
                                            m_client.set_num_out_threads(num_threads);
                                            //Remember the new number of threads
                                            m_params.m_num_resp_threads = num_threads;
                                        });
                                return false;
                            } else {
                                return true;
                            }
                        }
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void report_program_params() {
                        LOG_USAGE << m_params << END_LOG;
                    }

                    /**
                     * @see cmd_line_base
                     */
                    virtual void stop() {
                        //Stop the translation server
                        m_client.request_stop();

                        //Wait until the server's thread stops
                        m_balancer_thread.join();

                        LOG_USAGE << "The balancer server thread is stopped!" << END_LOG;
                    }

                private:
                    //Stores the reference to the balancer parameters
                    balancer_parameters & m_params;
                    //Stores the reference to the command line client
                    bl_cmd_line_client & m_client;
                    //Stores the reference to the balancer thread
                    thread & m_balancer_thread;

                };
            }
        }
    }
}

#endif /* BALANCER_CONSOLE_HPP */

