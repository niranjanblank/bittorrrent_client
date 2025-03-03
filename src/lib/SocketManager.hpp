#include <iostream>
#include <string>
#include<winsock2.h>
#include <ws2tcpip.h>

class SocketManager{
  private:
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    bool socketInitialized = false;
  public:
    // constructor to initialize winsock
    SocketManager() {
      if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        throw std::runtime_error("WSAStartup Failed");
      }
    }
    // destructor to clean up winsock
    ~SocketManager(){
      if(socketInitialized){
        closesocket(clientSocket);
      }
      WSACleanup();
    }

    // method to initialize and connect socketInitialized
    void connectToServer(const std::string& peer_ip, 
        unsigned short peer_port){
      // create the socketInitialized
      clientSocket = socket(AF_INET, SOCK_STREAM, 0);

      if (clientSocket == INVALID_SOCKET){
        throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
      }

      socketInitialized = true;


      // Define the server address
        sockaddr_in serverAddress = {};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(peer_port);

        // Convert IP string to binary form
        if (InetPton(AF_INET, peer_ip.c_str(), &serverAddress.sin_addr.s_addr) <= 0) {
            throw std::runtime_error("Invalid IP address: " + std::to_string(WSAGetLastError()));
        }

        // Connect to the server
        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            throw std::runtime_error("Client connection failed: " + std::to_string(WSAGetLastError()));
        }

        //std::cout << "Client connected to the server: " << peer_ip << ":" <<peer_port <<std::endl ;
    }

    // getter for the socket
    SOCKET getClientSocket() const {
      return clientSocket;
    }
};
