/* Modified version of ESP-Arduino WiFiServer.cpp/h */
#ifndef _FN_TCPSERVER_H_
#define _FN_TCPSERVER_H_


#include <memory>
//#include <lwip/sockets.h>

#include "fnTcpClient.h"

class fnTcpServer
{
  private:
    int _sockfd = -1;
    int _accepted_sockfd = -1;
    uint16_t _port;
    uint8_t _max_clients;
    bool _listening = false;
    bool _noDelay = false;

  public:
    void listenOnLocalhost(){}

    fnTcpServer(uint16_t port=80, uint8_t max_clients=4): _port(port), _max_clients(max_clients){}
    ~fnTcpServer(){ stop(); }

    void begin(uint16_t port=0);

    fnTcpClient accept(){ return available(); }
    void setNoDelay(bool nodelay) { _noDelay = nodelay; };
    bool getNoDelay() {return _noDelay; };
    int setTimeout(uint32_t seconds);
    void setMaxClients(int maxClients) { _max_clients = maxClients; }

    bool hasClient();
    fnTcpClient available();

    void stop();

    operator bool(){ return _listening; }
};


#endif // _FN_TCPSERVER_H_
