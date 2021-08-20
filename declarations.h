#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include "headers.h"
using namespace std;

extern mt19937 rng;
extern mt19937_64 rng64;
#define random_shuffle(v) shuffle((v).begin(), (v).end(), rng);
// Use mt19937_64 for 64 bit random numbers.

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
	static int counter;
	// units = KB
	int size, id, amount;
	Peer *sender, *receiver;
	Transaction(Peer* a, Peer* b, int coins);
};

// ======================================================================= //
class Block {
public:
	static int max_size;
	static int counter;
	// units = KB
	int size, id, depth;
	vector<Transaction*> txns;
	vector<Block*> next;
	Block* parent;
};

// ======================================================================= //
class Blockchain {
public:
	Block* genesis;
	Blockchain();
	void add(Block* parent, Block* b);
};

// ======================================================================= //
class Link {
	default_random_engine generator;
	exponential_distribution<ld> exp;
	ld ro, c;
	Peer* peer;
	Link(Peer* p, bool is_fast);
	ld get_delay(int length);
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
public:
	static int counter;
	static vector<Peer> peers;
	int id;
	bool is_fast;
	vector<int> balances;
	Peer();
	static void get_new_peers(int n, int num_slow);
};

// ======================================================================= //
class Simulator {
public:
	int n, slow_peers;
	ld Tk, Ttx;
	Simulator(int n, ld z);	
	set<Event> events;
	void form_peer_network();
	void init_events();
	void start();
};

#endif
