#include<iostream>
#include<string>
#include<fstream>
#include<iomanip>
#include<cmath>
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
#include "lib/TorrentParser/TorrentParser.hpp"
#include "lib/SocketManager.hpp"
#include "lib/PeerDiscovery.hpp"
#include "lib/HandshakeHandler.hpp"
#pragma comment(lib, "ws2_32.lib")
#include "common.hpp"




int main(int argc, char* argv[]){
  if(argc>1){
    
    // read rorrent file
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

      // TorentParser checker
      TorrentParser torrent_parser;
      TorrentFile torrent = torrent_parser.parse(file_name);

      std::cout << "----------Meta Data----------" << std::endl;
      std::cout << "Tracker URL: "<< decoded_value["announce"].get<std::string>()<<std::endl;


 // Extract file name from torrent metadata
      std::string output_file_name = decoded_value["info"]["name"].get<std::string>();
      std::cout << "Output File Name: " << output_file_name << std::endl;
      // peer peer_discovery
      //TorrentFile torrent(info_hash,peer_id,6681,0,0,decoded_value["info"]["piece length"],true);
      // discover the peers from where we can download the file using the url from torrent file
      PeerDiscovery peerDiscovery(torrent);
      std::vector<Peer> peers = peerDiscovery.discoverPeers();
      
        std::cout << "Discovered Peers:" << std::endl;
        for (const Peer& peer : peers) {
            std::cout << "IP: " << peer.ip << ", Port: " << peer.port << std::endl;
        }
      
      // initialize tcp server
      
      SocketManager socketManager;
      socketManager.connectToServer("165.232.41.73", 51556);
      SOCKET client_socket = socketManager.getClientSocket();

      
      // initialize handshake handler
      HandshakeHandler handshake_handler(info_hash, peer_id);
     // std::string handshake = handshake_handler.create_handshake();
     // handshake will be created and sent from send_handshake 
      auto handshake_received = handshake_handler.send_handshake(client_socket);
      //st165.232.41.73:51556d::cout << "Handshake message: " << handshake << std::endl;
     
      
        
      std::vector<uint8_t> file_data = handle_download_pieces(client_socket,
          torrent.total_length,
          torrent.piece_length,
          piece_hash);

      if(file_data.empty()){
        std::cerr << "Failed to download file" << std::endl;
        return -1;
      }

      // save the file
      std::ofstream output_file(output_file_name, std::ios::binary);
      if(!output_file){
        std::cerr << "Failed to open file for writing: " << output_file_name << std::endl;
      }

      output_file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());

      output_file.close();

      std::cout << "File saved successfully as " << output_file_name << std::endl;


    }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
