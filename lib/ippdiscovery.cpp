#include "ippdiscovery.h"

#include "log.h"
#include "stringutils.h"

#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace std::literals;

std::ostream& operator<<(std::ostream& os, const List<std::string>& sl)
{
  os << join_string(sl, ", ");
  return os;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const Map<T, U>& m)
{
  List<std::string> sl;
  for(const std::pair<T, U> p : m)
  {
    std::stringstream ss;
    ss << p.first << ": " << p.second;
    sl.push_back(ss.str());
  }
  os << sl;
  return os;
}

std::string ip4str(uint32_t ip)
{
  return std::to_string((ip >> 24) & 0xff) + "."
       + std::to_string((ip >> 16) & 0xff) + "."
       + std::to_string((ip >> 8) & 0xff) + "."
       + std::to_string(ip & 0xff);
}

std::string make_addr(const std::string& proto, uint16_t defaultPort, uint16_t port, const std::string& ip, const std::string& rp)
{
  std::string maybePort = port != defaultPort ? ":"+std::to_string(port) : "";
  std::string addr = proto+"://"+ip+maybePort+"/"+rp;
  return addr;
}

std::string make_ipp_addr(uint16_t port, const std::string& ip, const std::string& rp)
{
  return make_addr("ipp", 631, port, ip, rp);
}

std::string make_ipps_addr(uint16_t port, const std::string& ip, const std::string& rp)
{
  return make_addr("ipps", 443, port, ip, rp);
}

List<std::string> get_addr(Bytestream& bts, std::set<uint16_t> seenReferences={})
{
  List<std::string> addr;
  while(!(bts.next<uint8_t>(0)))
  {
    if((bts.peek<uint8_t>()&0xc0)==0xc0)
    {
      uint16_t ref = bts.get<uint16_t>() & 0x0fff;
      if(seenReferences.count(ref) != 0)
      {
        throw(std::logic_error("Circular reference"));
      }
      seenReferences.insert(ref);
      Bytestream tmp = bts;
      tmp.setPos(ref);
      addr += get_addr(tmp, seenReferences);
      break;
    }
    else
    {
      addr.push_back(bts.getString(bts.get<uint8_t>()));
    }
  }
  return addr;
}

std::string get_addr_str(Bytestream& bts)
{
  return join_string(get_addr(bts), ".");
}

IppDiscovery::IppDiscovery(std::function<void(std::string)> callback)
: _callback(std::move(callback))
{
}

void IppDiscovery::sendQuery(QType qtype, List<std::string> addrs)
{
  std::chrono::time_point<std::chrono::system_clock> nowClock = std::chrono::system_clock::now();
  std::time_t now = std::chrono::system_clock::to_time_t(nowClock);
  std::time_t aWhileAgo = std::chrono::system_clock::to_time_t(nowClock - 2s);

  for(const auto& [k, v] : _outstandingQueries)
  {
    if(_outstandingQueries[k] < aWhileAgo)
    { // Housekeeping for _outstandingQueries
      DBG(<< "skipping " << k.second);
      _outstandingQueries.erase(k);
    }
    else if(k.first == qtype && addrs.contains(k.second))
    { // we recently asked about this, remove it
      addrs.remove(k.second);
    }
  }

  if(addrs.empty())
  {
    return;
  }

  DBG(<< "querying " << (uint16_t)qtype << " " << addrs);

  Bytestream query;
  Map<std::string, uint16_t> suffixPositions;

  uint16_t flags = 0;
  uint16_t questions = addrs.size();

  query << _transactionId++ << flags << questions << (uint16_t)0 << (uint16_t)0 << (uint16_t)0;

  for(const std::string& addr : addrs)
  {
    _outstandingQueries.insert({{qtype, addr}, now});

    List<std::string> addrParts = split_string(addr, ".");
    std::string addrPart;
    std::string restAddr;
    while(!addrParts.empty())
    {
      restAddr = join_string(addrParts, ".");
      if(suffixPositions.contains(restAddr))
      {
        query << (uint16_t)(0xc000 | (0x0fff & suffixPositions[restAddr]));
        break;
      }
      else
      {
        // We are putting in at least one part of the address, remember where that was
        suffixPositions.insert({restAddr, query.size()});
        addrPart = addrParts.takeFront();
        query << (uint8_t)addrPart.size() << addrPart;
      }
    }
    if(addrParts.empty())
    {
      // Whole addr was put in without c-pointers, 0-terminate it
      query << (uint8_t)0;
    }

    query << (uint16_t)qtype << (uint16_t)0x0001;

  }

  _socket.send(query);

}

