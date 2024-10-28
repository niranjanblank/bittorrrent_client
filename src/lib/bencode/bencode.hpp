#include<string>
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
using json = nlohmann::json;


// functions for encode
std::string integer_to_bencode(int value);
std::string string_to_bencode(const std::string& value);
std::string list_to_bencode(const json& json_array);
std::string dict_to_bencode(const json& json_dict);
std::string encode_bencode(const json& j);


// functions for decode
json decode_bencoded_string(const std::string& encoded_value, size_t& index);
json decode_bencoded_integer(const std::string& encoded_value, size_t& index);
json decode_bencoded_list(const std::string& encoded_value, size_t& index);
json decode_bencoded_dictionary(const std::string& encoded_value, size_t& index);
json decode_bencode(const std::string& encoded_value, size_t& index);

// string
json decode_bencoded_string(const std::string& encoded_value, size_t& index){
        size_t colon_index = encoded_value.find(":", index);
        if(colon_index != std::string::npos){

          int length_of_string = std::stoi(encoded_value.substr(index,colon_index-index));
          // extracted string from colon_index+1 to the end
          std::string extracted_string = encoded_value.substr(colon_index+1, length_of_string);
          // validate length of extracted string matches actual length

          index = colon_index + 1 + length_of_string;
          // colon_index + 1 + length_of_string gives the ending of current bencoded string
          return json(extracted_string);

        }
        else{
                  // if no : is found
          throw std::runtime_error("Invalid encoded value:"+encoded_value);
        }

}
// integer 


json decode_bencoded_integer(const std::string& encoded_value, size_t& index){
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
      return json(std::atoll(int_part.c_str()));
    }

}

// list

json decode_bencoded_list(const std::string& encoded_value, size_t& index){
  // to store list data
  json decoded_list = json::array();
  // increase the index to skip l
  index++;


  // parse through the string until we reach end of list
  while(index < encoded_value.length()&& encoded_value[index]!='e'){
    json result = decode_bencode(encoded_value, index);
    decoded_list.push_back(result);
  }
  // increase index to skip e
  index = index + 1;
  return json(decoded_list);
}


// dictionary
json decode_bencoded_dictionary(const std::string& encoded_value, size_t& index){
  // increase the index to get past d
  index++;
  // to store the json
  json decoded_dict = json::object();

  while(index < encoded_value.length() && encoded_value[index]!='e'){
   json key = decode_bencoded_string(encoded_value, index);
    json value = decode_bencode(encoded_value, index);
    decoded_dict[key.get<std::string>()] = value;


  }
  index = index + 1;
  return decoded_dict;
}

// interger to bencoded
std::string integer_to_bencode(int value){
  return "i"+std::to_string(value)+"e";
}

// string to bencoded
std::string string_to_bencode(const std::string& value){
    return std::to_string(value.length())+":"+value;
}
// list to bencoded{
std::string list_to_bencode(const json& json_array){
  std::string bencoded_data = "l";

  for (auto& item: json_array){
    bencoded_data += encode_bencode(item);
  }

  bencoded_data += "e";
  return bencoded_data;
}

// Function to encode a json dictionary into Bencode format
std::string dict_to_bencode(const json& json_dict) {
    std::string bencoded_data = "d";

    // Get all keys and sort them lexicographically
    std::vector<std::string> keys;
    for (auto it = json_dict.begin(); it != json_dict.end(); it++) {
        keys.push_back(it.key());
    }
    std::sort(keys.begin(), keys.end());

    // Encode key-value pairs in Bencode format
    for (const auto& key : keys) {
        bencoded_data += string_to_bencode(key);  // Keys must always be strings in Bencode
        bencoded_data += encode_bencode(json_dict[key]);  // Values can be any Bencode-compatible type
    }

    bencoded_data += "e";  // End of dictionary
    return bencoded_data;
}

// entry point to decode bencoded data
json decode_bencode(const std::string& encoded_value, size_t& index){
  // string bencoded value have this format digit: string , here digit = length of string
  if(std::isdigit(encoded_value[index])){
        json decoded_value = decode_bencoded_string(encoded_value,index);
        return decoded_value;
  }
  else if(encoded_value[index]=='i'){
        json decoded_value  = decode_bencoded_integer(encoded_value,index);
        return decoded_value;
  }
  else if(encoded_value[index]=='l'){
        json decoded_value  = decode_bencoded_list(encoded_value,index);
        return decoded_value;

  }
  else if(encoded_value[index]=='d'){
    json decoded_value = decode_bencoded_dictionary(encoded_value, index);
    return decoded_value;
  }
  else {
    throw std::runtime_error("Unhandled encoded value: "+encoded_value);
  }
}

std::string encode_bencode(const json& j){

  if(j.is_string()){
    return string_to_bencode(j.get<std::string>());
  }
  else if(j.is_number_integer()){
    return integer_to_bencode(j.get<int>());
  }
  else if(j.is_array()){
    return list_to_bencode(j);
  }
  else if(j.is_object()){
    return dict_to_bencode(j);
  }
  else {
    throw std::runtime_error("Unsupported json type for bencode");
  }
}


