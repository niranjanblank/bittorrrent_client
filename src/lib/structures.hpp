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

struct PeerMessage{
  uint32_t length; // message length prefix
  uint8_t id; // message id
  std::vector<uint8_t> payload;
};

struct PieceInfo {
    uint32_t piece_index;
    uint32_t piece_length;
    std::string piece_hash;
  
    PieceInfo(uint32_t index, uint32_t length, const std::string& hash)
        : piece_index(index), piece_length(length), piece_hash(hash) {}
};
