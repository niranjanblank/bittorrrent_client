#include<iostream>
#include<string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;


// string 
std::pair<json, size_t> decode_bencoded_string(const std::string& encoded_value, size_t index){
        size_t colon_index = encoded_value.find(":", index);
        if(colon_index != std::string::npos){
          
          int length_of_string = std::stoi(encoded_value.substr(index,colon_index-index));
          // extracted string from colon_index+1 to the end 
          std::string extracted_string = encoded_value.substr(colon_index+1, length_of_string);
          // validate length of extracted string matches actual length 
    
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
        else{
          throw std::runtime_error("Unhandled encoded value: "+encoded_value);
        }
        decoded_list.push_back(result.first);
        index = result.second;
  }
   return {decoded_list, index + 1};
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
  else {
    throw std::runtime_error("Unhandled encoded value: "+encoded_value);
  }
}

int main(int argc, char* argv[]){
  if(argc>1){
    try{
      std::string input_encoded_value = argv[1];
      json decoded_value = decode_bencoded_value(input_encoded_value,0);
      std::cout << decoded_value;
    }
    catch(const std::exception& e){
      std::cerr << "Error:" << e.what() <<std::endl;
  
    }
  }
  return 0; 
}
