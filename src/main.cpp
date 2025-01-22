#include<iostream>
#include<string>
#include<fstream>
#include<iomanip>
#include<cmath>
#include <sstream>
#include<random>
#include<mutex>
#include<queue>
#include <condition_variable>
#include<thread>
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

// mutex for logging
std::mutex log_mutex;

// thread-safe counter
std::atomic<int> total_downloaded_pieces = 0;

// Mutex for file access
std::mutex file_mutex;

// piece queue for processing the pieces in threads

class PieceQueue {
  private:
    std::queue<PieceInfo> pieces;
    // to manage access to the pieces
    std::mutex mtx;
    std::condition_variable cv;
    // stop indicate if the queue has been filled, 
    // and no more piece should be added to queue
    bool stop = false;
  public:
    // add the piece to the queue
    void push(const PieceInfo& piece){
      std::unique_lock<std::mutex> lock(mtx);
      pieces.push(piece);
      // notify a waiting thread
      cv.notify_one();
    }

    std::optional<PieceInfo> pop(){
      std::unique_lock<std::mutex> lock(mtx);
      // wait until either queue is not empty or, stop flag is set to true
      cv.wait(lock, [this](){ return !pieces.empty() || stop;});
      // it means all the pieces are popped
      if (pieces.empty() && stop) {
        // signal no more pieces to process
            return std::nullopt;
        }
      PieceInfo piece = pieces.front();
      pieces.pop();
      return piece;
    }

    void finish() {
      std::unique_lock<std::mutex> lock(mtx);
      stop = true;
      cv.notify_all();
    }
};

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


void handle_peer(const Peer& peer, const TorrentFile& torrent, PieceQueue& piece_queue, const std::string& output_file_name){
  try{
      // to create socket and connection with the peer
      SocketManager socketManager;
      socketManager.connectToServer(peer.ip, peer.port);
      
      // sending handahake
      HandshakeHandler handshake_handler(torrent.info_hash, torrent.peer_id);
      handshake_handler.send_handshake(socketManager.getClientSocket());
      
      // read the peer message from server and handle Bitfield
      PeerMessageHandler peerHandler = PeerMessageHandler(socketManager.getClientSocket());
      PeerMessage message = peerHandler.read_peer_messages();
      
      if(message.id == 5){
        peerHandler.handle_bit_field(message);
      }

      //send interested to the server
      peerHandler.send_interested();

      // receive unchoke message
      message = peerHandler.read_peer_messages();  
      if(message.id==1){
        //std::cout << "Unchoked" << std::endl;
        peerHandler.set_choked(false);
      }

      // process the piece queue 
      while (true){
        auto optional_piece = piece_queue.pop();

        if(!optional_piece.has_value()){
          break;
        }

        PieceInfo piece = optional_piece.value();

        // check if the peer has the piece
        if (!peerHandler.has_piece(piece.piece_index)) {
                std::cerr << "Peer does not have the requested piece: " << piece.piece_index << std::endl;

                // add the piece back the the queue
                piece_queue.push(piece);
                continue;
        }

        // initiate the download of piece and save to file
        auto piece_data = PieceDownloader(peerHandler).handle_download_piece(
            piece.piece_index, piece.piece_length, piece.piece_hash
            );

        if(piece_data){

          {
          std::lock_guard<std::mutex> lock(file_mutex);
          save_piece_to_file(output_file_name, piece.piece_index, *piece_data, piece.piece_length);
         // std::cout << "Piece " << piece.piece_index << " downloaded and saved." << std::endl;
          }

          // update and log progress
          //// Calculate the total number of pieces
          int total_pieces = static_cast<int>(std::ceil(static_cast<double>(torrent.total_length) / torrent.piece_length));
          int completed_pieces = ++total_downloaded_pieces;
          double percentage = (static_cast<double>(completed_pieces) / total_pieces) * 100.0;
          {
            std::lock_guard<std::mutex> log_lock(log_mutex);
             std::cout << "Piece " << piece.piece_index << " downloaded from peer (IP: " << peer.ip
                              << "). Progress: " << std::fixed << std::setprecision(2) << percentage << "%\n";
          }

        }
        else {
          std::cerr << "Failed to download piece " << piece.piece_index << " from peer (IP: " << peer.ip
                          << ", Port: " << peer.port << ")" << std::endl;
        }
      }

  }
  catch(const std::exception& e){
    std::cerr << "Error with peer (IP: " << peer.ip << ", Port: " << peer.port << "): " << e.what() << std::endl;
  }

}

int main(int argc, char* argv[]){
  if(argc>1){
    
    // read rorrent file
  try{
      std::string file_name = argv[1];
      json decoded_value = parse_torrent_file(file_name);
 // std::string input_encoded_value = argv[1];
      // json decoded_value = decode_bencoded_value(input_encoded_value,0);
      //std::cout << "Tracker URL: "<< decoded_value["announce"].get<std::string>()<<std::endl;
      //std::cout << "Length: "<< decoded_value["info"]["length"].get<int>()<<std::endl;
      
      std::string base_url = decoded_value["announce"].get<std::string>();
      std::string encoded = encode_bencode(decoded_value["info"]);
      
      std::string info_hash = sha1_hash(encoded);
      //std::cout <<"Info Hash: "<<sha1_hash(encoded)<<std::endl;
      //std::cout <<"Piece Length: "<<decoded_value["info"]["piece length"].get<int>()<<std::endl;
      std::string piece_hash = to_hex(decoded_value["info"]["pieces"]);
      //std::cout <<"Piece Hashes: "<<piece_hash<<std::endl;

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
      
      // add the piece data to queue
      PieceQueue piece_queue;
      for (const auto& piece : piece_metadata) {
                piece_queue.push(piece);
      }
      piece_queue.finish(); // Signal no more pieces will be added
      
      //std::cout << "Piece Metadata added to queue" << std::endl;
      /*
      std::cout << "-------- Piece Metadata --------" << std::endl;
      for (auto piece : piece_metadata) {
          std::cout << "Piece Index: " << piece.piece_index
                    << ", Length: " << piece.piece_length
                    << ", Hash: " << to_hex(piece.piece_hash) << std::endl;
      }
      std::cout << "--------------------------------" << std::endl;
    */
      // peer peer_discovery
      //TorrentFile torrent(info_hash,peer_id,6681,0,0,decoded_value["info"]["piece length"],true);
      // discover the peers from where we can download the file using the url from torrent file
      PeerDiscovery peerDiscovery(torrent);
      std::vector<Peer> peers = peerDiscovery.discoverPeers();
      
      std::cout << "--------Discovered Peers------------" << std::endl;
      for (const Peer& peer : peers) {
          std::cout << "IP: " << peer.ip << ", Port: " << peer.port << std::endl;
      }
     

      std::vector<std::thread> threads;
      for(const Peer &peer: peers){
        threads.emplace_back(handle_peer, peer, std::cref(torrent), std::ref(piece_queue), std::cref(output_file_name));
      }

      // prorcess the threads
      for (std::thread& t: threads){
        if(t.joinable()){
          t.join();
        }
      }
      std::cout << "------------Download complete------------" << std::endl;      
    }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
