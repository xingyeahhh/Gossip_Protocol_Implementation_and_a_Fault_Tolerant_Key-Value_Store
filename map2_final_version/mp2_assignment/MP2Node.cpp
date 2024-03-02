/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
	//use as an iterator,especially used for iteratoring the whole transmap,
	//that start point is begin() 
	map<int, transaction*>::iterator it = transMap.begin();
	//iterate whole the transmap, and delete the second part,value part,because it is pointer! 
	while(it != transMap.end()){
		delete it->second;
		it++;
	}
}


/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();
	//Node represents the present time node
	//Node(addr) 这种方式是 C++ 语言提供的
	Node myself(this->memberNode->addr);//this is used for emphasizing now
	curMemList.push_back(myself);

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode

	//curMemList is used to construct the ring. Specifically, by sorting the list of members, the nodes are 
	//arranged in the order of their hash codes, effectively creating a ring. The ring variable is then used 
	//to store information about this constructed ring for subsequent operations. Hence, the relationship 
	//between them is that curMemList is used to construct ring
	sort(curMemList.begin(), curMemList.end());
	//check the current membership is the same as ring

	//确保每一项和顺序都是相同的
	if(ring.size() != curMemList.size()){
		change = true;
	}else if(ring.size() != 0){
		for(int i = 0; i < ring.size(); i++){
			if(curMemList[i].getHashCode() != ring[i].getHashCode()){
				change = true;
				break;
			}
		}
	}

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	//let ring use the latest curMemList!!
	ring = curMemList;
	if(change){
		stabilizationProtocol();
	}	
	//stabilizationProtocol() 是一个函数，其功能是运行稳定协议（stabilization protocol）。
	//在分布式系统中，稳定协议通常用于维护系统的一致性和稳定性。
	//1.成员管理，2.数据复制和一致性，3.环维护，4.其他一致性操作
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		//copy its address to the first 4
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		//copy its port to the 5th and 6th
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	//to calculate the hash value of this string
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;// so it is between 0-511
	//RING_SIZE is in stdincludes.h, a certain number, is used for build a ring and
	//evenly set 
	//hash number can be very large and it is certain a integer number.
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	//虽然存在多个副本，但它们都对应着相同的键（key），并且应该包含相同的值（value），
	//因此并不会导致多个不同的键值对存储在系统中。相反，副本的存在提高了系统的可用性和容错性。
	//Message msg = constructMsg(MessageType::CREATE, key, value);
	//string data = msg.toString();

	vector<Node> replicas = findNodes(key);
	for (int i =0; i < replicas.size(); i++) {
		Message msg = constructMsg(MessageType::CREATE, key, value);
		// cout << "client create trans_id :" << msg.transID << " ; address : "<< memberNode->addr.getAddress() << endl;
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), data);
		//g_transID ++;
	}
	g_transID ++;
	
	//??对于每个副本发送相同的消息是一个常见的做法，
	//因为它确保了在系统中的所有副本节点上执行相同的操作，从而保持数据的一致性。
	//g_transID is used for tracking in the distributed system

}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	//Message msg = constructMsg(MessageType::READ, key);
	//string data = msg.toString();

	vector<Node> replicas = findNodes(key);
	for (int i =0; i < replicas.size(); i++) {
		Message msg = constructMsg(MessageType::READ, key);
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), data);
		//g_transID ++;
	}
	g_transID ++;
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	//Message msg = constructMsg(MessageType::UPDATE, key, value);
	//string data = msg.toString();

	vector<Node> replicas = findNodes(key);
	for (int i =0; i < replicas.size(); i++) {
		Message msg = constructMsg(MessageType::UPDATE, key, value);
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), data);
		//g_transID ++;
	}
	g_transID ++;
	
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	//Message msg = constructMsg(MessageType::DELETE, key);
	//string data = msg.toString();

	vector<Node> replicas = findNodes(key);
	for (int i =0; i < replicas.size(); i++) {
		Message msg = constructMsg(MessageType::DELETE, key);
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), data);
		//g_transID ++;
	}
	g_transID ++;
}

