#include<iostream>
#include<string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

void decode_bencoded_value(const std::string& encoded_value){
  // string benced value have this format digit: string , here digit = length of string
  if(std::isdigit(encoded_value[0])){
    size_t colon_index = encoded_value.find(":");
    std::cout <<"First Occurence at: " << colon_index << std::endl;
    int length_of_string = std::stoi(encoded_value.substr(0,colon_index));
    
    std::cout << length_of_string << std::endl;
    std::cout << encoded_value.substr(colon_index+1,  length_of_string);
  }
}

int main(int argc, char* argv[]){
  
  
  decode_bencoded_value("5:hello");
  return 0; 
}
