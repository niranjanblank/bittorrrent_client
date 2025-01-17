#pragma once
#include "lib/sha1.hpp"
#include<string>
#include<sstream>
#include<iomanip>

// Third-Party Libraries
#include "lib/httplib.h"
#include "lib/bencode/bencode.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "lib/utils.hpp"
#include "lib/structures.hpp"
// get sha1 from the data
std::string sha1_hash(const std::string& data) {
    SHA1 sha1;
    sha1.update(data);
    std::string hash = sha1.final();
    return hash;
}

// Convert raw binary data to hexadecimal format for debugging
std::string to_hex(const std::string& input) {
    std::ostringstream hex_stream;
    for (unsigned char c: input) {
        hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return hex_stream.str();
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

std::string bytes_to_hash(const std::vector<uint8_t>& bytes){
  std::string data(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return sha1_hash(data);
}

std::string generate_peer_id(){
  const std::string prefix =  "-MY1000-"; // 8 bytes
  const int peer_id_length = 20;
    
  // random number generation
  std::random_device rd;

  std::uniform_int_distribution<> dist(0,35); // 0-9 and a-z for alpha numeric
  // initialize random number generator gen with random seed rd
  std::mt19937 gen(rd());
  std::string peer_id = prefix;

  // generate the remaining 12 random alphanumeric character
  for(size_t i = prefix.size(); i < peer_id_length; i++){
    int random_value = dist(gen);
    if(random_value < 10){
      peer_id += '0'+random_value;
    }
    else{
      peer_id += 'a' + (random_value - 10);
    }
  }

  return peer_id;
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


// read exact number of bytes from socket
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


// generate the number of pieces, their length and their hash
 
std::vector<PieceInfo> generate_piece_metadata(int total_length, int piece_length, const std::string& piece_hashes) {
    const size_t hash_length = 20; // Each SHA-1 hash is 20 bytes
    if (piece_hashes.size() % hash_length != 0) {
        throw std::runtime_error("Invalid piece_hash size. Not a multiple of 20.");
    }

    std::vector<PieceInfo> metadata;
    int total_pieces = static_cast<int>(std::ceil(static_cast<double>(total_length) / piece_length));

    //size_t total_pieces = piece_hashes.size() / hash_length;


    for (size_t i = 0; i < total_pieces; ++i) {
        uint32_t current_piece_length = (i == total_pieces - 1) 
            ? total_length % piece_length 
            : piece_length;

        //if (current_piece_length == 0) current_piece_length = piece_length;
            std::string hash = piece_hashes.substr(i * hash_length, hash_length);
            PieceInfo piece_info = {static_cast<uint32_t>(i), static_cast<uint32_t>(current_piece_length), hash};

          metadata.push_back(piece_info);    
    }

    return metadata;
}

