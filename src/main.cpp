#include<iostream>
#include<string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded_value){
  // string bencoded value have this format digit: string , here digit = length of string
  if(std::isdigit(encoded_value[0])){
    size_t colon_index = encoded_value.find(":");
    int length_of_string = std::stoi(encoded_value.substr(0,colon_index));
    
    return json(encoded_value.substr(colon_index+1,  length_of_string));

  }
  else if(encoded_value[0]=='i' && encoded_value.back()=='e'){
    //extracting integer part
    std::string int_part = encoded_value.substr(1,encoded_value.size()-2);
    if( (int_part[0]=='0' && int_part !="0") || (int_part[0]=='-' && int_part[1]=='0')){
      // invalid integer encoding
      throw std::runtime_error("Invalid Integer Encoding");
    }
    else{
      // parse the string to get integer
      return json(std::stoll(int_part));
    }
    
  }
  else {
    throw std::runtime_error("Unhandled encoded value: "+encoded_value);
  }
}

int main(int argc, char* argv[]){
  if(argc>1){
    try{
      std::string input_encoded_value = argv[1];
      json decoded_value = decode_bencoded_value(input_encoded_value);
      std::cout << decoded_value;
    }
    catch(const std::exception& e){
      std::cerr << "Error:" << e.what() <<std::endl;
    }
  }
  return 0; 
}
