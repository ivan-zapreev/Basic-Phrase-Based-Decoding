//Stores the number of progress bars for the translation process
var NUM_PROGRESS_BARS = 2;

/**
 * Allows to create a new web socket server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the web socket client module
 */
function create_ws_client(logger_mdl, url_input, url, server_cs_img, server_cs_bage,
                           needs_new_trans_fn, disable_interface_fn, enable_interface_fn,
                           on_open_fn, on_message_fn, on_close_fn, escape_html_fn,
                          request_progress_bar, response_progress_bar) {
    "use strict";
    
    //Create the first prototype of the client module
    var client = {
        PROTOCOL_VERSION : 1,
        MSG_TYPE_ENUM : {
            //The message type is undefined
            MESSAGE_UNDEFINED : 0,
            //The supported languages request message
            MESSAGE_SUPP_LANG_REQ : 1,
            //The supported languages response message
            MESSAGE_SUPP_LANG_RESP : 2,
            //The translation job request message
            MESSAGE_TRANS_JOB_REQ : 3,
            //The translation job response message
            MESSAGE_TRANS_JOB_RESP : 4
        },
        STATUS_CODE_ENUM : {
            RESULT_UNDEFINED : 0,
            RESULT_UNKNOWN : 1,
            RESULT_OK : 2,
            RESULT_PARTIAL : 3,
            RESULT_CANCELED : 4,
            RESULT_ERROR : 5
        },
        url : url,               //The web server url
        ws : null               //The web socket to the server
    };
    
    window.console.log("Creating a new ws client for the server: " + url);
    
    //Set the url into the server input
    url_input.val(url);

    /**
     * Allows to set the progress bar progress
     * @param {Boolean} is_init true if this is for (re-)initialization of the progress bar
     * @param {jquery Object} the jquesy object of the progress bar
     * @param {String} the message to be visualized
     * @param {Number} the current value for the progress bar
     * @param {Number} the maximum value for the progress bar
     * @param {Number} the number of translation process progress bars
     */
    function set_progress_bar(is_init, pb, msg, curr_num, max_num, num_prog_bars) {
        window.console.log("The current value: " + curr_num + " max value: " + max_num);

        var percent = window.Math.round((curr_num / max_num) * 100), span = pb.find("span");
        pb.attr("aria-valuenow", percent);
        pb.width((percent / NUM_PROGRESS_BARS) + "%");

        if (is_init) {
            span.html("");
        } else {
            span.html(escape_html_fn(msg + ": " + percent + "%"));
        }

        if (percent === 0 && !is_init) {
            pb.addClass("active");
        }
        if (percent === 100 || is_init) {
            pb.removeClass("active");
        }
    }

    /**
     * Allows to re-initialize the request/response progress bars
     */
    function initialize_progress_bars() {
        set_progress_bar(true, response_progress_bar, "Responses", 0, 1);
        set_progress_bar(true, request_progress_bar, "Requests", 0, 1);
    }

    /**
     * Allows to set the new value for the translation responses progress bar
     * @param {integer} curr the current value
     * @param {integer} max the maximum value
     */
    function set_response_progress_bar(curr, max) {
        set_progress_bar(false, response_progress_bar, "Responses", curr, max);
    }

    /**
     * Allows to set the new value for the translation requests progress bar
     * @param {integer} curr the current value
     * @param {integer} max the maximum value
     */
    function set_request_progress_bar(curr, max) {
        set_progress_bar(false, request_progress_bar, "Requests", curr, max);
    }
    
    //Export the request/response progress bar setters, initiaalizers
    client.set_response_pb_fn = set_response_progress_bar;
    client.set_request_pb_fn = set_request_progress_bar;
    client.init_req_resp_pb_fn = initialize_progress_bars;

    /**
     * This function allows to update the current connection status
     * in the GUI. We use the same values as the Websocket connection.
     * @param {Number} ws_status the Webscoket status value to be stored
     */
    function update_conn_status(ws_status) {
        switch (ws_status) {
        case window.WebSocket.CONNECTING:
            server_cs_img.attr("src", "./img/changing.gif");
            server_cs_img.attr("alt", "Connecting ...");
            server_cs_img.attr("data-original-title", "Connecting ...");
            server_cs_bage.attr("class", "badge changing");
            break;
        case window.WebSocket.OPEN:
            server_cs_img.attr("src", "./img/connected.png");
            server_cs_img.attr("alt", "Connected");
            server_cs_img.attr("data-original-title", "Connected");
            server_cs_bage.attr("class", "badge online");
            break;
        case window.WebSocket.CLOSING:
            server_cs_img.attr("src", "./img/changing.gif");
            server_cs_img.attr("alt", "Disconnecting ...");
            server_cs_img.attr("data-original-title", "Disconnecting ...");
            server_cs_bage.attr("class", "badge changing");
            break;
        case window.WebSocket.CLOSED:
            server_cs_img.attr("src", "./img/disconnected.png");
            server_cs_img.attr("alt", "Disconnected");
            server_cs_img.attr("data-original-title", "Disconnected");
            server_cs_bage.attr("class", "badge offline");
            break;
        default:
            server_cs_img.attr("src", "./img/puzzled.png");
            server_cs_img.attr("alt", "Puzzled :)");
            server_cs_img.attr("data-original-title", "Puzzled :)");
            server_cs_bage.attr("class", "badge puzzled");
            break;
        }
    }

    /**
     * This function is called if one needs to connect or re-connect to the translation server.
     */
    function connect_to_server() {
        window.console.log("Disable the controls before connecting to a new server");
        disable_interface_fn();

        try {
            window.console.log("Checking that the web socket connection is available");
            if (window.hasOwnProperty("WebSocket")) {
                window.console.log("Close the web socket connection if there is one");
                if ((client.ws !== null) && ((client.ws.readyState === window.WebSocket.CONNECTING) ||
                                            (client.ws.readyState === window.WebSocket.OPEN))) {
                    logger_mdl.info("Closing the previously opened connection");
                    update_conn_status(window.WebSocket.CLOSING);
                    client.ws.close();
                }

                logger_mdl.info("Opening a new web socket to the server: " + client.url);
                client.ws = new window.WebSocket(client.url);

                update_conn_status(window.WebSocket.CONNECTING);

                window.console.log("Set up the socket handler functions");
                
                //Set the on connection open handler
                client.ws.onopen = function () {
                    logger_mdl.success("The connection to '" + client.url + "' is open");
                    update_conn_status(window.WebSocket.OPEN);

                    //Call the on open connection handler
                    if (on_open_fn) {
                        on_open_fn();
                    } else {
                        logger_mdl.warning("The on-open handler for server " + url + " is not set!");
                    }

                    //Enable the interface controls
                    enable_interface_fn();
                };
                
                //Set the on message handler
                client.ws.onmessage = function (evt) {
                    //log the json data
                    window.console.log("Received message:" + evt.data);

                    window.console.log("Parsing to JSON");
                    var resp_obj = JSON.parse(evt.data);

                    //Check if the message type is detectable
                    if (resp_obj.hasOwnProperty('msg_type')) {
                        //Call the on message handler with the json object
                        if (on_message_fn) {
                            on_message_fn(resp_obj);
                        } else {
                            logger_mdl.warning("The on-message handler for server " + url + " is not set!");
                        }
                    } else {
                        logger_mdl.danger("Malformed server message: " + evt.data);
                    }
                };
                
                //Set the on connection close handler
                client.ws.onclose = function () {
                    update_conn_status(window.WebSocket.CLOSED);
                    
                    //Call the on closed connection handler
                    if (on_close_fn) {
                        on_close_fn();
                    } else {
                        logger_mdl.warning("The on-close handler for server " + url + " is not set!");
                    }
                    //Re-initialize the progress bars
                    initialize_progress_bars();

                    //Enable the interface controls
                    enable_interface_fn();
                };
            } else {
                //Disable the web page
                logger_mdl.danger("The WebSockets are not supported by your browser!");
            }
        } catch (err) {
            enable_interface_fn();
        }
    }
        
    /**
     * This function is called if the server URL change event is fired,
     * so we need to check if we need to (re-)connect.
     */
    function on_url_change() {
        var url;

        //Get the current value and trim it
        url = url_input.val().trim();
        //Put the trimmed value back into the input
        url_input.val(url);

        window.console.log("The new server url is: " + url);

        if ((client.ws === null) || (client.ws.readyState !== window.WebSocket.OPEN) || (client.url !== url)) {

            //A new server means new translation
            needs_new_trans_fn();

            window.console.log("Storing the new server url value");
            client.url = url;

            if (client.url !== "") {
                window.console.log("Connecting to the new server url");
                connect_to_server();
            }
        } else {
            window.console.log("The server url has not changed");
        }
    }

    //Set the server change handlers
    url_input.change(on_url_change);
    url_input.focus(on_url_change);
    
    //Set the server connection functions
    client.connect_fn = connect_to_server;
    client.is_connected_fn = function () {
        return ((client.ws !== null) && (client.ws.readyState === window.WebSocket.OPEN));
    };
    
    //Re-set progress bars
    initialize_progress_bars();

    return client;
}