/**
 * @file WebSocketsServer.h
 * @date 20.05.2015
 * @author Markus Sattler
 * @co-date 25.12.2019
 * @co-author Aldrin John O. Manalansan
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the WebSockets for Arduino.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
//#include <Arduino.h>
const char camwebpage[] PROGMEM = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 3656\r\n"
"Connection: closed\r\n\r\n"
"<html><head><title>Thesis IP Camera</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>img{position:absolute;top:0 left:0;display:block;margin-left:auto;margin-right:auto;height:100%;width:auto}.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:26px;width:26px;left:4px;bottom:4px;background-color:white;-webkit-transition:.4s;transition:.4s}input:checked + .slider{background-color:#2196F3}input:focus + .slider{box-shadow:0 0 1px #2196F3}input:checked + .slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}.button{background-color:#4CAF50;border:none;color:white;padding:5px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;cursor:pointer}.flipcambutton{border-radius:20px}.restartbutton{border-radius:20px;background-color:#f44336}.dropbtn{background-color:#4CAF50;color:white;padding:5px;font-size:16px;border:none;cursor:pointer}.dropdown{position:absolute;top:0;left:0;display:inline-block}.dropdown-content{display:none;position:absolute;background-color:#f9f9f9;min-width:180px;box-shadow:0 8px 16px 0 rgba(0,0,0,.2);z-index:1}.dropdown-content a{color:black;padding:12px 16px;text-decoration:none;display:block}.dropdown-content a:hover{background-color:#f1f1f1}.dropdown:hover .dropdown-content{display:block}.dropdown:hover .dropbtn{background-color:#3e8e41}</style></head><body style=\"background-color:#FFFFFF\"><img src=\"\"><div class=\"dropdown\"><button class=\"dropbtn\">Settings</button><div class=\"dropdown-content\"><a href=\"#\"><div style=\"position:relative; float:left;\"><b>FlashLight:</b></div><div style=\"position:relative; float:left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"flashlight\" onclick=\"setflashlight()\"><span class=\"slider round\"></span></label></div><br></a><a href=\"#\"><button class=\"button flipcambutton\" onclick=\"flipcamera()\">Flip Camera</button></a><a href=\"#\"><button class=\"button restartbutton\" onclick=\"restartcamera()\">Restart Camera</button></a><a href=\"#\"><select id=\"resolution\" onchange=\"changeresolution()\"><option value=\"10\">UXGA(1600x1200)</option><option value=\"9\">SXGA(1280x1024)</option><option value=\"8\">XGA(1024x768)</option><option value=\"7\">SVGA(800x600)</option><option value=\"6\" selected>VGA(640x480)</option><option value=\"5\">CIF(400x296)</option><option value=\"4\">QVGA(320x240)</option><option value=\"3\">HQVGA(240x176)</option><option value=\"2\">QCIF(176x144)</option><option value=\"1\">QQVGA2(128x160)</option><option value=\"0\">QQVGA(160x120)</option></select></a></div></div><script>\n"
"const img=document.querySelector('img');const WS_URL=(location.protocol==='https:'?'wss':'ws')+'://'+window.location.hostname+'/ws';const ws=new WebSocket(WS_URL);ws.onerror=function(){location.reload(!0)}\n"
"let urlObject;ws.onmessage=message=>{const arrayBuffer=message.data;if(urlObject){URL.revokeObjectURL(urlObject)}\n"
"urlObject=URL.createObjectURL(new Blob([arrayBuffer]));delete arrayBuffer;delete message;img.src=urlObject}\n"
"window.stop();if($.browser.msie){document.execCommand(\"Stop\")};function setflashlight(){ws.send(\"#\"+(document.getElementById(\"flashlight\").checked==!0?1:0))}\n"
"function flipcamera(){ws.send(\"@\")}\n"
"function restartcamera(){ws.send(\"$\")}\n"
"function changeresolution(){ws.send(\"!\"+document.getElementById(\"resolution\").value)}\n"
"</script></body></html>\r\n";

#ifndef WEBSOCKETSSERVER_H_
#define WEBSOCKETSSERVER_H_

#include "WebSockets.h"

#ifndef WEBSOCKETS_SERVER_CLIENT_MAX
#define WEBSOCKETS_SERVER_CLIENT_MAX (8)
#endif

class WebSocketsServer : protected WebSockets {
  public:
#ifdef __AVR__
    typedef void (*WebSocketServerEvent)(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    typedef bool (*WebSocketServerHttpHeaderValFunc)(String headerName, String headerValue);
#else
    typedef std::function<void(uint8_t num, WStype_t type, uint8_t * payload, size_t length)> WebSocketServerEvent;
    typedef std::function<bool(String headerName, String headerValue)> WebSocketServerHttpHeaderValFunc;
#endif

    WebSocketsServer(uint16_t port, String origin = "", String protocol = "arduino");
    virtual ~WebSocketsServer(void);

    void begin(void);
    void close(void);

#if(WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
    void loop(void);
#else
    // Async interface not need a loop call
    void loop(void) __attribute__((deprecated)) {}
#endif

    void onEvent(WebSocketServerEvent cbEvent);
    void onValidateHttpHeader(
        WebSocketServerHttpHeaderValFunc validationFunc,
        const char * mandatoryHttpHeaders[],
        size_t mandatoryHttpHeaderCount);

    bool sendTXT(uint8_t num, uint8_t * payload, size_t length = 0, bool headerToPayload = false);
    bool sendTXT(uint8_t num, const uint8_t * payload, size_t length = 0);
    bool sendTXT(uint8_t num, char * payload, size_t length = 0, bool headerToPayload = false);
    bool sendTXT(uint8_t num, const char * payload, size_t length = 0);
    bool sendTXT(uint8_t num, String & payload);

    bool broadcastTXT(uint8_t * payload, size_t length = 0, bool headerToPayload = false);
    bool broadcastTXT(const uint8_t * payload, size_t length = 0);
    bool broadcastTXT(char * payload, size_t length = 0, bool headerToPayload = false);
    bool broadcastTXT(const char * payload, size_t length = 0);
    bool broadcastTXT(String & payload);

    bool sendBIN(uint8_t num, uint8_t * payload, size_t length, bool headerToPayload = false);
    bool sendBIN(uint8_t num, const uint8_t * payload, size_t length);

    bool broadcastBIN(uint8_t * payload, size_t length, bool headerToPayload = false);
    bool broadcastBIN(const uint8_t * payload, size_t length);

    bool sendPing(uint8_t num, uint8_t * payload = NULL, size_t length = 0);
    bool sendPing(uint8_t num, String & payload);

    bool broadcastPing(uint8_t * payload = NULL, size_t length = 0);
    bool broadcastPing(String & payload);

    void disconnect(void);
    void disconnect(uint8_t num);

    void setAuthorization(const char * user, const char * password);
    void setAuthorization(const char * auth);

    int connectedClients(bool ping = false);

#if(WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC) || (WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP32)
    IPAddress remoteIP(uint8_t num);
#endif

  protected:
    uint16_t _port;
    String _origin;
    String _protocol;
    String _base64Authorization;    ///< Base64 encoded Auth request
    String * _mandatoryHttpHeaders;
    size_t _mandatoryHttpHeaderCount;

    WEBSOCKETS_NETWORK_SERVER_CLASS * _server;

    WSclient_t _clients[WEBSOCKETS_SERVER_CLIENT_MAX];

    WebSocketServerEvent _cbEvent;
    WebSocketServerHttpHeaderValFunc _httpHeaderValidationFunc;

    bool _runnning;

    bool newClient(WEBSOCKETS_NETWORK_CLASS * TCPclient);

    void messageReceived(WSclient_t * client, WSopcode_t opcode, uint8_t * payload, size_t length, bool fin);

    void clientDisconnect(WSclient_t * client);
    bool clientIsConnected(WSclient_t * client);

#if(WEBSOCKETS_NETWORK_TYPE != NETWORK_ESP8266_ASYNC)
    void handleNewClients(void);
    void handleClientData(void);
#endif

    void handleHeader(WSclient_t * client, String * headerLine);

    /**
         * called if a non Websocket connection is coming in.
         * Note: can be override
         * @param client WSclient_t *  ptr to the client struct
         */
    //virtual void handleNonWebsocketConnection(WSclient_t * client) {
    //    DEBUG_WEBSOCKETS("[WS-Server][%d][handleHeader] no Websocket connection close.\n", client->num);
    //    client->tcp->write(
    //        "HTTP/1.1 401 Unauthorized\r\n"
    //        "Server: arduino-WebSocket-Server\r\n"
    //        "Content-Type: text/plain\r\n"
    //        "Content-Length: 45\r\n"
    //        "Connection: close\r\n"
    //        "Sec-WebSocket-Version: 13\r\n"
    //        "WWW-Authenticate: Basic realm=\"WebSocket Server\""
    //        "\r\n"
    //        "This Websocket server requires Authorization!");
    //    clientDisconnect(client);
    //}
	
	virtual void handleNonWebsocketConnection(WSclient_t * client)
	{
		client->tcp->write_P(camwebpage,sizeof(camwebpage));
		//write(client,(uint8_t *)camwebpage,sizeof(camwebpage));
	}

    /**
         * called if a non Authorization connection is coming in.
         * Note: can be override
         * @param client WSclient_t *  ptr to the client struct
         */
    virtual void handleAuthorizationFailed(WSclient_t * client) {
        client->tcp->write(
            "HTTP/1.1 401 Unauthorized\r\n"
            "Server: arduino-WebSocket-Server\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 45\r\n"
            "Connection: close\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "WWW-Authenticate: Basic realm=\"WebSocket Server\""
            "\r\n"
            "This Websocket server requires Authorization!");
        clientDisconnect(client);
    }

    /**
         * called for sending a Event to the app
         * @param num uint8_t
         * @param type WStype_t
         * @param payload uint8_t *
         * @param length size_t
         */
    virtual void runCbEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
        if(_cbEvent) {
            _cbEvent(num, type, payload, length);
        }
    }

    /*
         * Called at client socket connect handshake negotiation time for each http header that is not
         * a websocket specific http header (not Connection, Upgrade, Sec-WebSocket-*)
         * If the custom httpHeaderValidationFunc returns false for any headerName / headerValue passed, the
         * socket negotiation is considered invalid and the upgrade to websockets request is denied / rejected
         * This mechanism can be used to enable custom authentication schemes e.g. test the value
         * of a session cookie to determine if a user is logged on / authenticated
         */
    virtual bool execHttpHeaderValidation(String headerName, String headerValue) {
        if(_httpHeaderValidationFunc) {
            //return the value of the custom http header validation function
            return _httpHeaderValidationFunc(headerName, headerValue);
        }
        //no custom http header validation so just assume all is good
        return true;
    }

  private:
    /*
         * returns an indicator whether the given named header exists in the configured _mandatoryHttpHeaders collection
         * @param headerName String ///< the name of the header being checked
         */
    bool hasMandatoryHeader(String headerName);
};

#endif /* WEBSOCKETSSERVER_H_ */
