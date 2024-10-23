#include<iostream>
#include<string>
#include<fstream>
#include<iomanip>
#include <sstream>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;


// convert raw data to hex
std::string to_hex(const std::string& input){
  std::ostringstream hex_stream;
  for(unsigned char c: input){
    hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
  
 return hex_stream.str();
}

// string 
std::pair<json, size_t> decode_bencoded_string(const std::string& encoded_value, size_t index){
        size_t colon_index = encoded_value.find(":", index);
        if(colon_index != std::string::npos){
          
          int length_of_string = std::stoi(encoded_value.substr(index,colon_index-index));
          // extracted string from colon_index+1 to the end 
          std::string extracted_string = encoded_value.substr(colon_index+1, length_of_string);
          

   // Check if the string contains binary data (non-printable characters)
       //   if (std::any_of(extracted_string.begin(), extracted_string.end(),
         //                 [](unsigned char c) { return c < 32 || c > 126; })) {
              // Treat as binary and encode it to hex
          //    return {json(to_hex(extracted_string)), colon_index + 1 + length_of_string};
         // }
    
          // colon_index + 1 + length_of_string gives the ending of current bencoded string
          return {json(extracted_string), colon_index + 1 + length_of_string};

        }
        else{
                  // if no : is found
          throw std::runtime_error("Invalid encoded value:"+encoded_value);
        }

}

// integer 

std::pair<json, size_t> decode_bencoded_integer(const std::string& encoded_value, size_t index){
   //extracting integer part
   if(encoded_value[index] != 'i'){
      throw std::runtime_error("Invalid Integer Encoding");
   }
   // increase the index
   index++;

   // find the location of e
   size_t e_location = encoded_value.find("e", index);

    std::string int_part = encoded_value.substr(index,e_location-index);
    if( (int_part[0]=='0' && int_part !="0") || (int_part[0]=='-' && int_part[1]=='0')){
      // invalid integer encodingvv
    throw std::runtime_error("Invalid Integer Encoding");
    }
    else{
      index = e_location+1;
      // parse the string to get integer
      return {json(std::stoll(int_part)), index };
    }
    
}

// list
std::pair<json,size_t> decode_bencoded_list(const std::string& encoded_value, size_t index){
  // to store list data
  json decoded_list = json::array();
// increase the index to skip l
  index++;

  std::pair<json, size_t> result;
  // parse through the string until we reach end of list
  while(encoded_value[index]!='e'){
      if(std::isdigit(encoded_value[index])){
          result = decode_bencoded_string(encoded_value, index);  

          }
        else if(encoded_value[index]=='i'){
          result = decode_bencoded_integer(encoded_value,index);
        }
        else if(encoded_value[index]=='l'){
          result = decode_bencoded_list(encoded_value, index);
        }
        else{
          throw std::runtime_error("Unhandled encoded value: "+encoded_value);
        }
        decoded_list.push_back(result.first);
        index = result.second;
  }
   return {decoded_list, index + 1};
}

// dictionary
std::pair<json, size_t> decode_bencoded_dictionary(const std::string& encoded_value, size_t index){
  // increase the index to get past d 
  index++;
  // to store the json
  json decoded_dict = json::object();
  bool key = true;
  std::string curr_key;
  // check the datatype and call function accordingly
  std::pair<json, size_t> result;
  while(encoded_value[index]!='e'){
     if(std::isdigit(encoded_value[index])){
          result = decode_bencoded_string(encoded_value, index);  

          }
        else if(encoded_value[index]=='i'){
          result = decode_bencoded_integer(encoded_value,index);
        }
        else if(encoded_value[index]=='l'){
          result = decode_bencoded_list(encoded_value, index);
        }
        else if(encoded_value[index]=='d'){
          result = decode_bencoded_dictionary(encoded_value, index);
        }
        else{
          throw std::runtime_error("Unhandled encoded value: "+encoded_value);
        }
        
        if(key){
          curr_key = result.first;
          key=false;
        }
        else{
          decoded_dict[curr_key] = result.first;
          key=true;
        }
        index = result.second;
      
  }
  return {decoded_dict, index+1};
}
//
json decode_bencoded_value(const std::string& encoded_value, size_t index){
  // string bencoded value have this format digit: string , here digit = length of string
  if(std::isdigit(encoded_value[index])){
        std::pair<json, size_t> decoded_value = decode_bencoded_string(encoded_value,index);
        return decoded_value.first;
  }
  else if(encoded_value[index]=='i'){
        std::pair<json, size_t> decoded_value  = decode_bencoded_integer(encoded_value,index);
        return decoded_value.first;
  }
  else if(encoded_value[index]=='l'){
        std::pair<json, size_t> decoded_value  = decode_bencoded_list(encoded_value,index);
        return decoded_value.first;

  }
  else if(encoded_value[index]=='d'){
    std::pair<json, size_t> decoded_value = decode_bencoded_dictionary(encoded_value, index);
    return decoded_value.first;
  }
  else {
    throw std::runtime_error("Unhandled encoded value: "+encoded_value);
  }
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
      json decoded_value = decode_bencoded_value(encoded_value, 0);

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
      std::cout << "Pieces Length: "<< decoded_value["info"]["length"].get<int>()<<std::endl;
      
   }
    catch(const std::exception& e){
      std::cerr << "Error: " << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
