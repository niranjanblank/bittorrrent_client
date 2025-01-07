
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

#include "lib/structures.hpp"
#include "lib/PeerMessageHandler.hpp"

class PieceDownloader {
public:
    PieceDownloader(SOCKET client_socket, PeerMessageHandler& message_handler)
        : client_socket_(client_socket), message_handler_(message_handler) {}

    // send request message to server for downloading a specific block of a piece
    void send_request_message(SOCKET client_socket, uint32_t piece_index, uint32_t offset, uint32_t length) {
        uint32_t length_prefix = htonl(13); // message_id:1 byte, payload: 12 bytes
        uint8_t message_id = 6; // 6 is for request

        char buffer[17];

        memcpy(buffer, &length_prefix, 4);
        memcpy(buffer + 4, &message_id, 1);

        uint32_t network_piece_index = htonl(piece_index);
        uint32_t network_offset = htonl(offset);
        uint32_t network_length = htonl(length);
        memcpy(buffer + 5, &network_piece_index, 4);
        memcpy(buffer + 9, &network_offset, 4);
        memcpy(buffer + 13, &network_length, 4);

        send(client_socket, buffer, sizeof(buffer), 0);
        std::cout << "Request Message sent" << std::endl;
    }

    // download a single piece from the server
    std::optional<std::vector<uint8_t>> download_piece(SOCKET client_socket, uint32_t piece_index, uint32_t piece_length) {
        const uint32_t block_size = 16 * 1024;
        uint32_t offset = 0;

        std::cout << "---------Piece Info----------" << std::endl;
        std::cout << "Piece Index: " << piece_index << std::endl;
        std::cout << "Piece Length: " << piece_length << std::endl;
        std::cout << "-----------------------------" << std::endl;

        // to store the blocks downloaded
        std::vector<uint8_t> piece_data(piece_length);

        while (offset < piece_length) {
            // current block size
            uint32_t current_block_size = std::min(block_size, piece_length - offset);

            // send request to the server
            send_request_message(client_socket, piece_index, offset, current_block_size);

            PeerMessage message = message_handler_.read_peer_messages();

            // Handle keep-alive messages
            if (message.length == 0) {
                std::cout << "Keep-alive message received. Skipping..." << std::endl;
                continue; // Ignore and wait for the next message
            }

            std::cout << "Message ID received: " << static_cast<int>(message.id) << std::endl;

            if (message.id < 0 || message.id > 9) {
                std::cerr << "Unexpected message ID: " << static_cast<int>(message.id) << std::endl;

                // log the invalid message
                std::cerr << "Message length: " << message.length << std::endl;
                std::cerr << "Message payload: ";

                for (const auto& byte : message.payload) {
                    std::cerr << std::hex << static_cast<int>(byte) << std::endl;
                }
                return std::nullopt;
            }

            if (message.id == 7) {
                // piece message
                uint32_t received_index = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data()));
                uint32_t received_begin = ntohl(*reinterpret_cast<uint32_t*>(message.payload.data() + 4));

                std::vector<uint8_t> block(message.payload.begin() + 8, message.payload.end());

                if (received_index == piece_index && received_begin == offset) {
                    // Store the block in the piece_data buffer
                    std::copy(block.begin(), block.end(), piece_data.begin() + offset);

                    // Update the offset to move to the next block
                    offset += block.size();

                    std::cout << "-----Block Received---" << std::endl;
                    std::cout << "Received Index: " << received_index << std::endl;
                    std::cout << "Received Begin: " << received_begin << std::endl;
                    std::cout << "Offset : " << offset << std::endl;
                    std::cout << "Block Size: " << block.size() << std::endl;
                    std::cout << "----------------------" << std::endl;
                } else {
                    std::cerr << "Invalid block received. Expected index: " << piece_index
                              << ", offset: " << offset
                              << ", but got index: " << received_index
                              << ", offset: " << received_begin << std::endl;
                    continue; // Reattempt to download the current block
                }
            }
        }

        return piece_data;
    }

    // handle downloading all pieces from the server
    std::vector<uint8_t> handle_download_pieces(SOCKET client_socket, int total_length, int piece_length, const std::string& piece_hash) {
        // buffer to hold the entire data
        std::vector<uint8_t> file_data(total_length, 0);
        size_t accumulated_size = 0; // Tracks the current write position in the file buffer

        // get total pieces
        int total_pieces = static_cast<int>(std::ceil(static_cast<double>(total_length) / piece_length));
        std::cout << "Total Pieces: " << total_pieces << std::endl;
        std::cout << "Piece Length: " << piece_length << std::endl;

        int bytes_to_send = piece_length;

        std::optional<std::vector<uint8_t>> piece_data;
        while (true) {
            PeerMessage message = message_handler_.read_peer_messages();

            // if we encounter keep alive message, we skip processing
            if (message.length == 0) {
                std::cout << "Received keep-alive message." << std::endl;
                continue; // Skip processing for keep-alive
            }

            // process peer messages
            std::cout << "Processing message ID: " << static_cast<int>(message.id) << std::endl;

            switch (message.id) {
                case 0: // Choke message
                    std::cout << "Received choke message. Cannot request pieces until unchoked." << std::endl;
                    break;

                case 5:
                    // bit field message
                    message_handler_.handle_bit_field(message);
                    message_handler_.send_interested();
                    break;

                case 1:
                    // unchoke message;
                    std::cout << "Received unchoke message" << std::endl;

                    // download all the pieces
                    for (int i = 0; i < total_pieces; i++) {
                        int bytes_sent = (i + 1) * piece_length;

                        if (bytes_sent > total_length) {
                            bytes_to_send = total_length - (i * piece_length);
                        }
                        std::cout << "Index: " << i << ", Bytes: " << bytes_to_send << std::endl;

                        // download piece at index
                        piece_data = download_piece(client_socket, static_cast<uint32_t>(i), static_cast<uint32_t>(bytes_to_send));
                        if (!piece_data) {
                            std::cerr << "Error downloading piece data at index " << i << ". Terminating peer message handling." << std::endl;
                            return {};
                        }

                        // validate the hash
                        std::string computed_hash = bytes_to_hash(*piece_data);
                        std::string expected_hash = piece_hash.substr(i * 40, 40);
                        if (computed_hash == expected_hash) {
                            std::cout << "Hash Validated" << std::endl;
                        } else {
                            std::cerr << "Invalid Hash" << std::endl;
                            return {};
                        }

                        // piece data downloaded
                        std::cout << "Piece downloaded successfully, size: " << piece_data->size() << " bytes." << std::endl;

                        // copy the downloaded piece to the buffer
                        size_t copy_length = piece_data->size();
                        if (accumulated_size + copy_length > file_data.size()) {
                            std::cerr << "Error: Piece data exceeds file buffer size." << std::endl;
                            return {}; // Return empty file data on failure
                        }
                        std::copy(piece_data->begin(), piece_data->end(), file_data.begin() + accumulated_size);
                        accumulated_size += copy_length;
                        std::cout << "Piece " << i << " added to the buffer" << std::endl;
                    }
                    return file_data;

                default:
                    std::cerr << "Unexpected message ID: " << static_cast<int>(message.id) << std::endl;
                    break;
            }
        }
    }

private:
    SOCKET client_socket_;
    PeerMessageHandler& message_handler_;
};

