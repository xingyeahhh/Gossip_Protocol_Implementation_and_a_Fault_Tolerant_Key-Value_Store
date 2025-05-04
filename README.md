
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


```
1. Node Initialization & Joining Process
Initialization (initThisNode):
- Sets initial state (not in group, not failed)
- Initializes empty membership list

Joining Protocol (introduceSelfToGroup):
- First node becomes bootstrap node (marks itself in group)
- Subsequent nodes send JOINREQ to introducer
- JOINREQ contains sender's address and member list

Join Response Handling (JOINREP in recvCallBack):
- Merges received member list into local view
- Marks joining complete

2. Heartbeat Mechanism & Membership Maintenance
Periodic Operations (nodeLoopOps):
- Increments local heartbeat counter
- Checks for timed-out nodes (TREMOVE threshold)
- Broadcasts PING messages to all known members

Heartbeat Processing (ping_handler):
- Updates sender's heartbeat info
- Compares and merges remote member lists
- Adds newly discovered nodes

3. Failure Detection Approach
Timeout-Based:
- Tracks last update timestamp for each member
- Removes nodes exceeding TREMOVE threshold

Heartbeat-Based:
- Nodes periodically prove liveness via heartbeats
- Receivers only keep higher heartbeat values

4. Message Types
JOINREQ: Join group request
JOINREP: Join acknowledgment
PING: Periodic heartbeat containing member list

5. State Synchronization Method
Gossip-Style Propagation:
- Each PING carries complete membership info
- Receivers merge information through comparison

Eventual Consistency:
- Doesn't require immediate consistency
- Achieves convergence through periodic exchanges


6.Heart Beat:
Local Increment (in nodeLoopOps):  memberNode->heartbeat++;
- Each node maintains its own heartbeat counter
- Automatically increments periodically during each cycle
- Represents "I'm still alive" declaration

Remote Increment (in update_src_member):  src_member->heartbeat++;
- When Node A receives a PING from Node B
- Node A increments Node B's heartbeat counter

For Self Node:
Only maintains and increments its own heartbeat
Includes this value in outgoing PING messages

For Other Nodes:
When receiving a PING, increments the sender's heartbeat
Represents "I've received your latest status"

Practical Example Flow
Consider Node A and Node B:

Node A:
Locally increments heartbeat from 5→6 (nodeLoopOps)
Sends PING containing heartbeat=6

Node B receives PING:
Sees Node A's heartbeat is 6
Executes src_member->heartbeat++ (6→7)
This isn't duplicate - it means "I acknowledge your heartbeat 6, now recorded as 7"

Key Strengths
- Simplicity: Straightforward heartbeat/timeout detection
- Self-Contained: Full state in messages enables quick joining
- Decentralized: No single point of failure
```

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

