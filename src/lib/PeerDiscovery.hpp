#include<iostream>
#include<sstream>
#include<string>
#include<vector>
#include "lib/TorrentParser/TorrentParser.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/bencode/bencode.hpp"
#include "lib/httplib.h"

struct Peer{
  std::string ip;
  uint16_t port;

  Peer(const std::string& ipAddress, uint16_t portNumber):
    ip(ipAddress), port(portNumber) {}
};

class PeerDiscovery{
  private:
    TorrentFile torrent;
    std::string domain;
    std::string entryPoint;

    // function foor encoding the urlEncode
    std::string urlEncode(const std::string& value) {
        std::ostringstream escaped;

        size_t i = 0;
        while (i < value.size()) {
            std::string two_chars = value.substr(i, 2);
            escaped << "%" << two_chars;
            i += 2;
        }

        return escaped.str();
    }

    // parsing all the peers from the request body
     std::vector<Peer> parsePeers(const std::string& responseBody) {
        std::vector<Peer> peers;
        try {
          // starting index to begin decoding
            size_t start_index = 0;
            // we get response as bencode from the server, so we need to decode it to get data
            json decodedResponse = decode_bencode(responseBody, start_index);

            std::cout << "Interval: " << decodedResponse["interval"] << std::endl;
            std::cout << "Peers size: " << decodedResponse["peers"].size() << std::endl;

            std::string peerString = decodedResponse["peers"];

            // Parse compact peer list
            for (size_t i = 0; i < peerString.size(); i += 6) {
                int ip1 = static_cast<unsigned char>(peerString[i]);
                int ip2 = static_cast<unsigned char>(peerString[i + 1]);
                int ip3 = static_cast<unsigned char>(peerString[i + 2]);
                int ip4 = static_cast<unsigned char>(peerString[i + 3]);
        
                //A uint16_t can hold values from 0 to 65535 (which is the range of valid port numbers)
                uint16_t port = (static_cast<uint16_t>(static_cast<unsigned char>(peerString[i + 4])) << 8) |
                                static_cast<uint16_t>(static_cast<unsigned char>(peerString[i + 5]));

                std::string ipAddress = std::to_string(ip1) + "." + std::to_string(ip2) + "." +
                                        std::to_string(ip3) + "." + std::to_string(ip4);

                peers.emplace_back(ipAddress, port);  // Add a Peer to the list
            }
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
        return peers;
    }

  public:
    PeerDiscovery(const TorrentFile& torrent): torrent(torrent){
      // setting the domain and entry point
      size_t last_index = torrent.base_url.find_last_of("/");
      // getting the domain and entry point, as cpp-httplib requires client to be made out of just domain
      if (last_index == std::string::npos) {
            throw std::invalid_argument("Invalid base URL in TorrentFile");
        }
      this->domain = torrent.base_url.substr(0,last_index);
      this->entryPoint = torrent.base_url.substr(last_index); 
    }

    // send request to the server and get the peers
    std::vector<Peer> discoverPeers(){
      std::cout<<"Entry: "<<entryPoint<<std::endl;

      httplib::Client client(this->domain);
      client.set_follow_location(true);

    // Define custom headers
    httplib::Headers headers = {
        {"User-Agent", "Mozilla/5.0"},  // Mimic a browser to avoid server rejection
        {"Accept", "*/*"},              // Accept any content type
        {"Connection", "keep-alive"}    // Keep the connection open if possible
    };
    
      // Create a string to hold the query parameters
    std::string queryParams = this->entryPoint + "?info_hash=" + urlEncode(this->torrent.info_hash) +
                                  "&peer_id=" + this->torrent.peer_id +
                                  "&port=" + std::to_string(this->torrent.port) +
                                  "&uploaded=" + std::to_string(this->torrent.uploaded) +
                                  "&downloaded=" + std::to_string(this->torrent.downloaded) +
                                  "&left=" + std::to_string(this->torrent.left) +
                                  "&compact=" + (this->torrent.compact ? "1" : "0");
    

 // send GET request
    auto res = client.Get(queryParams.c_str(), headers);

    if (!res || res->status != 200) {
        std::cerr << "Request failed with status: "
                  << (res ? std::to_string(res->status) : "no response") << "\n";
        return {};
    }

    return parsePeers(res->body);
    }
};
