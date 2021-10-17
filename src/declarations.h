#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include "headers.h"
using namespace std;

extern mt19937 rng;
extern mt19937_64 rng64;
#define random_shuffle(v) shuffle((v).begin(), (v).end(), rng64);
// Use mt19937_64 for 64 bit random numbers.

// constants
#define TRANSACTION_SIZE 1 // 1 KB	
#define MAX_BLOCK_SIZE 1000 // 1000 KB
#define START_TIME 0
#define MINING_FEE 50
#define LOW_HASH_POWER 0.1
#define HIGH_HASH_POWER 0.5
#define VERY_HIGH_HASH_POWER 1.0

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
class SelfishAttacker;
class StubbornAttacker;
class Simulator; 

// ======================================================================= //
class Transaction {
public:
	/* a static variable for unique txn id, incremented on every new txn */
	static int counter;
	/* id: unique txn id, amount: no of coins */
	int id, amount;
	/* timestamp at which txn was created */
	ld timestamp;
	/* coins transferred from sender to receiver */
	Peer* sender;
	Peer* receiver;

	Transaction(ld timestamp_, Peer* sender_, Peer* receiver_, int coins);
	string get_name();
};

// ======================================================================= //
class Block {
public:
	/* max allowed size for a block in KB */
	static int max_size;
	/* a static variable for unique block id, incremented on every new block */
	static int counter;
	/* size: size of this block, id: BlkID, depth: block_num, parent_id: id of parent block */
	int size, id, depth, parent_id;
	/* txs in this block */
	vector<Transaction*> txns;
	/* list of children blocks */
	vector<Block*> next;
	/* pointer to parent block */
	Block* parent;
	/* peer who mined this block */
	Peer* owner;

	Block(Peer* owner_);
	void add(Transaction* txn);
	string get_name();
	Block* clone();
	void set_parent(Block* b);
	void reset_parent();
	void set_id();
};

// ======================================================================= //
class Blockchain {
public:
	/* pointer to the genesis block */
	static Block* global_genesis;
	/* pointer to genesis block in this blockchain */
	Block* genesis;
	/* pointer to the last block in the blockchain */
	Block* current_block;
	
	Blockchain();
	void add(Block* b);
	static Block* backward(Block* b, vector<int>& balances, vector<Transaction*>& txns);
};

// ======================================================================= //
class Link {
public:
	/* exponential distribution to select queuing delay */
	exponential_distribution<ld> exp;
	/* ro: speed of light propagation delay, c: link rate */
	ld ro, c;
	/* peer to which this link is connecting to */
	Peer* peer;

	Link(Peer* p, bool is_fast);
	ld get_delay(int length);
};

// ======================================================================= //
class Event {
public:
	/* timestamp at which event occured/will occur */
	ld timestamp;
	/* bool to indicate if the event generates a new txn/block
	such events are ignored after simulation end time*/
	bool is_generate_type_event;

	virtual void run(Simulator* sim);
	bool operator<(const Event& other);
	Event(ld timestamp_);
	/* virtual destructor: required because we may delete derived class using base class pointer */
	virtual ~Event() {}
};

// ======================================================================= //
class ForwardTransaction : public Event {
public:
	Peer* peer; // peer who'll forward the transaction
	Peer* source; // peer who sent the transaction (won't forward to this peer)
	Transaction* txn; // transaction
	
	ForwardTransaction(ld timestamp, Peer* peer_, Peer* source_, Transaction* txn);
	void run(Simulator* sim);
};

// ======================================================================= //
class ReceiveTransaction : public Event {
public: 
	Peer* sender;
	Peer* receiver; // peer who receives the transaction
	Transaction* txn; // transaction
	
	void run(Simulator* sim);
	ReceiveTransaction(ld timestamp, Peer* sender_, Peer* receiver_, Transaction* txn);
};

// ======================================================================= //
class GenerateTransaction : public Event {
public:
	Peer* payed_by;

	void run(Simulator* sim);
	GenerateTransaction(ld timestamp, Peer* p);
};

// ======================================================================= //
class ReceiveBlock : public Event {
public:
	Peer* sender;
	Peer* receiver; // peer who receives the block
	Block* block; // block

	void run(Simulator* sim);
	ReceiveBlock(ld timestamp, Peer* sender_, Peer* receiver_, Block* block);
};

// ======================================================================= //
class ForwardBlock : public Event {
public:
	Peer* peer; // peer who'll forward the block
	Peer* source; // peer who sent the block (won't forward to this peer)
	Block* block; // block
	
	ForwardBlock(ld timestamp, Peer* peer_, Peer* source_, Block* block);
	void run(Simulator* sim);
};

// ======================================================================= //
class BroadcastMinedBlock : public Event {
public:
	Peer* owner;

	BroadcastMinedBlock(ld timestamp, Peer* p);
	void run(Simulator* sim);
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
	/* a static variable for unique peer id, incremented on every new peer */
	static int counter; 
	/* total no of peers in the network */
	static int total_peers; 
	/* Ttx: mean interarrival time b/n txns generated by peer */
	static ld Ttx; 
	/* Ttx: mean mining time of a block */
	static ld Tk; 
	/* fraction of the entire hash power */
	ld hash_power; 

