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
  enum QType : uint16_t
  {
    A = 1,
    PTR = 12,
    TXT = 16,
    AAAA = 28,
    SRV = 33,
    ALL = 255 //for querying
  };

  IppDiscovery(std::function<void(std::string)> callback);

  IppDiscovery() = delete;
  IppDiscovery(const IppDiscovery&) = delete;
  IppDiscovery& operator=(const IppDiscovery&) = delete;

  void discover();

private:
  void sendQuery(QType qtype, List<std::string> addrs);
  void update();
  void updateAndQueryPtrs(List<std::string>& ptrs, const List<std::string>& newPtrs);

  bool hasTxtEntry(const std::string& ptr, const std::string& key);

  UdpSocket _socket = UdpSocket("224.0.0.251", 5353);

  uint16_t _transactionId = 0;

  List<std::string> _ippPtrs;
  List<std::string> _ippsPtrs;

  Map<std::string, Map<std::string, std::string>> _TXTs;
  Map<std::string, uint16_t> _ports;
  Map<std::string, std::string> _targets;

  Map<std::string, List<std::string>> _As;

  Map<std::pair<QType, std::string>, std::time_t> _outstandingQueries;
  List<std::string> _found;

  std::function<void(std::string)> _callback;

};

#endif // IPPDISCOVERY_H