```
I. Basic Architecture
Ring Topology: All nodes form a hash ring, with each node responsible for a segment of the ring.
Data Distribution: Uses consistent hashing to determine data placement.
Triple Replication: Each data entry is stored in 3 replicas across the ring.

-------updateRing()
vector<Node> curMemList = getMembershipList();  // 从MP1获取成员列表
sort(curMemList.begin(), curMemList.end());    // 按哈希值排序
ring = curMemList;  // 形成环形结构

------------hashFunction()
size_t MP2Node::hashFunction(string key) {
    std::hash<string> hashFunc;
    return hashFunc(key) % RING_SIZE;  // RING_SIZE通常设为512
}

------------findNodes()实现数据定位：
vector<Node> MP2Node::findNodes(string key) {
    size_t pos = hashFunction(key);
    vector<Node> addr_vec;
    // 特殊情况处理：哈希值在环的起点之前
    if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
       ...
    }
    else {
        // 常规情况：找到第一个哈希值大于key哈希的节点
      ...
        }
    }
    return addr_vec;
}

II. Key Workflows
!!Client Operations (API)

Create Data:
Compute the hash of the key to locate its position on the ring.
Identify the 3 nodes responsible for the key.
Send CREATE requests to all 3 nodes.

Read Data:
Locate the 3 replica nodes.
Send READ requests.
Use Quorum (2/3 nodes must agree) for consistency.

Update/Delete:
Similar flow, requiring majority confirmation.

-------clientCreate()函数
void MP2Node::clientCreate(string key, string value) {
    vector<Node> replicas = findNodes(key);
    for (int i=0; i<replicas.size(); i++) {
        Message msg = constructMsg(MessageType::CREATE, key, value);
        emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), msg.toString());
    }
    g_transID++;  // 全局事务ID递增
}

--------消息构造constructMsg()：
Message MP2Node::constructMsg(MessageType mType, string key, string value) {
    int trans_id = g_transID;
    createTransaction(trans_id, mType, key, value);  // 创建事务记录
    return Message(trans_id, this->memberNode->addr, mType, key, value);
}

-----clientRead()实现：
void MP2Node::clientRead(string key){
    vector<Node> replicas = findNodes(key);
    for (int i=0; i<replicas.size(); i++) {
        Message msg = constructMsg(MessageType::READ, key);
        emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), msg.toString());
    }
    g_transID++;
}

!!Server Processing
Handles CREATE/READ/UPDATE/DELETE requests.
Executes operations on the local hash table.
Returns results to the client.

--------checkMessages()核心逻辑
while (!memberNode->mp2q.empty()) {
    // 从队列取出消息
    data = (char *)memberNode->mp2q.front().elt;
    size = memberNode->mp2q.front().size;
    memberNode->mp2q.pop();
    
    Message msg(string(data, data + size));
    
    // 根据消息类型处理
    switch(msg.type) {
        case CREATE: {
            bool success = createKeyValue(msg.key, msg.value, msg.replica, msg.transID);
            sendReply(msg.key, msg.type, success, &msg.fromAddr, msg.transID);
            break;
        }
        // 其他操作类型处理...
    }
    checkTransMap();  // 检查事务状态
}

-----创建数据createKeyValue()
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica, int transID) {
    bool success = this->ht->create(key, value);
    if(success)
        log->logCreateSuccess(&memberNode->addr, false, transID, key, value);
    else 
        log->logCreateFail(&memberNode->addr, false, transID, key, value);
    return success;
}

---------读取数据readKey();
string MP2Node::readKey(string key, int transID) {
    string content = this->ht->read(key);
    bool success = (content != "");
    if (success) {
        log->logReadSuccess(&memberNode->addr, false, transID, key, content);
    } else {
        log->logReadFail(&memberNode->addr, false, transID, key);
    }
    return content;
}


III. Core Mechanisms
Consistency Guarantees:
Writes require ≥2 successful acknowledgments.
Reads require ≥2 matching responses.
Ensures data remains consistent even if 1 node fails.

Failure Recovery:
Periodic ring checks (stabilizationProtocol).
Re-replicates data when nodes join/leave.
Operation timeout (10 time units).

Transaction Management:
Unique transaction IDs for each operation.
Tracks responses and applies Quorum rules to determine final state.

----事务结构体定义：
struct transaction {
    int id;             // 事务ID
    int timestamp;      // 创建时间戳  
    int replyCount;     // 已收到回复数
    int successCount;   // 成功回复数
    MessageType mType;  // 操作类型
    string key;         // 操作键
    string value;       // 操作值或读取结果
};

-------事务检查checkTransMap()：
void MP2Node::checkTransMap(){
    auto it = transMap.begin();
    while (it != transMap.end()){
        // 情况1：收到全部3个回复
        if(it->second->replyCount == 3) {
            bool success = (it->second->successCount >= 2);
            logOperation(it->second, true, success, it->first);
            delete it->second;
            it = transMap.erase(it);
            continue;
        }
        // 情况2：已获得2个成功(提前判定成功)
        else if(it->second->successCount == 2) {
            logOperation(it->second, true, true, it->first);
            transComplete.emplace(it->first, true);
            delete it->second;
            it = transMap.erase(it);
            continue;
        }
        // 情况3：已确认2个失败(提前判定失败) 
        else if(it->second->replyCount - it->second->successCount == 2) {
            logOperation(it->second, true, false, it->first);
            transComplete.emplace(it->first, false);
            delete it->second;
            it = transMap.erase(it);
            continue;
        }
        // 情况4：超时处理
        else if(par->getcurrtime() - it->second->getTime() > 10) {
            logOperation(it->second, true, false, it->first);
            transComplete.emplace(it->first, false);
            delete it->second;
            it = transMap.erase(it);
            continue;
        }
        it++;
    }
}

--------stabilizationProtocol()实现：
void MP2Node::stabilizationProtocol() {
    for(auto it = this->ht->hashTable.begin(); it != this->ht->hashTable.end(); it++) {
        vector<Node> replicas = findNodes(it->first);
        for (int i = 0; i < replicas.size(); i++) {
            Message createMsg(STABLE, this->memberNode->addr, MessageType::CREATE, 
                            it->first, it->second);
            emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), createMsg.toString());
        }
    }
}

IV. Example Scenarios
Data Write:
Client hashes the key and finds the next 3 nodes.
Sends CREATE to all 3.
Confirms success after 2 acknowledgments.

Node Joining:
Triggers stabilizationProtocol to rebalance data.
Ensures 3 replicas for all keys.


```
![image](https://github.com/user-attachments/assets/2511fcd0-7118-46f7-bdd9-95ce7887af3b)

```
Roles and Responsibilities
Role	   Responsibilities	               Typical Behaviors	                                                               Code Representation
Client	   Initiates data operations	   Determines operation type (CREATE/READ/UPDATE/DELETE) and selects target nodes	   clientCreate(), clientRead() etc.
Server	   Executes actual data storage	   Responds to operation requests and manipulates local hash table	                   createKeyValue(), readKey() etc.

Key Interaction Workflows
When a node initiates an operation (as Client):
- Calls client APIs (e.g., clientCreate())
- Locates 3 target nodes (including self) via findNodes()
- Sends requests to all target nodes (even if itself is one)

When a node receives a request (as Server):
- Processes message queue via checkMessages()
- Executes corresponding data operation (e.g., createKeyValue())
- Returns operation result


    participant Client
    participant Replica1
    participant Replica2
    participant Replica3

    Client->>Replica1: CREATE(key,value) [transID=100]
    Client->>Replica2: CREATE(key,value) [transID=100]
    Client->>Replica3: CREATE(key,value) [transID=100]
    
    Replica1->>Client: REPLY(success, transID=100)
    Replica2->>Client: REPLY(success, transID=100)
    Replica3->>Client: REPLY(fail, transID=100) 
    
    Note over Client: 收到2个success → 判定操作成功
    Client->>Client: 记录日志: CREATE成功

MP2Node的核心设计思想。在任意节点上都可以发起CRUD操作，无需关心该节点是否存储目标数据。这是分布式键值存储系统的关键特性之一，具体实现原理如下：
(1) 无中心化设计
所有节点完全对等（P2P架构），没有主从之分
每个节点都具备完整的路由能力（通过findNodes()定位数据位置）

(2) 数据定位透明化
当您在节点A上发起操作时：
关键点：操作的发起节点不依赖本地数据，而是通过哈希环计算数据应该存在哪里

场景1：新增节点
数据迁移：
新节点加入后，某些key的findNodes()结果会包含该节点
原有节点通过stabilizationProtocol()将数据迁移到新节点

示例：
原环：N1(100), N2(200), N3(300)
加入N4(150)后：
Key=120的副本从[N2,N3,N1]变为[N4,N2,N3]
N1会将相关数据发送给N4

场景2：节点退出
数据补充：
退出节点的数据会由其他副本节点通过findNodes()重新分配
例如原副本节点A,B,C中B退出，新计算可能变为A,C,D
故障检测：
通过checkMessages()超时机制检测节点离线
最终由存活节点触发数据补充

关键设计细节
增量同步：
只迁移因节点变化导致归属变动的数据（通过findNodes()差异检测）
避免重复存储：
bool createKeyValue(key, value, replica, STABLE) {
    if (ht->read(key).empty()) { // 只有不存在时才写入
        return ht->create(key, value);
    }
    return false;
}

稳定标记：
使用STABLE作为特殊transID，表示这是后台同步操作
区别于客户端发起的常规操作

为什么MP2感觉像"框架"？
因为MP2做了以下教学简化：
单机模拟：用内存map模拟分布式存储
真实系统会使用磁盘存储（如LevelDB/RocksDB）
无持久化：进程退出后数据消失
简化网络：EmulNet模拟理想网络环境

1.MP2是"真实"存储：只是用std::map做了最小化实现
2.核心价值：演示了如何通过协议层(MP1)+存储层(MP2)构建分布式系统
3.生产级扩展：若要实用化，需要：
    替换HashTable为持久化存储
    增加虚拟节点逻辑
    实现更高效的同步协议
理解协议再实现存储
```
![image](https://github.com/user-attachments/assets/ed951530-e8e7-446a-85b6-f6a19662e49e)

## Conclusion
Each node in the system contains both a Membership Protocol (MP1Node) and a KV Store (MP2Node). The MP1Node manages node membership and fault detection by periodically using the Gossip protocol to exchange state information with other nodes, ensuring the system maintains an updated view of the cluster. The MP2Node is responsible for key-value storage, which is achieved through data sharding and replication to ensure high availability and fault tolerance. Data operations (put, get, delete) in MP2Node utilize consistent hashing to determine the appropriate nodes for storage and ensure data is replicated based on the defined replication factor. The MP1Node continuously provides the latest cluster state information to the MP2Node, ensuring data operations are directed to the correct nodes. This integration of MP1Node and MP2Node within each node ensures a robust, scalable, and fault-tolerant distributed key-value store.

## Principle of Fault-Tolerant Key-Value Store
![image](https://github.com/xingyeahhh/Gossip_Protocol_Implementation_and_a_Fault_Tolerant_Key-Value_Store/assets/123461462/37974d05-e9c4-4ebd-87d2-d7ea918c67eb)

![image](https://github.com/user-attachments/assets/9c4b21b9-9621-48fc-93c6-c86699bedc8e)
