Message MP2Node::constructMsg(MessageType mType, string key, string value, bool success){
	int trans_id = g_transID;
	createTransaction(trans_id, mType, key, value);
	if(mType == CREATE || mType == UPDATE){
		Message msg(trans_id, this->memberNode->addr, mType, key, value);
		//message is a constructor class wrote in message class
		return msg;
	}else if(mType == READ || mType == DELETE){
		Message msg(trans_id, this->memberNode->addr, mType, key);
		return msg;
	}else{
		assert(1!=1); // for debug
	}
}

void MP2Node::createTransaction(int trans_id, MessageType mType, string key, string value){
	int timestamp = this->par->getcurrtime();
	transaction* t = new transaction(trans_id, timestamp, mType, key, value);
	this->transMap.emplace(trans_id, t);
}

//使用了 new 来动态分配内存并创建 transaction 类的对象实例。
//在 new 后面直接跟随着调用 transaction 类的构造函数，
//这样就会在分配的内存上构造一个 transaction 对象，并使用提供的参数对对象的成员变量进行初始化。

// Constructor of transaction
transaction::transaction(int trans_id, int timestamp, MessageType mType, string key, string value){
	this->id = trans_id;
	this->timestamp = timestamp;
	this->replyCount = 0;
	this->successCount = 0;
	this->mType = mType;
	this->key = key;
	this->value = value;
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
//这段代码是一个服务器端的创建键值对的API实现。让我来解释一下它的功能：

//如果当前节点的事务ID不等于STABLE（即事务ID不稳定），则表示当前节点正在处理一个事务。
//在这种情况下，函数尝试将键值对插入本地哈希表（HashTable）。如果插入成功，则将成功的
//日志记录到日志中，否则记录失败的日志。这里的日志记录函数分别是 logCreateSuccess 和 logCreateFail。

//如果当前节点的事务ID等于STABLE（即事务ID稳定），则表示当前节点没有正在处理的事务。
//在这种情况下，函数首先尝试从本地哈希表中读取给定键的值。如果读取的内容为空，则说明该
//键在本地哈希表中不存在，因此可以安全地创建新的键值对，并将其插入到哈希表中。如果键已
//经存在，则不执行任何操作。

//函数最后返回一个布尔值，表示创建键值对的操作是否成功。如果成功，则返回 true，否则返回 false。

//**ht代表的是hashtable**非常重要
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica, int transID) {
	bool success = false;
	if(transID != STABLE){
		success = this->ht->create(key, value);
		if(success)
			log->logCreateSuccess(&memberNode->addr, false, transID, key, value);
		else 
			log->logCreateFail(&memberNode->addr, false, transID, key, value);	
	}else{
		string content = this->ht->read(key);
		bool exist = (content != "");
		if(!exist){
			success = this->ht->create(key, value);	
		}
	}
	return success;
	// Insert key, value, replicaType into the hash table
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key, int transID) {
	string content = this->ht->read(key);
	bool success = (content!="");
	if (success) {
		log->logReadSuccess(&memberNode->addr, false, transID, key, content);
	}else {
		log->logReadFail(&memberNode->addr, false, transID, key);
	}

	return content;
	// Read key from local hash table and return value
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica, int transID) {
	bool success = this->ht->update(key,value);
	if (success) {
		log->logUpdateSuccess(&memberNode->addr, false, transID, key, value);
	} else {
		log->logUpdateFail(&memberNode->addr, false, transID, key, value);
	}
	return success;
	// Update key in local hash table and return true or false
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key, int transID) {
	bool success = this->ht->deleteKey(key);
	if(transID != STABLE){
		if (success) {
			log->logDeleteSuccess(&memberNode->addr, false, transID, key);
		} else {
			log->logDeleteFail(&memberNode->addr, false, transID, key);
		}
	}
	return success;
	// Delete the key from the local hash table
}

//这个函数的目的是根据原始消息的类型，发送相应类型的回复消息，以响应原始消息的请求。
//条件表达式确定了回复消息的类型，从而保证了发送的是正确类型的回复消息。
void MP2Node::sendreply(string key, MessageType mType, bool success, Address* fromaddr, int transID, string content) {
	MessageType replyType = (mType == MessageType::READ)? MessageType::READREPLY: MessageType::REPLY;
	
	if(replyType == MessageType::READREPLY){
		Message msg(transID, this->memberNode->addr, content);
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, fromaddr, data);	
	}else{
		// MessageType::REPLY
		Message msg(transID, this->memberNode->addr, replyType, success);
		string data = msg.toString();
		emulNet->ENsend(&memberNode->addr, fromaddr, data);
	}	
}



