/* 
 * File:   client_parameters.hpp
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
 * Created on January 26, 2016, 12:13 PM
 */

#ifndef CLIENT_PARAMETERS_HPP
#define CLIENT_PARAMETERS_HPP

#include <regex>
#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/websocket/websocket_client_params.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::common::messaging::websocket;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This structure stores the translation client execution parameters
                 */
                struct client_parameters_struct {
                    //Stores the default TLS mode
                    static const string CL_DEF_TLS_MODE;
                    //Stores the default TLS ciphers
                    static const string CL_DEF_TLS_CIPHERS;

                    //Stores the default pre-processor uri
                    static const string CL_DEF_PRE_PROC_URI;
                    //Stores the default translator uri
                    static const string CL_DEF_TRANS_URI;
                    //Stores the default post-processor uri
                    static const string CL_DEF_POST_PROC_URI;

                    //Stores the default minimum number of sentences to be sent per job
                    static const uint32_t CL_DEF_MIN_SENT_VAL;
                    //Stores the default minimum number of sentences to be sent per job
                    static const uint32_t CL_DEF_MAX_SENT_VAL;
                    //Stores the default job priority
                    static const int32_t CL_DEF_PRIORITY_VAL;
                    //Stores the default value for the flag to request translation info
                    static const bool CL_DEF_IS_TRANS_INFO_VAL;

                    //The main client configuration section name
                    static const string CL_CONFIG_SECTION_NAME;
                    //The pre-processor parameters section name
                    static const string CL_PRE_PARAMS_SECTION_NAME;
                    //The translator parameters section name
                    static const string CL_TRANS_PARAMS_SECTION_NAME;
                    //The post-processor parameters section name
                    static const string CL_POST_PARAMS_SECTION_NAME;

                    //The minimum sentence count parameter name
                    static const string CL_MIN_SENT_PARAM_NAME;
                    //The maximum sentence count parameter name
                    static const string CL_MAX_SENT_PARAM_NAME;
                    //The job priority parameter name
                    static const string CL_PRIORITY_PARAM_NAME;
                    //The translation into request flag parameter name
                    static const string CL_IS_TRANS_INFO_PARAM_NAME;

                    //The source file name with the text to translate
                    string m_source_file;
                    //The language to translate from
                    string m_source_lang;
                    //The target file name to put the result into
                    string m_target_file;
                    //The language to translate into
                    string m_target_lang;

                    //The pre-processor client parameters
                    websocket_client_params m_pre_params;
                    //The translation client parameters
                    websocket_client_params m_trans_params;
                    //The post-processor client parameters
                    websocket_client_params m_post_params;

                    //The flag indicating whether the client requests the translation details from the translation server or not.
                    bool m_is_trans_info;
                    //The maximum number of source sentences to send per translation request
                    uint64_t m_max_sent;
                    //The minimum number of source sentences to send per translation request
                    uint64_t m_min_sent;

                    //Stores the priority to be used for this client
                    int32_t m_priority;

                    /**
                     * The basic constructor
                     */
                    client_parameters_struct() :
                    m_source_file(""), m_source_lang(""),
                    m_target_file(""), m_target_lang(""),
                    m_pre_params("pre-processor"),
                    m_trans_params("translation"),
                    m_post_params("post-processor"),
                    m_is_trans_info(false), m_max_sent(0),
                    m_min_sent(0), m_priority(0) {
                    }

                    /**
                     * Allows to check if pre-processing is needed
                     * @return true if pre-processing is needed
                     */
                    inline bool is_pre_process() const {
                        return !m_pre_params.m_server_uri.empty();
                    }

                    /**
                     * Allows to check if post-processing is needed
                     * @return true if post-processing is needed
                     */
                    inline bool is_post_process() const {
                        return !m_post_params.m_server_uri.empty();
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    inline void finalize() {
                        //Check the pre-processor server uri format
                        if (is_pre_process()) {
                            m_pre_params.finalize();
                        }

                        //Check the translation server uri format
                        m_trans_params.finalize();

                        //Check the post-processor server uri format
                        if (is_post_process()) {
                            m_post_params.finalize();
                        }
                    }
                };

                typedef client_parameters_struct client_parameters;

                /**
                 * Allows to output the parameters object to the stream
                 * @param stream the stream to output into
                 * @param params the parameters object
                 * @return the stream that we output into
                 */
                static inline std::ostream& operator<<(
                        std::ostream& stream, const client_parameters & params) {
                    stream << "Translation client parameters: { "
                            << "source file = "
                            << params.m_source_file
                            << ", source language = "
                            << params.m_source_lang
                            << ", target file = "
                            << params.m_target_file
                            << ", target language = "
                            << params.m_target_lang
                            << ", pre-processor server = ";
                    if (params.is_pre_process()) {
                        stream << params.m_pre_params;
                    } else {
                        stream << "NONE";
                    }
                    stream << ", translation server = "
                            << params.m_trans_params
                            << ", post-processor server = ";
                    if (params.is_post_process()) {
                        stream << params.m_post_params;
                    } else {
                        stream << "NONE";
                    }
                    return stream << ", min sentences per request = "
                            << params.m_min_sent
                            << ", max sentences per request = "
                            << params.m_max_sent
                            << ", request priority = "
                            << params.m_priority
                            << ", translation info = "
                            << (params.m_is_trans_info ? "ON" : "OFF")
                            << " }";
                }
            }
        }
    }
}

#endif