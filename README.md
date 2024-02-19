## What is the project?
The project is about implementing a Gossip Protocol.
The main functionalities of the project :
Introduction : Each new peer contacts a well-known peer (the introducer) to join the group.

Membership : A membership protocol satisfies completeness all the time (for joins and failures), and accuracy when there are no message delays or losses (high accuracy when there are losses or delays).


## Detail & Principle :
Data Structure of Message :

typedef struct MessageHdr {
	enum MsgTypes msgType; 
	vector< MemberListEntry> member_vector; // membership list of source
	Address* addr; // the source of this message
}MessageHdr;

![image](https://github.com/xingyeahhh/Gossip-Protocol/assets/123461462/95a69e54-58b2-4c60-badc-04ef81d25b9b)
