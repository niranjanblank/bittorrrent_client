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
//#include "lib/networking.hpp"
#include "lib/TorrentParser/TorrentParser.hpp"
#include "lib/SocketManager.hpp"
#include "lib/PeerDiscovery.hpp"
#include "lib/HandshakeHandler.hpp"
#include "lib/PieceDownloader.hpp"
#include "lib/PeerMessageHandler.hpp"

#pragma comment(lib, "ws2_32.lib")
#include "common.hpp"


void save_piece_to_file(const std::string& file_path, uint32_t piece_index, const std::vector<uint8_t>& piece_data, uint32_t piece_length) {
    // Open the file in binary mode with read/write and append disabled
    std::ofstream file(file_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        // Create the file if it does not exist
        file.open(file_path, std::ios::binary | std::ios::out);
        if (!file) {
            throw std::runtime_error("Failed to open or create file: " + file_path);
        }
    }

    // Calculate the offset for the piece
    uint64_t offset = static_cast<uint64_t>(piece_index) * piece_length;

    // Seek to the correct offset
    file.seekp(offset);
    if (!file) {
        throw std::runtime_error("Failed to seek to offset " + std::to_string(offset) + " in file: " + file_path);
    }

    // Write the piece data
    file.write(reinterpret_cast<const char*>(piece_data.data()), piece_data.size());
    if (!file) {
        throw std::runtime_error("Failed to write piece data to file at offset " + std::to_string(offset));
    }

    file.close();
}

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
      
      // piece info of each piece
      std::vector<PieceInfo> piece_metadata;
      piece_metadata = generate_piece_metadata(torrent.total_length, torrent.piece_length, torrent.piece_hash);

    std::cout << "-------- Piece Metadata --------" << std::endl;
    for (auto piece : piece_metadata) {
        std::cout << "Piece Index: " << piece.piece_index
                  << ", Length: " << piece.piece_length
                  << ", Hash: " << to_hex(piece.piece_hash) << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;

      // peer peer_discovery
      //TorrentFile torrent(info_hash,peer_id,6681,0,0,decoded_value["info"]["piece length"],true);
      // discover the peers from where we can download the file using the url from torrent file
      PeerDiscovery peerDiscovery(torrent);
      std::vector<Peer> peers = peerDiscovery.discoverPeers();
      
        std::cout << "Discovered Peers:" << std::endl;
        for (const Peer& peer : peers) {
            std::cout << "IP: " << peer.ip << ", Port: " << peer.port << std::endl;
        }
     

      // create handlers fo each peer along with their sockets

      std::vector<std::unique_ptr<PeerMessageHandler>> peer_handlers;
      
      // maps peer index to available pieces
      std::unordered_map<int, std::vector<size_t>> peer_to_pieces;

      /*

      for (size_t i = 0; i < peers.size(); i++) {
        try{

  
        // creating peer handlers
        //if(i<5){
          //continue;
        //}
        std::cout << "Creating Peer Handlers: " << i << std::endl;
        SocketManager socketManager;
        socketManager.connectToServer(peers[i].ip, peers[i].port);
        SOCKET client_socket = socketManager.getClientSocket();

          // perform handshake
        // initialize handshake handler
        HandshakeHandler handshake_handler(info_hash, peer_id);
      // std::string handshake = handshake_handler.create_handshake();
      // handshake will be created and sent from send_handshake 
        auto handshake_received = handshake_handler.send_handshake(client_socket);

        // create peer handler
        auto handler = std::make_unique<PeerMessageHandler>(client_socket);

        // read and handle bitfields
        PeerMessage message = handler->read_peer_messages();
        if (message.id == 5) { // Bitfield message
                handler->handle_bit_field(message);
               // handler->send_interested();

                // Save available pieces for this peer
                peer_to_pieces[i] = handler->get_available_pieces();
            }

        peer_handlers.push_back(std::move(handler));
        std::cout << "Handshake and bitfield processing successful with peer " << peers[i].ip << std::endl;

      }
      catch (const std::exception& e){
 // Log the error for this peer and continue
          std::cerr << "Error with peer " << i << " (IP: " << peers[i].ip << ", Port: " << peers[i].port
                    << "): " << e.what() << std::endl;
          continue;
      }
      }
      // initialize tcp server
 
      
      if (peer_handlers.empty()) {
            std::cerr << "No valid peer connections established." << std::endl;
            return -1;
      }
    */
SocketManager socketManager;
SOCKET client_socket;

for(size_t i = 0; i<peers.size();i++){

try{
  socketManager.connectToServer(peers[i].ip, peers[i].port);
  client_socket = socketManager.getClientSocket();
      std::cout << "Successfully connected to peer " << i 
                  << " (IP: " << peers[i].ip 
                  << ", Port: " << peers[i].port << ")" << std::endl;
        break; // Exit the loop on successful connection

}
  catch (const std::exception &e){
  std::cerr << "Error with peer " << i << " (IP: " << peers[i].ip << ", Port: " << peers[i].port
                    << "): " << e.what() << std::endl;
  continue;
}

}



HandshakeHandler handshake_handler(info_hash, peer_id);
      // std::string handshake = handshake_handler.create_handshake();
      // handshake will be created and sent from send_handshake 
auto handshake_received = handshake_handler.send_handshake(client_socket);


PeerMessageHandler peerHandler = PeerMessageHandler(client_socket);
  PeerMessage message = peerHandler.read_peer_messages();
        if (message.id == 5) { // Bitfield message
                peerHandler.handle_bit_field(message);
               // handler->send_interested();

                // Save available pieces for this peer
                //peer_to_pieces[i] = handler->get_available_pieces();
            }


peerHandler.send_interested();
 message = peerHandler.read_peer_messages();  
 if(message.id==1){
   std::cout << "Unchoked" << std::endl;
   peerHandler.set_choked(false);
 }


 PieceDownloader pieceDownloader(peerHandler);

   for (auto piece : piece_metadata) {

  
   std::optional<std::vector<uint8_t>> piece_data = pieceDownloader.handle_download_piece(piece.piece_index, piece.piece_length, piece.piece_hash);
       
   std::cout << "Piece Index: " << piece.piece_index
                  << ", Length: " << piece.piece_length
                  << ", Hash: " << piece.piece_hash << std::endl;

          if (piece_data) {
              try {
                  save_piece_to_file(output_file_name, piece.piece_index, *piece_data, piece.piece_length);
                  std::cout << "Piece " << piece.piece_index << " saved to file." << std::endl;
              } catch (const std::exception& e) {
                  std::cerr << "Error saving piece " << piece.piece_index << ": " << e.what() << std::endl;
              }
            }
          else {
                std::cerr << "Failed to download piece " << piece.piece_index << std::endl;
            }
    }
    std::cout << "--------------------------------" << std::endl;

      // getting all the piece_index, along with their length and index
      
      /*
      // initialize handshake handler
      HandshakeHandler handshake_handler(info_hash, peer_id);
     // std::string handshake = handshake_handler.create_handshake();
     // handshake will be created and sent from send_handshake 
      auto handshake_received = handshake_handler.send_handshake(client_socket);
      //st165.232.41.73:51556d::cout << "Handshake message: " << handshake << std::endl;
    
      PeerMessageHandler message_handler(client_socket);
      PieceDownloader downloader(client_socket, message_handler);
      
        
      //std::vector<uint8_t> file_data = handle_download_pieces(client_socket,
        //  torrent.total_length,
          //torrent.piece_length,
          //piece_hash);


      std::vector<uint8_t> file_data = downloader.handle_download_pieces(
                client_socket, torrent.total_length, torrent.piece_length, piece_hash);
      
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
    */

    }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
