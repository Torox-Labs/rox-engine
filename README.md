# Rox-Engine

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Contributors](https://img.shields.io/github/contributors/Torox-Lab/rox-engine.svg)](https://github.com/Torox-Lab/rox-engine/graphs/contributors)
[![Stars](https://img.shields.io/github/stars/Torox-Lab/rox-engine.svg?style=social&label=Star)](https://github.com/Torox-Lab/rox-engine)

> **"Games are built by developers, run by players, and owned by communities."**

Rox-Engine is an innovative, decentralized game engine/framework written in **C++**, designed to revolutionize the development of Web3-native games. It empowers developers to create immersive, secure, and decentralized gaming experiences with advanced cryptographic features and seamless blockchain integration.

**Note:** Rox-Engine is currently in its early development stages (Alpha phase). We are actively expanding its capabilities and maintaining consistency across our repositories:

- [Rox-Engine](https://github.com/Torox-Lab/rox-engine)
- [Torox-Math](https://github.com/Torox-Lab/torox-math)
- [Orox-Token](https://github.com/Torox-Lab/orox-token)
- [Torox-SDK](https://github.com/Torox-Lab/torox-sdk)

Contributions from the community are highly encouraged!

## Table of Contents

- [Key Features](#key-features)
  - [P2P Networking for Game Connectivity](#p2p-networking-for-game-connectivity)
  - [Anonymity Network Integration](#anonymity-network-integration)
  - [Zero-Knowledge Proofs (ZKPs)](#zero-knowledge-proofs-zkps)
  - [Distributed Ledger Technology](#distributed-ledger-technology)
  - [Comprehensive Web3 Integration](#comprehensive-web3-integration)
  - [Wallet Integration via Browser](#wallet-integration-via-browser)
- [Architecture Overview](#architecture-overview)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
  - [Basic Usage](#basic-usage)
- [Documentation](#documentation)
- [Roadmap](#roadmap)
  - [Phase 1: Core Development (Current Stage)](#phase-1-core-development-current-stage)
  - [Phase 2: Feature Expansion](#phase-2-feature-expansion)
  - [Phase 3: Optimization and Security](#phase-3-optimization-and-security)
  - [Phase 4: Community and Ecosystem Building](#phase-4-community-and-ecosystem-building)
- [Contributing](#contributing)
- [License](#license)
- [Contact](#contact)
- [Important Notice](#important-notice)

---

## Key Features

### P2P Networking for Game Connectivity

- **Decentralized Multiplayer:** Enables direct peer-to-peer connections between players, allowing each game instance to function as a node within a decentralized network.
- **Serverless Architecture:** Eliminates reliance on centralized servers, reducing costs and single points of failure.
- **Scalable Networking:** Designed to support a large number of concurrent players with efficient resource utilization.

### Anonymity Network Integration

- **Enhanced Privacy:** Plans to integrate with anonymity networks such as Tor or I2P to conceal players' IP addresses and metadata.
- **Secure Communication:** Utilizes encrypted channels to protect data exchange between nodes.
- **User Safety:** Protects players from potential DDoS attacks and location tracking.

### Zero-Knowledge Proofs (ZKPs)

- **Secure Verification:** Implements experimental ZKP mechanisms, allowing players to prove ownership of in-game assets without revealing sensitive information.
- **Anti-Cheating Measures:** Verifies player actions and transactions without exposing underlying data, reducing fraud and cheating.
- **Cryptographic Innovations:** Explores cutting-edge cryptographic techniques to enhance game security and integrity.

  *Note:* ZKPs are advanced cryptographic protocols that enable one party (the prover) to prove to another (the verifier) that a statement is true without revealing any information beyond the validity of the statement itself.

### Distributed Ledger Technology

- **Blockchain Integration:** Synchronizes game state across all nodes using blockchain technology.
- **Consensus Mechanisms:** Implements consensus algorithms (e.g., Proof of Stake, Practical Byzantine Fault Tolerance) to ensure fairness and consistency.
- **Immutable Records:** Maintains a tamper-proof ledger of game events, transactions, and player interactions.

### Comprehensive Web3 Integration

- **Asset Management:** Supports the creation and integration of NFTs and other blockchain-based assets within games.
- **Smart Contract Interaction:** Provides interfaces for seamless interaction with smart contracts on various blockchain networks.
- **Developer Tools and SDKs:** Offers a suite of tools and libraries in C++ to streamline the development process and reduce complexity.

### Wallet Integration via Browser

- **Seamless Wallet Connections:** Enables players to connect their crypto wallets through their web browsers, similar to popular Web3 applications and games.
- **Enhanced User Experience:** Simplifies the process of linking wallets without requiring in-game software modifications.
- **Security and Privacy:** Ensures secure handling of wallet data and transactions through encrypted communication channels.
- **Cross-Platform Compatibility:** Supports major wallet providers and browsers, ensuring broad accessibility for players.

---

## Architecture Overview

Rox-Engine's architecture is modular and extensible, consisting of the following core components:

- **Networking Module:** Handles P2P connections, data transmission, and network topology management.
- **Blockchain Module:** Manages blockchain interactions, including transactions, smart contracts, and asset management.
- **Security Module:** Implements cryptographic functions, ZKPs, and anonymity features.
- **Game Engine Core:** Provides essential game development functionalities like rendering, physics, input handling, and scene management.
- **Wallet Integration Module:** Facilitates secure connections between the game engine and players crypto wallets via web browsers.
- **SDK and APIs:** Offers a set of tools and interfaces for developers to build and customize their games efficiently.

*Visual representation coming soon.*

---

## Getting Started

### Prerequisites

- **Operating System:**
  - Windows 10 or higher
  - macOS Catalina or higher
  - Linux (Ubuntu 18.04+)

- **Compiler:**
  - A modern C++ compiler (e.g., GCC 9+, Clang 9+, MSVC 2015+)

- **Build System:**
  - CMake 3.16 or higher

- **Dependencies:**
  - **OpenGL:** For rendering graphics. [Installation Guide](https://www.opengl.org/wiki/Getting_Started)
  - **GLFW:** A library for OpenGL, window, and input management. [Installation Guide](https://www.glfw.org/docs/latest/compile.html)
  - **Boost Libraries:** [Installation Guide](https://www.boost.org/doc/libs/1_76_0/more/getting_started/index.html)
  - **OpenSSL:** For cryptographic functions. [Installation Guide](https://www.openssl.org/source/)
  - **[Torox-Math Library](https://github.com/Torox-Lab/torox-math):** For advanced mathematical functions.
  - **[Orox-Token Library](https://github.com/Torox-Lab/orox-token):** For token and blockchain interactions.

*Note:* Torox-Math, Torox-Token, SDL3 and Assimp are under consideration for future integration and are not required at this stage.

### Installation

1. **Clone the Repository**

   ```bash
   git clone https://github.com/Torox-Lab/rox-engine.git
   cd rox-engine
   ```

2. **Initialize Submodules**

   If using Git submodules for dependencies:

   ```bash
   git submodule update --init --recursive
   ```

3. **Install Dependencies**

   Ensure all external libraries are installed and properly linked. Refer to the [Prerequisites](#prerequisites) section for installation guides.

4. **Build the Engine**

   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

### Basic Usage

After building the engine, you can start creating your game project:

1. **Create a New Project**

   Use the provided project generator script:

   ```bash
   ./tools/create_project.sh MyGameProject
   ```

2. **Include Rox-Engine Headers**

   In your C++ source files:

   ```cpp
   #include <rox/app.hpp>
   ```

   *Note:* Currently, to use the engine, you need to include `rox/app.hpp`. We plan to update this to `rox/rox.hpp` in future releases for consistency.

3. **Initialize the Engine**

   ```cpp
   int main() {
       Rox::Engine engine;
       engine.initialize();
       // Your game logic here
       engine.run();
       return 0;
   }
   ```

4. **Build and Run Your Game**

   Use your preferred build system to compile and run your game project.

---

## Documentation

Comprehensive documentation is being developed and will include:

- **API Reference:** Detailed descriptions of all classes, functions, and modules.
- **Tutorials:** Step-by-step guides for common tasks and features.
- **Sample Projects:** Example game projects demonstrating various engine capabilities.
- **Integration Guides:** Instructions on integrating with different blockchain networks, anonymity services, and wallet providers.

Documentation will be available in the `/docs` directory and online at [docs.torox.org](https://docs.torox.org) *(coming soon)*.

---

## Roadmap

Our roadmap outlines the planned development stages of Rox-Engine:

### Phase 1: Core Development (Current Stage)

- [x] Set up project repository and development environment.
- [x] Establish the foundational codebase for Rox-Engine.
- [ - ] Develop core engine functionalities (rendering, physics, input handling) using OpenGL and GLFW.
- [ ] Implement basic P2P networking capabilities.
- [ ] Establish initial blockchain interaction layer.
- [ ] Release Alpha version for internal testing.

### Phase 2: Feature Expansion

- [ ] Enhance P2P networking with NAT traversal and connection stability improvements.
- [ ] Integrate [Torox-Math](https://github.com/Torox-Lab/torox-math) for advanced mathematical computations.
- [ ] Implement anonymity network support (Tor/I2P integration).
- [ ] Develop experimental ZKP functionalities for secure verification.
- [ ] Introduce smart contract interaction capabilities.
- [ ] Implement Wallet Integration via Browser for seamless crypto wallet connections.
- [ ] Evaluate and possibly integrate SDL3 and Assimp for additional functionality.
- [ ] Release Beta version for community testing.

### Phase 3: Optimization and Security

- [ ] Optimize engine performance for various platforms.
- [ ] Conduct security audits and vulnerability assessments.
- [ ] Refine ZKP implementations and test extensively.
- [ ] Enhance consensus algorithms for distributed ledger synchronization.
- [ ] Expand documentation and create developer tutorials.
- [ ] Implement comprehensive Wallet Integration features and support for major wallet providers.
- [ ] Release Version 1.0 for public use.

### Phase 4: Community and Ecosystem Building

- [ ] Establish a community forum and support channels.
- [ ] Host developer workshops and webinars.
- [ ] Encourage community contributions and third-party integrations.
- [ ] Explore partnerships with blockchain platforms and gaming studios.
- [ ] Continuously update and improve the engine based on feedback.
- [ ] Develop a marketplace for Web3 assets and plugins.

---

## Contributing

We are excited to collaborate with developers and enthusiasts who share our vision. To contribute:

1. **Fork the Repository**

   Click the "Fork" button at the top right of this page.

2. **Create a Feature Branch**

   ```bash
   git checkout -b feature/YourFeature
   ```

3. **Commit Your Changes**

   ```bash
   git commit -m "Add YourFeature"
   ```

4. **Push to Your Fork**

   ```bash
   git push origin feature/YourFeature
   ```

5. **Submit a Pull Request**

   Open a pull request detailing your changes and contributions.

Please refer to our [Contributing Guidelines](CONTRIBUTING.md) for more information.

---

## License

Rox-Engine is licensed under the [MIT License](LICENSE), allowing you flexibility in how you use the engine in your projects.

---

## Contact

For inquiries, support, or feedback:

- **Email:** [support@torox.org](mailto:support@torox.org)
- **Website:** [torox.org](https://torox.org)
- **GitHub Issues:** [Submit an issue](https://github.com/Torox-Lab/rox-engine/issues)

---

## Important Notice

*Rox-Engine is currently in active development and certain features are in the conceptual or experimental stages. We are committed to transparency and will keep the community updated on our progress through regular commits and roadmap adjustments.*

*We invite you to join us on this exciting journey to pioneer the future of decentralized, community-driven gaming.*

---

*By focusing on creating a robust and decentralized game engine, we believe that Rox-Engine embodies the spirit of community-driven development where games are truly built by developers, run by players, and owned by communities.*

---

# Acknowledgments

We would like to thank all contributors, supporters, and the open-source community for their invaluable input and collaboration.

---

**Note:** This README is intended to provide an overview of Rox-Engine's vision and direction. We encourage developers and enthusiasts to contribute, provide feedback, and help shape the future of decentralized gaming.

---

*Last updated on, October 10, 2024*
