#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include "headers.h"
using namespace std;

// Forward declarations
class Transaction; 
class Block; 
class Blockchain; 
class Link; 
class Event; 
class SendTransaction; 
class ReceiveTransaction; 
class BroadcastTransaction; 
class ReceiveBlock; 
class ForwardBlock; 
class BroadcastMinedBlock; 
class Peer; 
class Simulator; 

// ======================================================================= //
class Transaction {
public:
	// units = KB
	int size = 1;
};

// ======================================================================= //
class Block {
public:
	// units = KB
	int size;
	int max_size = 1000;
	vector<Block*> next;
	Block* parent;
};

// ======================================================================= //
class Blockchain {
public:
	int size;
	Block* genesis;
	void add(Block* parent, Block* b);
};

// ======================================================================= //
class Link {
	ld ro, c, d;
	Peer* peer;
	ld get_delay();
};

// ======================================================================= //
class Event {
public:
	int timestamp;
	virtual void run();
	bool operator<(const Event& other);
};

// ======================================================================= //
class SendTransaction : Event {
	void run();
};

// ======================================================================= //
class ReceiveTransaction : Event {
	void run();
};

// ======================================================================= //
class BroadcastTransaction : Event {
	void run();
};

// ======================================================================= //
class ReceiveBlock : Event {
	void run();
};

// ======================================================================= //
class ForwardBlock : Event {
	void run();
};

// ======================================================================= //
class BroadcastMinedBlock : Event {
	void run();
};

// ======================================================================= //
class Peer {
	static vector<Peer> peers;
private:
	vector<int> balances;
};

// ======================================================================= //
class Simulator {
public:
	set<Event> events;
	void init_events();
	void start();
};

#endif
