#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include "headers.h"
using namespace std;

extern mt19937 rng;
extern mt19937_64 rng64;
#define random_shuffle(v) shuffle((v).begin(), (v).end(), rng);
// Use mt19937_64 for 64 bit random numbers.

// constants
#define TRANSACTION_SIZE 1 // 1 KB
#define START_TIME 0

// Forward declarations
class Transaction; 
class Block; 
class Blockchain; 
class Link; 
class Event; 
class ForwardTransaction; 
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
	int id, amount;
	int senderId, receiverId;
	Transaction(int a, int b, int coins);
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
public:
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
	ld timestamp;
	virtual void run(Simulator* sim);
	bool operator<(const Event& other);
	Event(ld timestamp_);
};

// ======================================================================= //
class ForwardTransaction : public Event {
public:
	int peer_id; // peer who'll forward the transaction
	int source_id; // peer who sent the transaction (won't forward to this peer)
	Transaction* txn; // transaction
	void run(Simulator* sim);
	ForwardTransaction(ld timestamp, int peer_id, int source_id, Transaction *txn);
};

// ======================================================================= //
class ReceiveTransaction : public Event {
public: 
	int sender_id, receiver_id; // peer who receives the transaction
	Transaction* txn; // transaction
	void run(Simulator* sim);
	ReceiveTransaction(ld timestamp, int sender_id, int receiver_id, Transaction* txn);
};

// ======================================================================= //
class GenerateTransaction : public Event {
public:
	int peer_id;
	void run(Simulator* sim);
	GenerateTransaction(ld timestamp, int peer_id);
};

// ======================================================================= //
class ReceiveBlock : public Event {
	void run(Simulator* sim);
};

// ======================================================================= //
class ForwardBlock : public Event {
	void run(Simulator* sim);
};

// ======================================================================= //
class BroadcastMinedBlock : public Event {
	void run(Simulator* sim);
};

// ======================================================================= //
class Peer {
public:
	static int counter;
	static int total_peers;
	static ld Ttx; // Ttx: mean interarrival time b/n txns generated by peer

	exponential_distribution<ld> exp_dist_time;
	uniform_int_distribution<int> unif_dist_peer;
	int id;
	bool is_fast;
	vector<int> balances;
	vector<Link> adj;
	set<int> recv_pool; // all transaction ids received
	set<Transaction*> txn_pool; // transactions not yet mined

	Peer();
	static void add_edge(Peer* a, Peer* b);
	set<Event*> generate_next_transaction(ld cur_time);
	set<Event*> generate_transaction(ld cur_time); // generate transaction for this peer
	set<Event*> forward_transaction(ld cur_time, int source_id, Transaction* txn); 
	set<Event*> receive_transaction(ld cur_time, int sender_id, Transaction *txn);
};

// ====================================================================== //
struct EventPtrComp{
	bool operator()(const Event* lhs, const Event* rhs) const;
};

// ======================================================================= //
class Simulator {
public:
	int n, slow_peers, edges;
	ld Tk, Ttx;
	Simulator(int n, ld z);	
	set<Event*, EventPtrComp> events;
	vector<Peer> peers;

	void get_new_peers();
	void form_random_network();
	void init_events();
	void run(ld end_time);
};

#endif
