#ifndef IPPDISCOVERY_H
#define IPPDISCOVERY_H

#include "bytestream.h"
#include "list.h"
#include "map.h"
#include "udpsocket.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <string>

class IppDiscovery
{
public:
  IppDiscovery(std::function<void(std::string)> callback);

  IppDiscovery() = delete;
  IppDiscovery(const IppDiscovery&) = delete;
  IppDiscovery& operator=(const IppDiscovery&) = delete;
  ~IppDiscovery();

  void discover();

private:
  void sendQuery(uint16_t qtype, List<std::string> addrs);
  void update();
  void updateAndQueryPtrs(List<std::string>& ptrs, List<std::string> new_ptrs);

  UdpSocket _socket = UdpSocket("224.0.0.251", 5353);

  uint16_t _transactionId = 0;

  List<std::string> _ippPtrs;
  List<std::string> _ippsPtrs;

  Map<std::string, std::string> _rps;
  Map<std::string, uint16_t> _ports;
  Map<std::string, std::string> _targets;

  Map<std::string, List<std::string>> _As;

  Map<std::pair<uint16_t, std::string>, std::time_t> _outstandingQueries;
  List<std::string> _found;

  std::function<void(std::string)> _callback;

};

#endif // IPPDISCOVERY_H
