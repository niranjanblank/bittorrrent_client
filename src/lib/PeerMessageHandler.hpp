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

#include "lib/utils.hpp"
#include "lib/structures.hpp"
class PeerMessageHandler{
  public:
    explicit PeerMessageHandler(SOCKET socket): client_socket(socket), is_choked(false) {}

    PeerMessage read_peer_messages(){
          PeerMessage message; 
        // read the length
          char length_buffer[4];
          read_exact_bytes(client_socket, length_buffer, 4);
          // nhtohl converts a 32-bit integerr from network byte order to host byte order
          message.length = ntohl(*reinterpret_cast<uint32_t*>(length_buffer));

          // checking if the mssagte is keep alive message
          if (message.length == 0) {
                std::cout << "Received keep-alive message." << std::endl;
                //message.id = 0; 
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
     // std::cout << "-----------Bitfield message received----------" << std::endl;
      // ignore bitfield for now
      // clear any existing piece availability information
      piece_availability_.clear();

      // process each byte in the payload
      for(size_t i=0; i < message.payload.size(); i++){
          // convert byte to 8 bits
          std::bitset<8> bits(message.payload[i]);
          
          //std::cout << "Byte " << i << ":" << bits << std::endl; 
          
          // add each bit to piece availability vector (MSB to LSB)
          for (int bit=7; bit>=0; bit--){
            piece_availability_.push_back(bits[bit]);
          }

      }
      
      /*
      std::cout << "Bitfield: ";
      for (size_t i = 0; i < piece_availability_.size(); ++i) {
          std::cout << (piece_availability_[i] ? '1' : '0');
      }
      std::cout << std::endl;
      std::cout << "Peer has the following pieces available: ";
      for (auto piece_index: get_available_pieces()){
          std::cout << piece_index << " "; 
      }
      std::cout << std::endl << "--------------------------------------" << std::endl;
      */
    }

  void send_interested(){
    uint8_t id = 2; // for interested peer message 
    uint32_t length = htonl(1);
      // 5 bytes: 4 bytes for length and 1 for message id 
    char buffer[5];
    memcpy(buffer, &length,4);
    memcpy(buffer+4, &id,1);

    send(client_socket, buffer, sizeof(buffer),0);
    //std::cout << "Interested message sent" << std::endl;

    }

  bool has_piece(size_t index) const{
    return index < piece_availability_.size() && piece_availability_[index];
  }

  // get index of all the available pieces
  std::vector<size_t> get_available_pieces() const{
    std::vector<size_t> available_pieces;
    for(size_t i=0; i < piece_availability_.size();i++){
      if (piece_availability_[i]){
        available_pieces.push_back(i);
      }
    }
    return available_pieces;
  }

  SOCKET get_socket() const {
    return client_socket;
  }

  void set_choked(bool val){
    is_choked = val;
  }

  bool get_choked(){
    return is_choked;
  }

  int get_total_available_pieces(){
    int count_of_ones = std::count(piece_availability_.begin(), piece_availability_.end(), true);
   // std::cout << "Number of 1s in the vector: " << count_of_ones << std::endl;

    return count_of_ones;
    // Print the result
   // std::cout << "Number of 1s in the vector: " << count_of_ones << std::endl;

  }


  private:
    SOCKET client_socket;
    std::vector<bool> piece_availability_;
    bool is_choked;
};
