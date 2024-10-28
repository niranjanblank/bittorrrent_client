#include<iostream>
#include<string>
#include<fstream>
#include<iomanip>
#include <sstream>
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "lib/bencode/bencode.hpp"
#include "lib/utils.hpp"
#include "lib/httplib.h"
using json = nlohmann::json;


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

json peer_discovery(std::string& base_url, Torrent& torrent ){
  httplib::Client client(base_url);

  // params object to hold query parameters
  httplib::Params params;
  params.emplace("info_hash", torrent.info_hash);
  params.emplace("peer_id", torrent.peer_id);
  params.emplace("port", std::to_string(torrent.port));
  params.emplace("uploaded", std::to_string(torrent.uploaded));
  params.emplace("downloaded", std::to_string(torrent.downloaded));
  params.emplace("left",std::to_string(torrent.left));
  params.emplace("compact", torrent.compact?"1":"0");

  // Create a JSON object
    json jsonParams;
    for (const auto& param : params) {
        jsonParams[param.first] = param.second; // Add key-value pairs to the JSON object
    }

  return jsonParams;

}


// parsing the torrent file
json parse_torrent_file(std::string& file_name){
    // read file
      std::ifstream file(file_name, std::ios::binary);

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
   
}




int main(int argc, char* argv[]){
  if(argc>1){
    
    // read torrent file
    try{
      std::string file_name = argv[1];
      json decoded_value = parse_torrent_file(file_name);
 // std::string input_encoded_value = argv[1];
      // json decoded_value = decode_bencoded_value(input_encoded_value,0);
      std::cout << "Tracker URL: "<< decoded_value["announce"].get<std::string>()<<std::endl;
      std::cout << "Length: "<< decoded_value["info"]["length"].get<int>()<<std::endl;
      
      std::string base_url = decoded_value["announce"].get<std::string>();
      std::string encoded = encode_bencode(decoded_value["info"]);
      
      std::string info_hash = sha1_hash(encoded);
      std::cout <<"Info Hash: "<<sha1_hash(encoded)<<std::endl;
      std::cout <<"Piece Length: "<<decoded_value["info"]["piece length"].get<int>()<<std::endl;
      std::string piece_hash = to_hex(decoded_value["info"]["pieces"]);
      std::cout <<"Piece Hashes: "<<piece_hash<<std::endl;

      // peer peer_discovery
      Torrent torrent(info_hash,"00112233445566778899",6681,0,0,decoded_value["info"]["piece length"],true);
      json torrent_test = peer_discovery(base_url,torrent);
      std::cout << torrent_test.dump();

   }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
