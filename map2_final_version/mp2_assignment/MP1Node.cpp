/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 * 
 * DESCRIPTION: Join the distributed system
 * 1.检查是否为群组启动者
 * 首先，通过比较当前节点的地址和加入地址（joinaddr），检查当前节点是否是群组的启动者（也就是第一个加入群组的节点）。
   如果是，节点标记自己已经在群组中，并可能执行一些群组初始化的动作。

   2.构造加入请求消息
   如果当前节点不是群组的启动者，节点会构造一个加入请求（JOINREQ）消息。在新的实现方式中，使用new关键字动态创建
   一个MessageHdr类型的对象，msg，并设置消息类型为JOINREQ。

   将当前节点的成员列表（member_vector）赋给消息的member_vector字段。这样做的目的是为了快速同步成员信息，
   以及在一定程度上验证和优化网络状态。

   将当前节点的地址赋给消息的addr字段，这样接收方就知道这个加入请求是从哪个节点发起的。

   3.发送加入请求消息
   使用emulNet->ENsend函数将构造好的加入请求消息发送给加入地址（joinaddr）。
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        //size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        //msg = (MessageHdr *) malloc(msgsize * sizeof(char));
        //msg->msgType = JOINREQ;
        // write address behind the type
        //memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        // write heartbeat num behind the type
        //memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
        
        //******a new way********
        // create JOINREQ message: format of data is {struct Address myaddr}
        msg = new MessageHdr();
        msg->msgType = JOINREQ;
        msg->member_vector = memberNode->memberList;

        //add member_vector into the req is to quickly synchronize, with the same data,check and optmize the network
        //address information
        msg->addr = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, sizeof(MessageHdr));

        //free(msg);
        delete msg;
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 * 一个消息处理回调函数，用于处理分布式系统中节点收到的不同类型的消息。
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    MessageHdr* msg = (MessageHdr*)data;
    if (msg->msgType == JOINREQ) {
        //1.add to memberlist
        push_member_list(msg);
        //2.send JOINREP to source node
        Address* toaddr = msg->addr;
        // cout << "JOINREQ : from " << toaddr->getAddress() << " to " << memberNode->addr.getAddress() << endl; 
        send_message(toaddr, JOINREP);   
    }
    else if (msg->msgType == JOINREP) {
        // 1. add to memberlist
        // cout << "JOINREP : from " << msg->addr->getAddress() << " to " << memberNode->addr.getAddress() << endl;
        push_member_list(msg);
        // 2. set memberNode->inGroup = true
        memberNode->inGroup = true;
    }
    else if(msg->msgType==PING){
        // 1. Check memberlist
        // cout << "PING : from " << msg->addr->getAddress() << " to " << memberNode->addr.getAddress() << endl;
        ping_handler(msg);
    }
    delete msg;
    return true;
}

/**
 * FUNCTION NAME: push_member_list
 *
 * DESCRIPTION: If a node does not exist in the memberList, it will be pushed to the memberList.
 */

//在第一个函数中，主要处理的是接收到的加入请求（JOINREQ消息）。当一个新节点想要加入分布式系统时，
//它发送一个加入请求给其他节点
void MP1Node::push_member_list(MessageHdr* msg) {
    // id, port, heartbeat, timestamp
    int id = 0;
    short port;
    memcpy(&id, &msg->addr->addr[0], sizeof(int));
    memcpy(&port, &msg->addr->addr[4], sizeof(short));
    long heartbeat = 1;
    long timestamp = this->par->getcurrtime();
    if (check_member_list(id, port) != nullptr)
        return;
    MemberListEntry e(id, port, heartbeat, timestamp);
    memberNode->memberList.push_back(e);
    Address* added = get_address(id, port);
    log->logNodeAdd(&memberNode->addr, added);
    delete added;
}