/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	//mp2q 是 Member 类中的一个成员变量，用于存储 MP2 协议中接收到的消息队列。
	//这个变量通常在模拟节点中维护，用于暂时存储从网络中接收到的消息，然后按照一定的顺序逐个处理。

	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		//使用 char* 类型的指针来处理消息内容主要是因为在底层的网络通信中，消息通常是以字节流的形式传输的，
		//而 char* 类型的指针可以直接指向字节流的起始位置，并且可以更方便地对字节流进行处理。
		//q_elt本身是一个void *elt;和int size;，任意指针和长度值
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();
		//这是C++标准库提供的构造函数形式之一。这个构造函数接受两个迭代器作为参数，用来指定要复制的范围。
		//在这里，data 是指向字符串的起始位置的指针，data + size 是指向字符串末尾位置的指针。
		//因此，这个构造函数将从 data 开始的 size 个字符复制到 message 中，从而创建了一个新的字符串对象。
		string message(data, data + size);

		/*
		 * Handle the message types here
		 */
		//watch message class,has its explanation
		Message msg(message);


		switch(msg.type){
			case MessageType::CREATE:{
				bool success = createKeyValue(msg.key, msg.value, msg.replica, msg.transID);
				if (msg.transID != STABLE) {
					sendreply(msg.key, msg.type, success, &msg.fromAddr, msg.transID);
				}
				break;
			}
			case MessageType::DELETE:{
				bool success = deletekey(msg.key, msg.transID);
				if (msg.transID != STABLE){
					sendreply(msg.key, msg.type, success, &msg.fromAddr, msg.transID);
				}
				break;
			}
			case MessageType::READ:{
				string content = readKey(msg.key, msg.transID);
				bool success = !content.empty();
				sendreply(msg.key, msg.type, success, &msg.fromAddr, msg.transID, content);
				break;
			}
			case MessageType::UPDATE:{
				bool success = updateKeyValue(msg.key, msg.value, msg.replica, msg.transID);
				sendreply(msg.key, msg.type, success, &msg.fromAddr, msg.transID);
				break;
			}

			//MessageType::READREPLY：
			//当收到的消息类型为 READREPLY 时，表示收到了读操作的回复消息。
			//首先，根据消息中的事务ID查找对应的事务对象（transaction），这个事务ID应该是之前发起的读操作的ID。
			//如果找到了对应的事务对象，则将其回复计数 replyCount 加一，并将消息中的值赋给事务对象的 value 属性，表示读取到的内容。
			//判断消息中的值是否非空，若非空则将成功计数 successCount 加一。
			case MessageType::READREPLY:{
				map<int, transaction*>::iterator it = transMap.find(msg.transID);
				if(it == transMap.end())
					break;
				transaction* t = transMap[msg.transID];
				t->replyCount ++;
				t->value = msg.value; // content 
				bool success = (msg.value != "");
				
				if(success) {
					t->successCount ++;
				}	
				break;
			}

			//MessageType::REPLY：
			//当收到的消息类型为 REPLY 时，表示收到了一个普通的回复消息（可能是创建、更新、删除操作的回复）。
			//同样地，首先根据消息中的事务ID查找对应的事务对象。
			//如果找到了对应的事务对象，则将其回复计数 replyCount 加一，并根据消息中的成功标志 msg.success 
			//判断是否将成功计数 successCount 加一。
			case MessageType::REPLY:{
				map<int, transaction*>::iterator it = transMap.find(msg.transID);
				if(it == transMap.end()){
					break;
				}
				
				transaction* t = transMap[msg.transID];
				t->replyCount ++;
				if(msg.success)
					t->successCount ++;
				break;
			}
		}
		//当一个节点发出读请求后，其他节点回复该请求并返回所请求的值时，这个节点会接收到读回复（READREPLY）消息。
		//因此，在这种情况下，节点接收到的消息类型为读回复（READREPLY）

		//节点发送读请求后，接收到读回复后不会再次发送读请求。读回复消息是用来响应先前发送的读请求的，一旦节点收到了预期的响应，
		//它就不会再发送新的读请求。这样可以确保读操作不会陷入无限循环。
		
		checkTransMap();

	}
	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}


