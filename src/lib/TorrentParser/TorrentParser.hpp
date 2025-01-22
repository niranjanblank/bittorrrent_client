#pragma once

#include<vector>
#include<cstdint>
#include "lib/nlohmann/json.hpp"
#include "common.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>
#include "lib/bencode/bencode.hpp"
#include "lib/sha1.hpp"
#include "lib/utils.hpp"

struct TorrentFile{
  std::string info_hash;
  std::string peer_id;  
  int port;
  int uploaded;
  int downloaded;
  int left;
  bool compact;
  std::string piece_hash;
  std::string base_url;
  int total_length;
  int piece_length;
  
  // constructor
  TorrentFile(const std::string& ih,const std::string& pid,int p, int up, int down, 
      int l, bool c, const std::string& ph, const std::string& bu, 
      int tl, int pl
      ): info_hash(ih), peer_id(pid), port(p), uploaded(up), downloaded(down), 
  left(l), compact(c), piece_hash(ph), base_url(bu), total_length(tl), piece_length(pl){}
};

// class for parsing Torrent
class TorrentParser{
  public:
    TorrentFile parse(const std::string& filePath){
      json decoded_value =  parse_torrent(filePath);
      
      //std::cout << "Inside TorrentParser" << std::endl; 
      //std::cout << "Tracker URL: "<< decoded_value["announce"].get<std::string>()<<std::endl;
      //std::cout << "Length: "<< decoded_value["info"]["length"].get<int>()<<std::endl;
      
      std::string base_url = decoded_value["announce"].get<std::string>();
      std::string encoded = encode_bencode(decoded_value["info"]);
      std::string info_hash = sha1_hash(encoded);
      std::string piece_hash = to_hex(decoded_value["info"]["pieces"]);
      std::string peer_id = generate_peer_id();
      int total_length = decoded_value["info"]["length"].get<int>();
      int piece_length = decoded_value["info"]["piece length"].get<int>();
      TorrentFile torrent(info_hash,peer_id,6681,0,0,decoded_value["info"]["piece length"],true, 
          piece_hash, base_url, total_length, piece_length);
      return torrent;

    }

  private:
    json parse_torrent(const std::string& filePath){
  
              // read file
      std::ifstream file(filePath, std::ios::binary);

      if(!file){
        throw std::runtime_error("File not found");
      }

      // parse the file
      // get the size of the file
      file.seekg(0, std::ios::end); // move to the end of the file
      std::streamsize fileSize = file.tellg(); // get the file size 
      // return back to the start
      file.seekg(0, std::ios::beg);

      // hold the file content in buffer
      std::vector<char> buffer(fileSize);
      // read the file to buffer;
      if (!file.read(buffer.data(), fileSize)){
        // if the file cannot be read
        throw std::runtime_error("Couldnt read the file");
      }
      
      // converting the buffer to string
      std::string encoded_value(buffer.begin(), buffer.end());
      // start index for parsing
      size_t index = 0;
      json decoded_value = decode_bencode(encoded_value, index);
     // std::string input_encoded_value = argv[1];
      // json decoded_value = decode_bencoded_value(input_encoded_value,0);
      return decoded_value;
    };
      
};
