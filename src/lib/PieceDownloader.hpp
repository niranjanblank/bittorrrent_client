
#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bitset>
#include <chrono>
#include "lib/structures.hpp"
#include "lib/PeerMessageHandler.hpp"

class PieceDownloader {
public:
    PieceDownloader(PeerMessageHandler& message_handler)
        : message_handler_(message_handler) {}

    // send request message to server for downloading a specific block of a piece
    void send_request_message(uint32_t piece_index, uint32_t offset, uint32_t length) {
        uint32_t length_prefix = htonl(13); // message_id:1 byte, payload: 12 bytes
        uint8_t message_id = 6; // 6 is for request

        char buffer[17];

        memcpy(buffer, &length_prefix, 4);
        memcpy(buffer + 4, &message_id, 1);

        uint32_t network_piece_index = htonl(piece_index);
        uint32_t network_offset = htonl(offset);
        uint32_t network_length = htonl(length);
        memcpy(buffer + 5, &network_piece_index, 4);
        memcpy(buffer + 9, &network_offset, 4);
        memcpy(buffer + 13, &network_length, 4);

        send(message_handler_.get_socket(), buffer, sizeof(buffer), 0);
        //std::cout << "Request Message sent" << std::endl;
    }

    // download a single piece from the server
    std::optional<std::vector<uint8_t>> download_piece(uint32_t piece_index, uint32_t piece_length) {
        const uint32_t block_size = 16 * 1024;
        uint32_t offset = 0;
        
        /*
        std::cout << "---------Piece Info----------" << std::endl;
        std::cout << "Piece Index: " << piece_index << std::endl;
        std::cout << "Piece Length: " << piece_length << std::endl;
        std::cout << "-----------------------------" << std::endl;
        */
        // to store the blocks downloaded
        std::vector<uint8_t> piece_data(piece_length);

        int keep_alive_retries = 0;
        int retries = 0;

        while (offset < piece_length) {
            if(retries>50 or keep_alive_retries >1){
              return std::nullopt;
            }

            // current block size
            uint32_t current_block_size = std::min(block_size, piece_length - offset);

            // send request to the server
            send_request_message(piece_index, offset, current_block_size);

            PeerMessage message = message_handler_.read_peer_messages();

            // Handle keep-alive messages
            if (message.length == 0) {
                std::cout << "Keep-alive message received for piece "<< piece_index<<" Skipping..." << std::endl;
              
               // keep_alive_retries++;
                continue; // Ignore and wait for the next message
            }

            //std::cout << "Message ID received: " << static_cast<int>(message.id) << std::endl;

            if (message.id < 0 || message.id > 9) {
                std::cerr << "Unexpected message ID: " << static_cast<int>(message.id) << std::endl;

                // log the invalid message
                std::cerr << "Message length: " << message.length << std::endl;
                std::cerr << "Message payload: ";

                for (const auto& byte : message.payload) {
                    std::cerr << std::hex << static_cast<int>(byte) << std::endl;
                }
                return std::nullopt;
            }

            if (message.id == 0){
              std::cerr << "Choke Message Received: Severing connection to the Peer" << std::endl;
              return std::nullopt;
            }

            if (message.id == 7) {
                // piece message
                uint32_t received_index = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()));
                uint32_t received_begin = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data() + 4));

                std::vector<uint8_t> block(message.payload.begin() + 8, message.payload.end());

                if (received_index == piece_index && received_begin == offset) {
                    // Store the block in the piece_data buffer
                    std::copy(block.begin(), block.end(), piece_data.begin() + offset);

                    // Update the offset to move to the next block
                    offset += block.size();
                    
                   /* 
                    std::cout << "-----Block Received---" << std::endl;
                    std::cout << "Received Index: " << received_index << std::endl;
                    std::cout << "Received Begin: " << received_begin << std::endl;
                    std::cout << "Offset : " << offset << std::endl;
                    std::cout << "Block Size: " << block.size() << std::endl;
                    std::cout << "----------------------" << std::endl;
                    */
                    
                } else {
                 /* 
                    std::cout << "-----Invalid Block Received---" << std::endl;
                    std::cout << "Received Index: " << received_index << std::endl;
                    std::cout << "Received Begin: " << received_begin << std::endl;
                    //std::cout << "Offset : " << offset << std::endl;
                    std::cout << "Block Size: " << block.size() << std::endl;
                    std::cout << "Expected Index:" << piece_index << std::endl;
                    std::cout << "Expected Begin" << offset << std::endl; 
                    std::cout << "----------------------" << std::endl;
                   */     
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        /*
                    std::cerr << "Invalid block received. Expected index: " << piece_index
                              << ", offset: " << offset
                              << ", but got index: " << received_index
                              << ", offset: " << received_begin << std::endl;
                              */
                           
                   // std::cerr << "Invalid block received. Reattempting block download" << std::endl;
                  // retries++;
                    continue; // Reattempt to download the current block
                }
            }
        }

        return piece_data;
    }

  std::optional<std::vector<uint8_t>> handle_download_piece(uint32_t piece_index, 
      uint32_t piece_length, const std::string& piece_hash){
    std::optional<std::vector<uint8_t>> piece_data;

    if(!message_handler_.get_choked()){
    // std::cout << "Attempting Download: Piece " << piece_index << "Size: " <<piece_length << std::endl;
      // download the piece
      piece_data = download_piece(piece_index, piece_length);

      if (!piece_data) {
              //std::cerr << "Error downloading piece data at index " << piece_index << ". Terminating peer message handling." << std::endl;
              return std::nullopt;    
      }

      std::string computed_hash = bytes_to_hash(*piece_data);
      //std::string expected_hash = piece_hash.substr(i * 40, 40);
      /*
      if (computed_hash == piece_hash) {
          //std::cout << "\nHash validated for " << "piece " << piece_index << std::endl;
      } else {
          std::cerr << "Invalid hash for " << "piece " << piece_index << std::endl;
          return std::nullopt;                    }
      */
      if ( computed_hash != piece_hash){
         std::cerr << "Invalid hash for " << "piece " << piece_index << std::endl;
          return std::nullopt;   
      }
       else {
       // std::cout << "Valid hash for piece " << piece_index << std::endl;
      }
      // piece data downloaded
      
      //std::cout << "piece downloaded successfully, size: " << piece_data->size() << " bytes." << std::endl;

      return piece_data;
    }

    return std::nullopt;
  }

 

private:
//    SOCKET client_socket_;
    PeerMessageHandler& message_handler_;
};

