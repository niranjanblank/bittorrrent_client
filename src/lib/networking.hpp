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

// Third-Party Libraries
#include "lib/httplib.h"
#include "lib/bencode/bencode.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "lib/utils.hpp"

// Project-Specific Headers
#include "lib/structures.hpp"
#include "lib/TorrentParser/TorrentParser.hpp"


// function to read exact number of bytes from the tcp stream
int read_exact_bytes(SOCKET socket, char* buffer, int bytes_to_read){
  int total_bytes_read = 0;
// we keep on reading from the stream until we have rearched our required bytes
  while(total_bytes_read < bytes_to_read){
    int bytes_read = recv(socket,buffer+total_bytes_read,bytes_to_read - total_bytes_read,0);

    if (bytes_read <=0){
      // connection closed or errorr occured
      throw std::runtime_error("Connection closed or errorr during recv");
    }

    total_bytes_read += bytes_read;
  }

  return total_bytes_read;
}


PeerMessage read_peer_messages(SOCKET client_socket){
 PeerMessage message; 
// read the length
  char length_buffer[4];
  read_exact_bytes(client_socket, length_buffer, 4);
  // nhtohl converts a 32-bit integerr from network byte order to host byte order
  message.length = ntohl(*reinterpret_cast<uint32_t*>(length_buffer));

  // checking if the mssagte is keep alive message
  if (message.length == 0) {
        std::cout << "Received keep-alive message." << std::endl;
        message.id = 0; 
        return message; // Continue without processing further
    }

  // read message id byte
  char id_buffer[1];
  read_exact_bytes(client_socket, id_buffer, 1);
  message.id = id_buffer[0];

  // read payload
  // payload size = total length - size of message id
  message.payload.resize(message.length - 1); // exclude the message ID   
// message.payload.data() returns pointer to the first element of vector, read_exact_bytes requires char pointer,
// so reinterpret_casting it to char * 
  read_exact_bytes(client_socket, reinterpret_cast<char*>(message.payload.data()), message.payload.size());

  /*
  std::cout << "----Peer Messages-----"  << std::endl;
  std::cout << "Length : " << message.length << std::endl;
  std::cout << "Message ID: " << static_cast<int>(message.id) << std::endl;
  */

  return message;

}

// handling each message
void handle_bit_field(const PeerMessage& message){
   std::cout << "Bitfield message received" << std::endl;
  // ignore bitfield for now
  /*
    for(size_t i=0; i < message.payload.size(); i++){
      std::bitset<8> bits(message.payload[i]);
      std::cout << "Byte " << i << ":" << bits << std::endl; 
    }
  */
  // process the bits to determine piece availability
  //for (int bit = 7; bit >=0; bit --)
}

void send_interested(SOCKET client_socket){
  uint8_t id = 2; // for interested peer message 
  uint32_t length = htonl(1);
    // 5 bytes: 4 bytes for length and 1 for message id 
  char buffer[5];
  memcpy(buffer, &length,4);
  memcpy(buffer+4, &id,1);

  send(client_socket, buffer, sizeof(buffer),0);
  std::cout << "Interested message sent" << std::endl;

}

void send_request_message(SOCKET client_socket, uint32_t piece_index, uint32_t offset, uint32_t length){
  uint32_t length_prefix = htonl(13); // message_id:1 byte, payload: 12 bytes 
  uint8_t message_id = 6; // 6 is for request

  char buffer[17];

  memcpy(buffer, &length_prefix, 4);
  memcpy(buffer+4, &message_id,1);

  uint32_t network_piece_index = htonl(piece_index);
  uint32_t network_offset = htonl(offset);
  uint32_t network_length = htonl(length);
  memcpy(buffer+5, &network_piece_index,4);
  memcpy(buffer+9, &network_offset,4);
  memcpy(buffer+13, &network_length,4);

  send(client_socket, buffer, sizeof(buffer),0);
  std::cout << "Request Message sent" << std::endl;

}

