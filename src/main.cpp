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


// Function for peer discovery
std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;

    size_t i = 0;
    while(i<value.size()){
        std::string two_chars = value.substr(i,2);
        escaped << "%" << two_chars;
    
        i = i+2;
    }

    
    return escaped.str();
}
// Function for peer discovery
//


std::vector<std::string> peer_discovery(const std::string& base_url, const Torrent& torrent) {
  size_t last_index = base_url.find_last_of("/");
// getting the domain and entry point, as cpp-httplib requires client to be made out of just domain
  std::string domain = base_url.substr(0, last_index);
  std::string entry_point = base_url.substr(last_index,base_url.size());
  std::cout<<"Entry: "<<entry_point<<std::endl;
    httplib::Client client(domain);
    client.set_follow_location(true);  // Follow redirects, if any
                                       //
                                       //  // Define custom headers
    httplib::Headers headers = {
        {"User-Agent", "Mozilla/5.0"},  // Mimic a browser to avoid server rejection
        {"Accept", "*/*"},              // Accept any content type
        {"Connection", "keep-alive"}    // Keep the connection open if possible
    };

    // Create a string to hold the query parameters
    std::string query_params = entry_point+"?info_hash=" + urlEncode(torrent.info_hash) +
                               "&peer_id=" + torrent.peer_id +
                               "&port=" + std::to_string(torrent.port) +
                               "&uploaded=" + std::to_string(torrent.uploaded) +
                               "&downloaded=" + std::to_string(torrent.downloaded) +
                               "&left=" + std::to_string(torrent.left) +
                               "&compact=" + (torrent.compact ? "1" : "0");

    // send the get Request
    auto res = client.Get(query_params.c_str(), headers);

    std::vector<std::string> peers;  
  
    // Check the response
    if (res && res->status == 200) {
        //std::cout << "Response: " << res->body << std::endl;  // Print the response body
        // Parse the response body into a JSON object (assuming the response is valid JSON)
        try {
          // start index for decoding
          size_t start_index = 0;
          json decoded_response =  decode_bencode(res->body,start_index);    
          std::cout << "Interval: " << decoded_response["interval"];
          //printing peers size
          std::cout << "Peer size: " << decoded_response["peers"].size() << std::endl;

          std::string peer_string = decoded_response["peers"];
          
          for(size_t i = 0; i < peer_string.size(); i += 6){
          int ip1 = static_cast<unsigned char>(peer_string[i]);
          int ip2 = static_cast<unsigned char>(peer_string[i+1]);
          int ip3 = static_cast<unsigned char>(peer_string[i+2]);
          int ip4 = static_cast<unsigned char>(peer_string[i+3]);

           //A uint16_t can hold values from 0 to 65535 (which is the range of valid port numbers)
          uint16_t port  = (static_cast<uint16_t>(static_cast<unsigned char>(peer_string[i+4])) << 8) | 
            static_cast<uint16_t>(static_cast<unsigned char>(peer_string[i+5]));
             // Convert each segment of the IP address to a string
          std::string address = std::to_string(ip1) + "." + std::to_string(ip2) + "." +
                          std::to_string(ip3) + "." + std::to_string(ip4) + ":" + 
                          std::to_string(port);
          peers.push_back(address);
        

          }


          // jsonResponse = json::parse(res->body);
        } catch (json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Request failed with status: " << (res ? std::to_string(res->status) : "no response") << "\n";  // Print error status
    }

  return peers;  // Return the JSON response object
}

// converting hexadecimal representation to bytes
std::vector<uint8_t> hex_to_bytes(const std::string hex) {
  std::vector<uint8_t> bytes;

  for (size_t i =0; i < hex.size(); i+=2){
    // stoi converts the hexadecimal value to its integer value, which is then casted to byte value
    // we use 16 in stoi to tell it that the value is hex. so the conversion will be hex to integer
    uint8_t byte = static_cast<uint8_t>(std::stoi(hex.substr(i,2), nullptr, 16));
    bytes.push_back(byte);
  }
  return bytes;
}

// function for handshake
std::string create_handshake(const std::string& info_hash, const std::string& peer_id){
    
  std::string handshake;
  // length of the protocol
  handshake += static_cast<char>(19);
  handshake += "BitTorrent protocol";
  // rerserve 8 bytes
  uint8_t reserved[8] = {0,0,0,0,0,0,0,0};
  // reinterpret cast tells compiler to treat the uint8_t arrary as a raw sequence of bytes
  handshake.append(reinterpret_cast<const char*>(reserved), sizeof(reserved));
  handshake += info_hash;
  handshake += peer_id;

  return handshake;
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
      std::vector<std::string> peers = peer_discovery(base_url,torrent);
      for (std::string address: peers){
        std::cout << address << std::endl;
      }

      // handshake
      std::string handshake = create_handshake(info_hash, "00112233445566778899");
      std::cout << "Handshake message: " << handshake << std::endl;
    }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
