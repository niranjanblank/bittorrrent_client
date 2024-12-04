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


// Function for url encoding
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

// initialize socket
SOCKET initialize_socket(const std::string& peer_ip, unsigned short peer_port){
 WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2,2), &wsaData);
  
  if(result != 0){
    std::cerr << "WSAStartup failed: " << result << std::endl;
  }

  // socket for client
  SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);

  // checking socket
  if (client_socket == INVALID_SOCKET) {
    std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return INVALID_SOCKET;
  }


  // define the server address 
  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(peer_port);

  // link to server ip
  if (InetPton(AF_INET, peer_ip.c_str(), &server_address.sin_addr.s_addr) <= 0) {
        std::cerr << "Invalid IP address: " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return INVALID_SOCKET;
    }

  // connect to the server
  if(connect(client_socket, (sockaddr*) &server_address, sizeof(server_address))==0){
    std::cout << "Client connected to server" << std::endl;
  }
  else{
    std::cerr << "Client connection failed" << std::endl;
    WSACleanup();
    return INVALID_SOCKET;
  }

  return client_socket;

}

// socket cleanup
void cleanup_socket(SOCKET client_socket){
  closesocket(client_socket);
  WSACleanup();
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
  handshake.append(reinterpret_cast<const char *>(reserved), sizeof(reserved));
  // Convert the hex-encoded info_hash to raw bytes
  std::vector<uint8_t> info_hash_bytes = hex_to_bytes(info_hash);
  // Append the info_hash (20 bytes)
  handshake.append(info_hash_bytes.begin(), info_hash_bytes.end());
  handshake += peer_id;

  return handshake;
}



HandshakeMessage parse_handshake_message(const char* received_data, int bytes_received){
  std::cout << "Data received: " << received_data << std::endl;
  std::cout << "Bytes received: " << bytes_received << std::endl;
  
  HandshakeMessage handshake_message;
  // parsing the handshake
  // first byte is length of protocol
  handshake_message.protocol_length = static_cast<uint8_t>(received_data[0]);
  // 19 bytes protocol identifier
  handshake_message.protocol_identifier = std::string(received_data+1, handshake_message.protocol_length);
  // 8 reserved_bytes
  handshake_message.reserved_bytes = std::vector<uint8_t>(received_data+1+handshake_message.protocol_length, 
      received_data+1+handshake_message.protocol_length+8);
  // 20 bytes info hash
  handshake_message.info_hash = std::vector<uint8_t>(received_data+1+handshake_message.protocol_length+8, 
      received_data+1+handshake_message.protocol_length+8+20);
  // 20 bytes peer id
  handshake_message.peer_id = std::string(received_data + 1 + handshake_message.protocol_length + 8 + 20, 
                                    received_data + 1 + handshake_message.protocol_length + 8 + 20 + 20);
  return handshake_message;
}

std::optional<HandshakeMessage> send_handshake(const std::string& handshake, SOCKET client_socket){
  std::cout << "Handshake message: " << handshake << std::endl;

  // send handshake to server
  size_t byte_count = send(client_socket, handshake.c_str(), handshake.size(),0);
 if(byte_count == SOCKET_ERROR){
    std::cerr << "Server sent error :" << WSAGetLastError() << std::endl;
    return std::nullopt;
  }
  else{
    std::cout << "Handshake sent: " << byte_count << " bytes" << std::endl;
  }

  // receive data from server
  char handshake_buffer[68];

  int bytes_received = recv(client_socket, handshake_buffer, sizeof(handshake_buffer),0);

  
   if (bytes_received <= 0) {
        std::cerr << "Failed to receive handshake response: " << WSAGetLastError() << std::endl;
        return std::nullopt; // Indicate failure
    }
 if (bytes_received < 68) {
        std::cerr << "Failed to receive complete handshake: " << bytes_received << " bytes received." << std::endl;
        return std::nullopt;
    }
  // parsed received handshake
  HandshakeMessage parsed_handshake = parse_handshake_message(handshake_buffer, bytes_received); 
  std::cout << "Protocol Length: " << static_cast<int>(parsed_handshake.protocol_length) << std::endl;
  std::cout << "Protocol identifier: " << parsed_handshake.protocol_identifier << std::endl;
  std::cout << "PeerID: " << to_hex(parsed_handshake.peer_id) << std::endl;

 return parsed_handshake;

}

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

bool download_piece(SOCKET client_socket, uint32_t piece_index, uint32_t piece_length){
    const uint32_t block_size = 16*1024;
    uint32_t offset = 0;


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
         return false;
     // Skip andwait for the next message
      }     

    if(message.id == 7){
        // piece message
        uint32_t received_index = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()));
        uint32_t received_begin = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()+4));

        std::vector<uint8_t> block(message.payload.begin()+8, message.payload.end());

       if (received_index != piece_index || received_begin != offset) {
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

  return true;
}

void handle_peer_messages(SOCKET client_socket, int32_t piece_index, uint32_t piece_length){
  std::cout << "Piece Length" << piece_length << std::endl;

  while(true){
   PeerMessage message = read_peer_messages(client_socket);

   //if we encounter keep alive message, we skip processing
   if (message.length == 0) {
        std::cout << "Received keep-alive message." << std::endl;
        continue; // Skip processing for keep-alive
    } 

    // process peer messages 
    std::cout << "Processing messaage ID:" << static_cast<int>(message.id) << std::endl;
    switch(message.id){
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
        if (!download_piece(client_socket, piece_index, piece_length)) {
                    std::cerr << "Error during piece download. Terminating peer message handling." << std::endl;
                    return; // Exit the loop if download_piece fails
                }
      default:
        break;
      }

      }
}
