# BitTorrent Client

BitTorrent Client is a custom implementation of the BitTorrent protocol, built from scratch to understand and replicate the basic functionalities of a BitTorrent client. This project is designed as a learning tool to explore how the BitTorrent protocol works under the hood and to implement core features incrementally.

## Features

- **Torrent File Parsing**: Extract metadata such as trackers, file information, and piece hashes from `.torrent` files.
- **Tracker Communication**: Communicate with trackers to announce, scrape, and retrieve peer information.
- **Peer-to-Peer Communication**: Establish connections with peers and exchange data blocks.
- **Piece Downloading**: Download pieces of the file from multiple peers simultaneously.
- **Piece Verification**: Validate downloaded pieces using SHA-1 hashing.
- **Resumable Downloads**: Save and resume partially downloaded files.
- **Multi-threading**: Efficient use of system resources by handling multiple peers simultaneously.
- **Command-Line Interface (CLI)**: Simple and intuitive CLI for managing torrents.

## Directory Structure

```markdown
BitTorrent_Client/
├── CMakeLists.txt          # Build configuration
├── src/                    # Source files
│   ├── TorrentParser       # Handles .torrent file parsing
│   │   └── bencode         # Handles bencode decoding
│   ├── HandshakeHandler.hpp # Manages handshake process with peers
│   ├── PeerDiscovery.hpp   # Handles discovering and connecting to peers
│   ├── PeerMessageHandler.hpp # Manages communication with peers
│   ├── PieceDownloader.hpp # Handles downloading pieces from peers
│   ├── SocketManager.hpp   # Manages socket connections
│   ├── common.hpp          # Includes common utilities and constants
│   ├── sha1.hpp            # For hash calculations
│   ├── utils.hpp           # Additional helper functions
│   ├── main.cpp            # Main program entry point
├── build/                  # Build directory (generated files)
```

## Getting Started

### Prerequisites

Ensure you have the following installed:

- **Compiler**: Modern C++17-compatible compiler:
  - GCC 7+
  - Clang 6+
  - MSVC with appropriate flags
- **CMake**: Version 3.10 or higher.

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/niranjanblank/bittorrrent_client
   cd bittorrent_client
   ```

2. Create a build directory and navigate to it:
   ```bash
   mkdir build
   cd build
   ```

3. Generate the build files with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build .
   ```

### Running the Application

Navigate to the build\Debug directory and run the client:

```bash
bittorrent sample.torrent
```

## Contact

For questions or feedback, please contact [Niranjan Shah](mailto:niranjanshah@example.com).

