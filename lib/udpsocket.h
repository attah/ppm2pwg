#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "bytestream.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

class UdpSocket
{
public:
  UdpSocket() = delete;
  UdpSocket(const UdpSocket&) = delete;
  UdpSocket& operator=(const UdpSocket&) = delete;

  UdpSocket(const std::string& addr, uint16_t port)
  {
    // Creating socket file descriptor
    if((_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
      throw(std::runtime_error("socket creation failed"));
    }

    memset(&_servaddr, 0, sizeof(_servaddr));
    _servaddr.sin_family = AF_INET;
    _servaddr.sin_port = htons(port);
    _servaddr.sin_addr.s_addr = inet_addr(addr.c_str());
  }

  ~UdpSocket()
  {
    close(_sock);
  }

  ssize_t send(const Bytestream& msg)
  {
    return sendto(_sock, (const char*)msg.raw(), msg.size(), 0,
                  (const sockaddr*)&_servaddr, sizeof(_servaddr));
  }

  Bytestream receive(time_t tmo_sec=0, time_t tmo_usec=0) const
  {
    Bytestream msg;
    std::array<char, BS_REASONABLE_FILE_SIZE> buffer;

    timeval tv;
    tv.tv_sec = tmo_sec;
    tv.tv_usec = tmo_usec;
    if(setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
      throw(std::runtime_error("setsockopt failed"));
    }

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    socklen_t len = sizeof(servaddr);

    ssize_t n = recvfrom(_sock, buffer.data(), sizeof(buffer), 0, (sockaddr*)&servaddr, &len);
    if(n > 0)
    {
      msg = Bytestream(buffer.data(), n);
    }
    return msg;
  }

private:
  int _sock;
  sockaddr_in _servaddr;

};

#endif // CONVERTER_H
