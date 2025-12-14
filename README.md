# 🦎🐛🐝 Distributed Multiplayer Game System

A distributed real-time multiplayer system implemented in C, where three types of clients — **Lizards**, **Roaches**, and **Wasps** — interact on a shared board managed by a central **Server**.  
All communication between clients and server is performed through **ZeroMQ TCP sockets**, enabling asynchronous message passing and coordinated state updates.

This project demonstrates principles of **distributed systems**, **network programming**, **inter-process communication**, and **client–server architectures**.

- Centralized **game server** maintaining global board state  
- Independent clients with distinct behaviors  
- **ZeroMQ-based TCP communication** between server and clients  
- Real-time state updates and event handling  
- Support for **multiple simultaneous clients** of each type  
- Defensive protocol handling for invalid or unexpected messages  

---

## Game Rules and Client Restrictions

### 🦎 Lizard Client
- Moves according to user keyboard input  
- **Eats roaches**, gaining the roach’s value  
- **Gets stung by wasps**, losing 10 points per sting  
- When two lizards collide, **scores are averaged**  
- Wins at **50 points**, loses at **negative score**  

### 🐛 Roach Client
- Moves **randomly**  
- Each roach has a **numerical value**  
- Can be eaten by lizards  

### 🐝 Wasp Client
- Moves **randomly**  
- Stings lizards, reducing their score  
- Cannot be eaten  

### Global Restrictions
- All clients must adhere to the defined messaging protocol  
- The server is the **only authority** on game state  
- Clients never communicate directly with one another  
- Movements and interactions must follow the assignment specifications  

---

## Architecture Overview

### Block Diagram

<img width="474" height="208" alt="Captura de ecrã 2025-12-14, às 16 59 10" src="https://github.com/user-attachments/assets/228c2fcb-2e88-4c6d-a78e-66df763c7008" />



<img width="410" height="249" alt="Captura de ecrã 2025-12-14, às 16 59 39" src="https://github.com/user-attachments/assets/7f637a03-73d9-4e6f-a29e-6f4abc0480b5" />

<img width="490" height="241" alt="Captura de ecrã 2025-12-14, às 16 59 27" src="https://github.com/user-attachments/assets/f00790eb-61f6-41bf-a6a1-183a2993ed8b" />


The server:
- Maintains the full board state  
- Processes movement commands  
- Broadcasts game updates to all connected clients  

---

## Build Instructions

Each component (Server, Lizards, Roaches, Wasps) includes its own Makefile.

### Compile everything
Inside each folder, run the following command to create server, lizard, roach, wasp executables:

```sh
make
```
---

## Running the System

- ./server
- ./lizard <server_ip> 5555 5558
- ./roach <server_ip> 5557
- ./wasp <server_ip> 5557

Note: Replace <server_ip> with the actual IP address of the machine running the server
