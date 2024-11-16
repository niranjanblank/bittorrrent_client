#pragma once

#include<string>
#include<vector>
#include<cstdint>

struct Torrent{
  std::string info_hash;
  std::string peer_id;
  int port;
  int uploaded;
  int downloaded;
  int left;
  bool compact;

  // constructor
  Torrent(const std::string& ih,const std::string& pid,int p, int up, int down, 
      int l, bool c): info_hash(ih), peer_id(pid), port(p), uploaded(up), downloaded(down), 
  left(l), compact(c){}
};

struct HandshakeMessage{
  uint8_t protocol_length;
  std::string protocol_identifier;
  std::vector<uint8_t> reserved_bytes;
  std::vector<uint8_t> info_hash;
  std::string peer_id;
};
