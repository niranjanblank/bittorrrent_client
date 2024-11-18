#include<iostream>
#include<string>
#include<fstream>
#include<iomanip>
#include <sstream>
#include<random>
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "lib/bencode/bencode.hpp"
#include "lib/utils.hpp"
#include "lib/httplib.h"
#include<winsock2.h>
#include <ws2tcpip.h>// Required for InetPton()                       
#include "lib/structures.hpp"
#include "lib/networking.hpp"
#pragma comment(lib, "ws2_32.lib")
using json = nlohmann::json;



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

      std::string peer_id = generate_peer_id();
      std::cout <<"Peer ID: "<<peer_id<<std::endl;
      // peer peer_discovery
      Torrent torrent(info_hash,peer_id,6681,0,0,decoded_value["info"]["piece length"],true);
      std::vector<std::string> peers = peer_discovery(base_url,torrent);
      for (std::string address: peers){
        std::cout << address << std::endl;
      }
      
      // initialize tcp server
      SOCKET client_socket = initialize_socket("165.232.41.73", 51556);
      if (client_socket == INVALID_SOCKET) {
        return -1;
      }

      // handshake
      std::string handshake = create_handshake(info_hash, peer_id);
      
      auto handshake_received = send_handshake(handshake, client_socket);
      //st165.232.41.73:51556d::cout << "Handshake message: " << handshake << std::endl;
      //
      if (!handshake_received){
        std::cerr << "Handshake failed" << std::endl;
        cleanup_socket(client_socket);
        return -1;
      }
    //}
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