std::optional<std::vector<uint8_t>> download_piece(SOCKET client_socket, uint32_t piece_index, uint32_t piece_length){
    const uint32_t block_size = 16*1024;
    uint32_t offset = 0;
    
    std::cout <<"---------Piece Info----------" << std::endl;
    std::cout <<"Piece Index: " << piece_index << std::endl;
    std::cout <<"Piece Length: " << piece_length << std::endl;
    std::cout <<"-----------------------------" << std::endl;

    // to store the blocks downloaded
    std::vector<uint8_t> piece_data(piece_length);

    while(offset < piece_length){
      // current block size 
      uint32_t current_block_size = std::min(block_size, piece_length - offset);
    
    // send request to the server
      send_request_message(client_socket, piece_index, offset, current_block_size);

      PeerMessage message = read_peer_messages(client_socket);



        // Handle keep-alive messages
        if (message.length == 0) {
            std::cout << "Keep-alive message received. Skipping..." << std::endl;
            continue; // Ignore and wait for the next message
        }

        std::cout << "Message ID received: " << static_cast<int>(message.id) << std::endl;

     if (message.id < 0 || message.id > 9) {
         std::cerr << "Unexpected message ID: " << static_cast<int>(message.id) << std::endl;

         // log the invalid message
        std::cerr << "Message length: " << message.length << std::endl;
        std::cerr << "Message payload: ";

        for (const auto& byte: message.payload){
          std::cerr << std::hex << static_cast<int>(byte)<<std::endl;
        }
         return std::nullopt;
      }     

      if(message.id == 7){
          // piece message
          uint32_t received_index = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()));
          uint32_t received_begin = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()+4));

          std::vector<uint8_t> block(message.payload.begin()+8, message.payload.end());

           if (received_index == piece_index && received_begin == offset) {
              // Store the block in the piece_data buffer
              std::copy(block.begin(), block.end(), piece_data.begin() + offset);

              // Update the offset to move to the next block
              offset += block.size();

              std::cout << "-----Block Received---" << std::endl;
              std::cout << "Received Index: " << received_index << std::endl;
              std::cout << "Received Begin: " << received_begin << std::endl;
              std::cout << "Offset : " << offset << std::endl;
              std::cout << "Block Size: " << block.size() << std::endl;
              std::cout << "----------------------" << std::endl;
          } else {
              std::cerr << "Invalid block received. Expected index: " << piece_index
                        << ", offset: " << offset
                        << ", but got index: " << received_index
                        << ", offset: " << received_begin << std::endl;
              continue; // Reattempt to download the current block
          }    
        
    }
        
  //  std::cout << "Length Received: " << message.length << std::endl;
   // std::cout << "Message Id Received: " << static_cast<int>(message.id) << std::endl;

  }

  return piece_data;
}



std::vector<uint8_t> handle_download_pieces(SOCKET client_socket, int total_length, int piece_length, const std::string& piece_hash) {

    // buffer to hold the entire data
    std::vector<uint8_t> file_data(total_length, 0);
    size_t accumulated_size = 0; // Tracks the current write position in the file buffer

    // get total pieces
    int total_pieces = static_cast<int>(std::ceil(static_cast<double>(total_length) / piece_length));
    std::cout << "Total Pieces: " << total_pieces << std::endl;
    std::cout << "Piece Length: " << piece_length << std::endl;

    int bytes_to_send = piece_length;

    std::optional<std::vector<uint8_t>> piece_data;
    // std::string piece_data_string;
    while (true) {
        PeerMessage message = read_peer_messages(client_socket);

        // if we encounter keep alive message, we skip processing
        if (message.length == 0) {
            std::cout << "Received keep-alive message." << std::endl;
            continue; // Skip processing for keep-alive
        }

        // process peer messages
        std::cout << "Processing message ID: " << static_cast<int>(message.id) << std::endl;

        switch (message.id) {
            case 0: // Choke message
                std::cout << "Received choke message. Cannot request pieces until unchoked." << std::endl;
                break;

            case 5: 
                // bit field message
                handle_bit_field(message);
                send_interested(client_socket);
                break;

            case 1:
                // unchoke message;
                std::cout << "Received unchoke message" << std::endl;

                // download all the pieces
                for (int i = 0; i < total_pieces; i++) {
                    int bytes_sent = (i + 1) * piece_length;

                    if (bytes_sent > total_length) {
                        bytes_to_send = total_length - (i * piece_length);
                    }
                    std::cout << "Index: " << i << ", Bytes: " << bytes_to_send << std::endl;

                    // download piece at index
                    piece_data = download_piece(client_socket, static_cast<uint32_t>(i), static_cast<uint32_t>(bytes_to_send));
                    if (!piece_data) {
                        std::cerr << "Error downloading piece data at index " << i << ". Terminating peer message handling." << std::endl;
                        return {};
                        // return std::nullopt;
                    }

                    // validate the hash
                    std::string computed_hash = bytes_to_hash(*piece_data);
                    std::string expected_hash = piece_hash.substr(i * 40, 40);
                    if (computed_hash == expected_hash) {
                        std::cout << "Hash Validated" << std::endl;
                    } else {
                        std::cerr << "Invalid Hash" << std::endl;
                        return {};
                    }

                    // piece data downloaded
                    std::cout << "Piece downloaded successfully, size: " << piece_data->size() << " bytes." << std::endl;

                    // copy the downloaded piece to the buffer
                    size_t copy_length = piece_data->size();
                    if (accumulated_size + copy_length > file_data.size()) {
                        std::cerr << "Error: Piece data exceeds file buffer size." << std::endl;
                        return {}; // Return empty file data on failure
                    }
                    std::copy(piece_data->begin(), piece_data->end(), file_data.begin() + accumulated_size);
                    accumulated_size += copy_length;
                    std::cout << "Piece " << i << " added to the buffer" << std::endl;
                }
                return file_data;

            default:
                std::cerr << "Unexpected message ID: " << static_cast<int>(message.id) << std::endl;
                break;
        }
    }
}