void IppDiscovery::update()
{
  List<std::pair<std::string,std::string>> ippsIpRps;
  std::string target;
  std::string rp;

  for(const std::string& it : _ippsPtrs)
  {
    if(!_targets.contains(it) || !_ports.contains(it) || !(_TXTs.contains(it) && _TXTs[it].contains("rp")))
    {
      continue;
    }

    uint16_t port = _ports.at(it);
    target = _targets.at(it);
    rp = _TXTs.at(it).at("rp");

    if(_As.contains(target))
    {
      for(const std::string& ip : _As.at(target))
      {
        std::string addr = make_ipps_addr(port, ip, rp);
        if(!_found.contains(addr))
        { // Don't add duplicates
          ippsIpRps.push_back({ip, rp});
          _found.push_back(addr);
          _callback(addr);
        }
      }
    }
  }

  for(const std::string& it : _ippPtrs)
  {
    if(!_targets.contains(it) || !_ports.contains(it) || !(_TXTs.contains(it) && _TXTs[it].contains("rp")))
    {
      continue;
    }

    uint16_t port = _ports.at(it);
    target = _targets.at(it);
    rp = _TXTs.at(it).at("rp");

    if(_As.contains(target))
    {
      for(const std::string& ip : _As.at(target))
      {
        std::string addr = make_ipp_addr(port, ip, rp);
        if(!_found.contains(addr) && !ippsIpRps.contains({ip, rp}))
        { // Don't add duplicates, don't add if already added as IPPS
          _found.push_back(addr);
          _callback(addr);
        }
      }
    }
  }
}

void IppDiscovery::updateAndQueryPtrs(List<std::string>& ptrs, const List<std::string>& newPtrs)
{
  for(const std::string& ptr : newPtrs)
  {
    if(ptrs.contains(ptr))
    {
      continue;
    }
    else
    {
      ptrs.push_back(ptr);
    }
    // If pointer does not resolve to a target or is missing information, query about it
    if(!_TXTs.contains(ptr))
    {
      // Avahi *really* hates sending TXT to anything else than a TXT query
      sendQuery(TXT, {ptr});
    }
    if(!_targets.contains(ptr) || !_ports.contains(ptr))
    {
      sendQuery(SRV, {ptr});
    }
  }
}

void IppDiscovery::discover()
{
  sendQuery(PTR, {"_ipp._tcp.local", "_ipps._tcp.local"});

  Bytestream resp;

  while((resp = _socket.receive(2)))
  {
    List<std::string> newIppPtrs;
    List<std::string> newIppsPtrs;
    List<std::string> newTargets;

    std::string qaddr;
    std::string aaddr;
    std::string tmpname;
    std::string target;

    uint16_t transactionidResp;
    uint16_t flags;
    uint16_t questions;
    uint16_t answerRRs;
    uint16_t authRRs;
    uint16_t addRRs;

    try
    {
      resp >> transactionidResp >> flags >> questions >> answerRRs >> authRRs >> addRRs;
      size_t totalRRs = answerRRs + authRRs + addRRs;

      for(size_t i = 0; i < questions; i++)
      {
        uint16_t qtype;
        uint16_t qflags;
        qaddr = get_addr_str(resp);
        resp >> qtype >> qflags;
      }

      for(size_t i = 0; i < totalRRs; i++)
      {
        uint16_t atype;
        uint16_t aflags;
        uint16_t len;
        uint32_t ttl;

        aaddr = get_addr_str(resp);
        resp >> atype >> aflags >> ttl >> len;

        uint16_t pos_before = resp.pos();
        switch(atype)
        {
          case PTR:
          {
            tmpname = get_addr_str(resp);
            if(string_ends_with(aaddr, "_ipp._tcp.local"))
            {
              newIppPtrs.push_back(tmpname);
            }
            else if(string_ends_with(aaddr, "_ipps._tcp.local"))
            {
              newIppsPtrs.push_back(tmpname);
            }
            break;
          }
          case TXT:
          {
            while(resp.pos() < pos_before+len)
            {
              std::string txt = resp.getString(resp.get<uint8_t>());
              size_t pos = txt.find('=');
              std::string key = txt.substr(0, pos);
              std::string value = txt.substr(pos+1);
              _TXTs[aaddr][key] = value;
            }
            break;
          }
          case SRV:
          {
            uint16_t prio;
            uint16_t w;
            uint16_t port;
            resp >> prio >> w >> port;
            target = get_addr_str(resp);
            _ports[aaddr] = port;
            _targets[aaddr] = target;
            if(!newTargets.contains(target))
            {
              newTargets.push_back(target);
            }
            break;
          }
          case A:
          {
            std::string ip = ip4str(resp.get<uint32_t>());
            _As[aaddr].push_back(ip);
            break;
          }
          default:
          {
            resp += len;
            break;
          }

          if(resp.pos() != pos_before + len)
          {
            throw(std::logic_error("failed to parse DNS record"));
          }
        }
      }
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what();
    }

    DBG(<< "------------");
    DBG(<< "new ipp ptrs: " << newIppPtrs);
    DBG(<< "new ipps ptrs: " << newIppsPtrs);
    DBG(<< "ipp ptrs: " << _ippPtrs);
    DBG(<< "ipps ptrs: " << _ippsPtrs);
    DBG(<< "TXTs: " << _TXTs);
    DBG(<< "ports: " << _ports);
    DBG(<< "new targets: " << newTargets);
    DBG(<< "targets: " << _targets);
    DBG(<< "As: " << _As);

    // These will send one query per unique new ptr.
    // some responders doesn't give TXT records for more than one thing at at time :(
    updateAndQueryPtrs(_ippsPtrs, newIppsPtrs);
    updateAndQueryPtrs(_ippPtrs, newIppPtrs);

    List<std::string> unresolvedAddrs;

    for(const std::string& t : newTargets)
    {
      // If target does not resolve to an address, query about it
      if(!_As.contains(t))
      {
        unresolvedAddrs.push_back(t);
      }
    }

    if(!unresolvedAddrs.empty())
    {
      sendQuery(A, unresolvedAddrs);
    }

    update();

  }
}
