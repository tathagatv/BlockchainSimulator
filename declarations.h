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
#define MINING_FEE 50

// Forward declarations
class Transaction; 
class Block; 
class Blockchain; 
class Link; 
class Event; 
class GenerateTransaction;
class ForwardTransaction; 
class ReceiveTransaction; 
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
	ld timestamp;
	Peer* sender;
	Peer* receiver;

	Transaction(ld timestamp_, Peer* sender_, Peer* receiver_, int coins);
	string get_name();
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
	Peer* owner;

	Block(Block* parent_, Peer* owner_);
	void add(Transaction* txn);
	string get_name();
};

// ======================================================================= //
class Blockchain {
public:
	Block* genesis;
	Block* current_block;
	Blockchain(Peer* p);

	void add(Block* b);
	static Block* backward(Block* b, bool collect_txn, vector<int>& balances, vector<Transaction*>& txns);
};

// ======================================================================= //
class Link {
public:
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

	virtual void run(Simulator* sim, bool verbose);
	bool operator<(const Event& other);
	Event(ld timestamp_);
	virtual ~Event() {}
};

// ======================================================================= //
class ForwardTransaction : public Event {
public:
	Peer* peer; // peer who'll forward the transaction
	Peer* source; // peer who sent the transaction (won't forward to this peer)
	Transaction* txn; // transaction
	
	ForwardTransaction(ld timestamp, Peer* peer_, Peer* source_, Transaction* txn);
	void run(Simulator* sim, bool verbose);
};

// ======================================================================= //
class ReceiveTransaction : public Event {
public: 
	Peer* sender;
	Peer* receiver; // peer who receives the transaction
	Transaction* txn; // transaction
	
	void run(Simulator* sim, bool verbose);
	ReceiveTransaction(ld timestamp, Peer* sender_, Peer* receiver_, Transaction* txn);
};

// ======================================================================= //
class GenerateTransaction : public Event {
public:
	Peer* payed_by;

	void run(Simulator* sim, bool verbose);
	GenerateTransaction(ld timestamp, Peer* p);
};

// ======================================================================= //
class ReceiveBlock : public Event {
public:
	Peer* sender;
	Peer* receiver; // peer who receives the block
	Block* block; // block

	void run(Simulator* sim, bool verbose);
	ReceiveBlock(ld timestamp, Peer* sender_, Peer* receiver_, Block* block);
};

// ======================================================================= //
class ForwardBlock : public Event {
public:
	Peer* peer; // peer who'll forward the block
	Peer* source; // peer who sent the block (won't forward to this peer)
	Block* block; // block
	
	ForwardBlock(ld timestamp, Peer* peer_, Peer* source_, Block* block);
	void run(Simulator* sim, bool verbose);
};

// ======================================================================= //
class BroadcastMinedBlock : public Event {
public:
	Peer* owner;

	BroadcastMinedBlock(ld timestamp, Peer* p);
	void run(Simulator* sim, bool verbose);
};

// ====================================================================== //
struct TxnPtrComp {
	bool operator()(const Transaction* lhs, const Transaction* rhs) const {
		if (lhs->timestamp != rhs->timestamp)
			return lhs->timestamp < rhs->timestamp;
		else
			return lhs < rhs;
	}
};
// ======================================================================= //
class Peer {
public:
	static int counter;
	static int total_peers;
	static ld Ttx; // Ttx: mean interarrival time b/n txns generated by peer
	static ld Tk; // Ttx: mean mining time of a block

	exponential_distribution<ld> txn_interarrival_time, block_mining_time;
	uniform_int_distribution<int> unif_dist_peer;
	int id;
	bool is_fast;
	vector<int> balances;
	vector<Link> adj;
	
	set<int> recv_pool; // all transaction ids received
	set<Transaction*, TxnPtrComp> txn_pool; // transactions not yet mined

	Blockchain* blockchain; 
	BroadcastMinedBlock* next_mining_event;
	Block* next_mining_block;

	set<Block*> chain_blocks, free_blocks;
	map<Block*, vector<Block*>> free_block_parents;

	Peer();
	static void add_edge(Peer* a, Peer* b);
	string get_name();
	void add_block(Block* block, bool update_balances);

	void schedule_next_transaction(Simulator* sim);
	Transaction* generate_transaction(Simulator* sim); // generate transaction for this peer
	void forward_transaction(Simulator* sim, Peer* source, Transaction* txn); 
	void receive_transaction(Simulator* sim, Peer* sender, Transaction* txn);

	bool validate_txn(Transaction* txn, vector<int>& custom_balances);
	bool validate_block(Block* block, vector<int>& custom_balances);
	Block* generate_new_block(Simulator* sim);
	void schedule_next_block(Simulator* sim);

	void forward_block(Simulator* sim, Peer* source, Block* block); 
	void receive_block(Simulator* sim, Peer* sender, Block* block);
};

// ====================================================================== //
struct EventPtrComp {
	bool operator()(const Event* lhs, const Event* rhs) const {
		if (lhs->timestamp != rhs->timestamp)
			return lhs->timestamp < rhs->timestamp;
		else
			return lhs < rhs;
	}
};

// ======================================================================= //
class Simulator {
public:
	int n, slow_peers, edges;
	ld Tk, Ttx;
	ld current_timestamp;
	Event* current_event;
	set<Event*, EventPtrComp> events;
	vector<Peer> peers;

	Simulator(int n_, ld z_, ld Ttx_, ld Tk_, int edges_);	
	void get_new_peers();
	void form_random_network();
	void init_events();
	void add_event(Event* event);
	void delete_event(Event* event);
	void run(ld end_time, bool verbose_, int max_txns_, int max_blocks_);
	void log_time(ostream& os);
};

#endif