	/* distributions to sample txn interarrival time and block mining time */
	exponential_distribution<ld> txn_interarrival_time, block_mining_time; 
	/* uniform distribution to select a peer */
	uniform_int_distribution<int> unif_dist_peer;
	/* uniform distribution to select a real no between 0 and 1 */
	uniform_real_distribution<ld> unif_rand_real;

	/* unique id for every block */
	int id;
	/* no of peers connected to this */
	int degree;
	/* is_fast: true indicates that this peer is labeled as fast */
	bool is_fast;
	/* balance[i] stores current balance for peer with id i */
	vector<int> balances;
	/* list of peers adjacent to this peer */
	vector<Link> adj;
	
	/* all transaction ids received so far */
	custom_unordered_set<int> recv_pool; 
	/* transactions not yet mined */
	set<Transaction*, TxnPtrComp> txn_pool; 

	/* local blockchain copy */
	Blockchain blockchain; 
	/* pointer to next mining event */
	BroadcastMinedBlock* next_mining_event;
	/* pointer to next mining block */
	Block* next_mining_block;

	/* blocks which are invalid */
	custom_unordered_set<int> rejected_blocks;
	/* chain_blocks: blocks in the blockchain, free_blocks: blocks not yet in blockchain */
	custom_map<int, Block*> chain_blocks, free_blocks;
	/* map from free_block_parents ids to pointer to free blocks */
	custom_map<int, vector<Block*>> free_block_parents;
	/* stores arrival times for each block */
	vector<pair<Block*, ld>> block_arrival_times;

	Peer();
	virtual ~Peer(){};
	static void add_edge(Peer* a, Peer* b, ostream& os);
	void initialize_block_mining_distribution(ld hash_power);
	int get_degree();
	string get_name();
	void add_block(Block* block, bool update_balances);

	void delete_invalid_free_blocks(Block* block, Simulator* sim);
	void free_blocks_dfs(Block* block, vector<int>& cur_balances, custom_unordered_set<Block*>& blocks_to_add, Block*& deepest_block, Simulator* sim);

	void schedule_next_transaction(Simulator* sim);
	Transaction* generate_transaction(Simulator* sim); 
	void forward_transaction(Simulator* sim, Peer* source, Transaction* txn); 
	void receive_transaction(Simulator* sim, Peer* sender, Transaction* txn);

	bool validate_txn(Transaction* txn, vector<int>& custom_balances);
	bool validate_block(Block* block, vector<int>& custom_balances);
	virtual Block* generate_new_block(Simulator* sim);
	void schedule_next_block(Simulator* sim);

	void forward_block(Simulator* sim, Peer* source, Block* block); 
	virtual void receive_block(Simulator* sim, Peer* sender, Block* block);

	virtual void broadcast_mined_block(Simulator* sim);

	void traverse_blockchain(Block* b, ostream& os, Block*& deepest_block, vector<int>& total_blocks);
	void export_blockchain(ostream& os);
	void export_arrival_times(ostream& os);
	void export_stats(Simulator* sim, ostream& os);
};

// ====================================================================== //
class SelfishAttacker : public Peer {
	Block* generate_new_block(Simulator* sim);
	void receive_block(Simulator* sim, Peer* sender, Block* block);
	void broadcast_mined_block(Simulator* sim);
};

class StubbornAttacker : public Peer {
	Block* generate_new_block(Simulator* sim);
	void receive_block(Simulator* sim, Peer* sender, Block* block);
	void broadcast_mined_block(Simulator* sim);
};

// ====================================================================== //
/* comparator for Event class pointer */
struct EventPtrComp {
	bool operator()(const Event* lhs, const Event* rhs) const {
		if (lhs->timestamp != rhs->timestamp)
			return lhs->timestamp < rhs->timestamp;
		else{
			// two different event pointers with same timestamp should not be considered as equal
			return lhs < rhs;
		}
	}
};

// ======================================================================= //
class Simulator {
public:
	/* n: no of peers, slow_peers: no of slow peers, edges: total no of edges in network */
	int n, slow_peers, edges;
	/* Tk: mean block inter arrival time, Ttx: mean txn interarrival time */
	ld Tk, Ttx;
	/* probabilities with which invalid blocks and invalid transactions are generated */
	ld invalid_txn_prob, invalid_block_prob;
	ld current_timestamp;
	/* event being processed currently */
	Event* current_event;
	/* stores events sorted as per timestamp */
	set<Event*, EventPtrComp> events;
	static vector<Peer*> peers;
	/* prints logs if true */
	bool verbose;
	/* true after end_time */
	bool has_simulation_ended;
	/* fraction of honest nodes the adversary is connected to */
	ld zeta;
	/* adversary type */
	string adversary;

	Simulator(int n_, ld z_, ld Ttx_, ld Tk_, int edges_, bool verbose_, ld invalid_txn_prob_, ld invalid_block_prob_, ld zeta_, string adversary_);
	~Simulator();
	void get_new_peers();
	void form_random_network(ostream& os);
	void init_events();
	void add_event(Event* event);
	void delete_event(Event* event);
	void run(ld end_time, int max_txns_, int max_blocks_);
	void log(ostream& os, const string& s);
	void complete_non_generate_events();
	void reset(const fs::path& dir_path);
};

#endif

