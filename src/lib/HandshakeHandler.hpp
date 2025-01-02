#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <winsock2.h>
#include "lib/utils.hpp"

#include "lib/structures.hpp"


class HandshakeHandler {
public:
    HandshakeHandler(const std::string& info_hash, const std::string& peer_id)
        : info_hash_(info_hash), peer_id_(peer_id) {}

    // function for handshake creation
    std::string create_handshake() const {
        std::string handshake;
        // length of the protocol
        handshake += static_cast<char>(19);
        handshake += "BitTorrent protocol";

        // reserve 8 bytes
        uint8_t reserved[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        handshake.append(reinterpret_cast<const char*>(reserved), sizeof(reserved));

        // convert the hex-encoded info_hash to raw bytes
        std::vector<uint8_t> info_hash_bytes = hex_to_bytes(info_hash_);
        handshake.append(info_hash_bytes.begin(), info_hash_bytes.end());

        handshake += peer_id_;
        return handshake;
    }

    // function to parse the received handshake message
    HandshakeMessage parse_handshake_message(const char* received_data, int bytes_received) const {
        HandshakeMessage handshake_message;

        // parsing the handshake
        // first byte is the length of the protocol
        handshake_message.protocol_length = static_cast<uint8_t>(received_data[0]);

        // protocol identifier (19 bytes)
        handshake_message.protocol_identifier = std::string(
            received_data + 1, handshake_message.protocol_length);

        // 8 reserved bytes
        handshake_message.reserved_bytes = std::vector<uint8_t>(
            received_data + 1 + handshake_message.protocol_length,
            received_data + 1 + handshake_message.protocol_length + 8);

        // 20 bytes info_hash
        handshake_message.info_hash = std::vector<uint8_t>(
            received_data + 1 + handshake_message.protocol_length + 8,
            received_data + 1 + handshake_message.protocol_length + 8 + 20);

        // 20 bytes peer_id
        handshake_message.peer_id = std::string(
            received_data + 1 + handshake_message.protocol_length + 8 + 20,
            received_data + 1 + handshake_message.protocol_length + 8 + 20 + 20);

        return handshake_message;
    }

    // function to send the handshake and process the response
    std::optional<HandshakeMessage> send_handshake(SOCKET client_socket) const {
        std::string handshake = create_handshake();
        std::cout << "Handshake message: " << handshake << std::endl;

        // send handshake to server
        size_t byte_count = send(client_socket, handshake.c_str(), handshake.size(), 0);
        if (byte_count == SOCKET_ERROR) {
            throw std::runtime_error("Server sent error: " + std::to_string(WSAGetLastError()));
        }

        std::cout << "Handshake sent: " << byte_count << " bytes" << std::endl;

        // receive data from server
        char handshake_buffer[68];
        int bytes_received = recv(client_socket, handshake_buffer, sizeof(handshake_buffer), 0);

        if (bytes_received <= 0) {
            throw std::runtime_error("Failed to receive handshake response: " + std::to_string(WSAGetLastError()));
        }

        if (bytes_received < 68) {
            throw std::runtime_error("Failed to receive complete handshake: " + std::to_string(bytes_received) + " bytes received.");
        }

        // parse the received handshake
        HandshakeMessage parsed_handshake = parse_handshake_message(handshake_buffer, bytes_received);
        std::cout << "Protocol Length: " << static_cast<int>(parsed_handshake.protocol_length) << std::endl;
        std::cout << "Protocol identifier: " << parsed_handshake.protocol_identifier << std::endl;
        std::cout << "PeerID: " << to_hex(parsed_handshake.peer_id) << std::endl;

        return parsed_handshake;
    }

private:
    std::string info_hash_;
    std::string peer_id_;
};