//在第二个函数中，可能是在处理内部状态同步时调用的，
//例如，当一个节点从其他节点接收到包含成员列表的消息（可能是JOINREP，心跳消息或其他类型的同步消息）
void MP1Node::push_member_list(MemberListEntry* e) {
    // 首先检查成员列表中是否已存在该节点
    if (check_member_list(e->id, e->port) != nullptr) {
        // 如果已存在，根据需要更新现有条目或直接返回
        return;
    }

    Address* addr = get_address(e->id, e->port);

    if (*addr == memberNode->addr) {
        delete addr;
        return;
    }

    if (par->getcurrtime() - e->timestamp < TREMOVE) {
        log->logNodeAdd(&memberNode->addr, addr);
        MemberListEntry new_entry = *e;
        memberNode->memberList.push_back(new_entry);
    }
    delete addr;
}

/**
 * FUNCTION NAME: get_address
 *
 * DESCRIPTION: return address
 */
Address* MP1Node::get_address(int id, short port) {
    Address* address = new Address();
    memcpy(&address->addr[0], &id, sizeof(int));//从id的地址开始复制4个字节的数据到address->addr数组中
    memcpy(&address->addr[4], &port, sizeof(short));
    return address;
}

/**
 * FUNCTION NAME: check_member_list
 *
 * DESCRIPTION: If the node exists in the memberList, the function will return pointer. Otherwise, the function will return false.
 */
MemberListEntry* MP1Node::check_member_list(int id, short port) {
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        if (memberNode->memberList[i].id == id && memberNode->memberList[i].port == port)
            return &memberNode->memberList[i];//非另一个MemberNode实例
        //它是一个指向当前MemberNode实例的成员列表(memberList)中第i个元素的指针，这个元素是一个MemberListEntry对象。
        //MemberListEntry对象通常包含了分布式系统中某个节点的具体信息，如节点的ID、端口号、
        // 心跳值和最后一次更新的时间戳等。
    }
    return nullptr;
}

MemberListEntry* MP1Node::check_member_list(Address* node_addr) {
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        int id = 0;
        short port = 0;
        memcpy(&id, &node_addr->addr[0], sizeof(int));
        memcpy(&port, &node_addr->addr[4], sizeof(short));
        if (memberNode->memberList[i].id == id && memberNode->memberList[i].port == port)
            return &memberNode->memberList[i];
    }
    return nullptr;
}


//在代码中，MemberNode可能是一个类或结构体的实例，包含了节点自身的状态信息、网络地址、心跳计数器等。
//MemberListEntry 是分布式系统中节点用于记录其他节点信息的数据结构。每个MemberListEntry条目通常
// 包含了一个节点的标识符（如ID）、网络地址、心跳计数器和最后一次更新的时间戳等信息。


/**
 * FUNCTION NAME: send_message
 *
 * DESCRIPTION: send message
 */
void MP1Node::send_message(Address* toaddr, MsgTypes t) {
    //auto msg = std::make_unique<MessageHdr>(); // 使用智能指针自动管理内存
    MessageHdr* msg = new MessageHdr();
    //MessageHdr* msg = new MessageHdr();
    msg->msgType = t;
    msg->member_vector = memberNode->memberList;
    msg->addr = &memberNode->addr;
    emulNet->ENsend(&memberNode->addr, toaddr, (char*)msg, sizeof(MessageHdr));
    //通过EmulNet网络层发送消息。EmulNet是模拟网络环境的一个组件，
    //用于在节点间传递消息。这行代码的作用是将msg对象作为网络消息从当前节点发送到目的地址toaddr。
}
//这个操作改变的是系统的状态，具体来说，是节点A的状态。通过接收加入响应（JOINREP）消息，节点A得知自己已经被允许加入网络，
// 并可能获取了网络中其他节点的信息。这有助于节点A初始化自己的成员列表，开始与网络中的其他节点进行通信。


/**
 * FUNCTION NAME: ping_handler
 *ping_handler函数的核心目的是利用接收到的心跳消息更新本地的成员列表信息。它通过处理消息中携带的成员信息来：
    *确认发送心跳消息的节点仍然活跃。
    *更新已知节点的最新状态。
    *添加新发现的节点到本地成员列表。
    * 
    * 其他节点发送来的心跳信息，
    *   对于发送方，它发送心跳消息以通知其他节点自己仍然活跃。
        对于接收方（即当前的MemberNode），ping_handler函数是在处理收到的来自发送节点的心跳消息。

 * DESCRIPTION: The function handles the ping messages.
 */