//在提到 fault-tolerance 和 quorum consistency level 的描述中，确实提到了每个键会被复制
//到三个连续的节点中，并且需要至少两个副本来实现 quorum consistency level。
void MP2Node::checkTransMap(){
	map<int, transaction*>::iterator it = transMap.begin();
	while (it != transMap.end()){
		if(it->second->replyCount == 3) {
			if(it->second->successCount >= 2) {
				logOperation(it->second, true, true, it->first);
			}else{
				logOperation(it->second, true, false, it->first); 
			}
			delete it->second;
			it = transMap.erase(it);
			continue;
		}else {
			if(it->second->successCount == 2) {
				logOperation(it->second, true, true, it->first);
				transComplete.emplace(it->first, true);
				delete it->second;
				it = transMap.erase(it);
				continue;
			}
			
			if(it->second->replyCount - it->second->successCount == 2) {
				logOperation(it->second, true, false, it->first);
				transComplete.emplace(it->first, false);
				delete it->second;
				it = transMap.erase(it);
				continue;
			}
		}
		//reply count 表示已经收到了两个回复，而 success count 表示其中有一个回复表示成功。
		//这可能意味着其中一个副本节点成功执行了操作，但另一个副本节点可能因为某种原因无法执
		//行或者执行失败。系统可能会根据具体情况采取不同的策略，比如等待更多的回复，重新尝试
		//操作，或者根据应用需求来决定如何处理这种情况。

		//如果系统在等待一定时间后仍然没有达到所需的 quorum，那么系统可以选择进行结算，然后
		//根据具体情况采取相应的措施，比如记录失败的操作、尝试重新执行操作、或者进行其他错误处理。
		
		// time limit 
		if(this->par->getcurrtime() - it->second->getTime() > 10) {
				logOperation(it->second, true, false, it->first);
				transComplete.emplace(it->first, false);
				delete it->second;
				it = transMap.erase(it);
				continue;
		}

		it++;
	}	
}

void MP2Node::logOperation(transaction* t, bool isCoordinator, bool success, int transID) {
	switch (t->mType) {
		case CREATE: {
			if (success) {
				log->logCreateSuccess(&memberNode->addr, isCoordinator, transID, t->key, t->value);
			} else {
				log->logCreateFail(&memberNode->addr, isCoordinator, transID, t->key, t->value);
			}
			break;
		}
			
		case READ: {
			if (success) {
				log->logReadSuccess(&memberNode->addr, isCoordinator, transID, t->key, t->value);
			} else {
				log->logReadFail(&memberNode->addr, isCoordinator, transID, t->key);
			}
			break;
		}
			
		case UPDATE: {
			if (success) {
				log->logUpdateSuccess(&memberNode->addr, isCoordinator, transID, t->key, t->value);
			} else {
				log->logUpdateFail(&memberNode->addr, isCoordinator, transID, t->key, t->value);
			}
			break;
		}
			
		case DELETE: {
			if (success) {
				log->logDeleteSuccess(&memberNode->addr, isCoordinator, transID, t->key);
			} else {
				log->logDeleteFail(&memberNode->addr, isCoordinator, transID, t->key);
			}
			break;
		}
	}
}

//logOperation: 这个函数用于记录各种操作（CREATE、READ、UPDATE、DELETE）的执行结果，
//包括成功和失败。它根据操作类型调用相应的日志记录方法（logCreateSuccess、logCreateFail、
//logReadSuccess、logReadFail等），以记录操作的成功或失败情况。它是一个通用的操作执行结果
//记录函数，用于记录事务的执行情况。

//logReadSuccess: 这个函数是专门用于记录读操作成功的情况。它记录了读操作成功时的相关信息，包
//括节点地址、是否为协调器、事务ID、操作的键和值等。与logOperation相比，它更专注于读操作的成功
//情况，提供了读操作成功时特定的日志记录。




/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	map<string, string>::iterator it;
	for(it = this->ht->hashTable.begin(); it != this->ht->hashTable.end(); it++) {
		string key = it->first;
		string value = it->second;
		vector<Node> replicas = findNodes(key);
		for (int i = 0; i < replicas.size(); i++) {
			// create
			Message createMsg(STABLE, this->memberNode->addr, MessageType::CREATE, key, value);
			string createData = createMsg.toString();
			emulNet->ENsend(&memberNode->addr, replicas[i].getAddress(), createData);
		}
	}
}
