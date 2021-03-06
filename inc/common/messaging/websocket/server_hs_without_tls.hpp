/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dummy_tls_handshake.hpp
 * Author: Dr. Ivan S. Zapreev <ivan.zapreev@gmail.com>
 *
 * Created on May 25, 2018, 5:16 PM
 */

#ifndef DUMMY_TLS_HANDSHAKE_HPP
#define DUMMY_TLS_HANDSHAKE_HPP

#include <string>

#define ASIO_STANDALONE
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

#include "common/messaging/websocket/websocket_server_params.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This is a dummy TLS handshake class that is to be used when NO TLS is needed
                         */
                        class server_hs_without_tls {
                        public:
                            //Define the server type
                            typedef websocketpp::server<websocketpp::config::asio> server_type;

                            /**
                             * The dummy constructor, is only needed for proper compilation
                             * @param params NOT USED as it is a STUB
                             * @param server NOT USED as it is a STUB
                             */
                            server_hs_without_tls(
                                    const websocket_server_params & params,
                                    server_type & server) {
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* DUMMY_TLS_HANDSHAKE_HPP */