void MP1Node::ping_handler(MessageHdr* msg) {
    update_src_member(msg);
    for (int i = 0; i < msg->member_vector.size(); i++) {
        // cout << " id : " << msg->member_vector[i].id << " , port : " << msg->member_vector[i].port << endl;
        if (msg->member_vector[i].id > 10 || msg->member_vector[i].id < 0) {
            assert(1 != 1);
        }
        MemberListEntry* node = check_member_list(msg->member_vector[i].id, msg->member_vector[i].port);
        //更新membernode中的每一个有的条目的心跳
        if (node != nullptr) {
            if (msg->member_vector[i].heartbeat > node->heartbeat) {
                node->heartbeat = msg->member_vector[i].heartbeat;
                node->timestamp = par->getcurrtime();
            }
        }//遍历心跳取最大的；
        else {
            push_member_list(&msg->member_vector[i]);
        }//如果没有当前节点，则现场加入即可；
    }
}


void MP1Node::update_src_member(MessageHdr* msg) {
    MemberListEntry* src_member = check_member_list(msg->addr);
    if (src_member != nullptr) {
        src_member->heartbeat++;
        src_member->timestamp = par->getcurrtime();
    }
    else {
        push_member_list(msg);
    }
}
//心跳数目的增加
//在update_src_member函数中，当一个已知的节点发送心跳消息到当前节点时，
// 确实会增加该节点在当前节点成员列表中记录的心跳计数（src_member->heartbeat++）。
//这里的心跳计数增加并不是因为收到心跳消息本身就自动增加，而是当前节点收到心跳消息后，手动更新这个计数，
// 作为对收到心跳的一种记录和响应。这反映了一个设计选择，即利用心跳消息来跟踪和更新节点的活跃状态。

//在这个update_src_member函数的上下文中，处理的焦点确实是直接针对发送心跳消息的节点，
// 而不是遍历并处理心跳消息携带的成员列表（msg->member_vector）。这个函数的目的是更新或添加发送心跳消息节
// 点的信息到当前节点的成员列表中，而不是处理心跳消息中附带的其他节点信息。

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
     * 表示活跃状态：通过定期增加自身的心跳计数，节点能够向其他节点证明它仍然是活跃的。
       这个计数随后会被包含在向其他节点发送的心跳消息中。
       故障检测：其他节点使用这个心跳计数来检测节点是否活跃。如果一个节点在一定时间内没有更
       新其心跳计数（即没有发送新的心跳消息），它可能会被视为失效，并从其他节点的成员列表中
       移除。
	 */
     // Update heartbeat
    memberNode->heartbeat++;
    // Check TREMOVE

    //循环遍历成员列表，检查每个节点的最后心跳时间戳。如果某个节点自上次接收到心跳以来的时间超
    // 过了TREMOVE定义的阈值，这意味着节点可能已经失败或离开了网络，因此需要将其从成员列表中
    // 移除。
    for (int i = memberNode->memberList.size() - 1; i >= 0; i--) {
        if (par->getcurrtime() - memberNode->memberList[i].timestamp >= TREMOVE) {
            Address* removed_addr = get_address(memberNode->memberList[i].id, memberNode->memberList[i].port);
            log->logNodeRemove(&memberNode->addr, removed_addr);
            memberNode->memberList.erase(memberNode->memberList.begin() + i);
            delete removed_addr;
        }
    }

    // Send PING to the members of memberList
    //再次遍历成员列表，对每个成员节点发送心跳消息。这是分布式系统中节点用来相互监测的机制。
    //通过发送心跳消息，节点可以告知网络中的其他节点它仍然活跃。
    for (int i = 0; i < memberNode->memberList.size(); i++) {
        Address* address = get_address(memberNode->memberList[i].id, memberNode->memberList[i].port);
        send_message(address, PING);
        delete address;
    }

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
