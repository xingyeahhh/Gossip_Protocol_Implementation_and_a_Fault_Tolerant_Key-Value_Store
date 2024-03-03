
# Gossip Protocol Implementation in Distributed Systems

## Introduction

This project implements a Gossip Protocol to manage membership in a distributed system. Each new peer initiates its entry into the system by contacting a well-known peer, the introducer, marking the beginning of its journey in the peer group.

## Core Functionalities

### 1. **Introduction Phase**
Every new member introduces itself to the network through a predefined introducer peer. This step is crucial for the peer to join the network and start participating in the gossip protocol for membership management.

### 2. **Membership Management**
The membership protocol ensures:
- **Completeness:** The system maintains a complete view of the membership, accounting for both joins and failures consistently.
- **Accuracy:** High accuracy in the membership view is maintained under ideal conditions, with allowances for slight variations in the presence of message delays or losses.

## Detailed Implementation & Principles

### Data Structure for Messages

```c
typedef struct MessageHdr {
    enum MsgTypes msgType;
    vector<MemberListEntry> member_vector; // Membership list from the source
    Address* addr; // Source address of the message
} MessageHdr;
```

### Heartbeat Mechanism
A fundamental aspect of our implementation is the heartbeat mechanism. Each node periodically sends heartbeat messages to a randomly selected subset of peers. These heartbeats serve as an indication of the node's liveliness and help in identifying failures within the system.

### Gossip Protocol Mechanics
The Gossip Protocol operates on the principle of randomized information dissemination. When a node receives a gossip message (heartbeat update), it updates its local membership list and forwards the message to a randomly selected set of peers. This ensures efficient and robust spread of membership information across the network, even in the face of failures or message losses.

## Implementation Highlights

- **File:** `MP1Node.cpp`
- **Description:** Contains the membership protocol executed by nodes. It includes functionality for joining the group, processing received messages, and performing periodic tasks related to the protocol.

## Key Functions

### `introduceSelfToGroup(Address *joinaddr)`
Handles the process of a new node introducing itself to the group. This is the first step for any node to join the distributed system.

### `void nodeLoopOps()`
Executes periodic tasks for a node, such as sending heartbeat messages and checking for node failures or timeouts, thereby maintaining the membership list's accuracy.

### `bool recvCallBack(void *env, char *data, int size)`
Processes received messages, categorizing them based on their type (e.g., JOINREQ, JOINREP, PING) and taking appropriate actions.

### Gossip-Based Membership List Propagation
The mechanism of spreading membership information through gossip ensures that the system is scalable and can efficiently manage membership information even as the network size grows.

## Conclusion

The Gossip Protocol implementation demonstrated in this project leverages heartbeat and gossip mechanisms to efficiently manage membership in a distributed system. It ensures robustness against failures and scalability, making it an ideal choice for large-scale distributed systems.

![image](https://github.com/xingyeahhh/Gossip-Protocol/assets/123461462/95a69e54-58b2-4c60-badc-04ef81d25b9b)


#  A Fault-Tolerant Key-Value Store in Distributed Systems

## Core Functionalities
- The hash table is used to store and manage local data on each node.
- The hash ring is used to determine the distribution of data among different nodes, enabling load balancing and data replication.
- CRUD operations : A key-value store supporting CRUD operations (Create, Read, Update, Delete).
- Load-balancing : via a consistent hashing ring to hash both servers and keys.
- Fault-tolerance up to two failures : by replicating each key three times to three successive nodes in the ring, starting from the first node at or to the clockwise of the hashed key.
- Quorum consistency level for both reads and writes (at least two replicas).
- Stabilization : after failure (recreate three replicas after failure).

## Key Functions
- ### `MP2Node::updateRing`
  This function should set up/update the virtual ring after it gets the updated membership list from MP1Node via an upcall.
- ### `MP2Node::clientCreate, MP2Node::clientRead, MP2Node::clientUpdate, MP2Node::clientDelete`
  These function should implement the client-side CRUD interfaces.
- ### `MP2Node::createKeyValue, MP2Node::readKey, MP2Node::updateKeyValue, MP2Node::deletekey`  
  These functions should implement the server-side CRUD interfaces.
- ### `MP2Node::stabilizationProtocol()`    
  This function should implement the stabilization protocol that ensures that there are always three replicas of every key in the key value store. 

## Principle of Fault-Tolerant Key-Value Store
![image](https://github.com/xingyeahhh/Gossip_Protocol_Implementation_and_a_Fault_Tolerant_Key-Value_Store/assets/123461462/37974d05-e9c4-4ebd-87d2-d7ea918c67eb)

















