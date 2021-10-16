#include "declarations.h"
using namespace std;

int Peer::counter;
int Peer::total_peers;
ld Peer::Ttx;
ld Peer::Tk;
vector<Peer*> Simulator::peers;

/* constructor */
Peer::Peer() {
    // initializa balances of all peers with 0
    balances.assign(total_peers, 0);
    assert(Ttx > 0);
    assert(Tk > 0);
    degree = 0;
    // avg of expo dist = 1/lambda
    txn_interarrival_time = exponential_distribution<ld>(1.0 / Ttx);
    unif_dist_peer = uniform_int_distribution<int>(0, total_peers - 1);
    unif_rand_real = uniform_real_distribution<ld>(0, 1);

    blockchain = Blockchain();
    // add genesis block in chain_blocks
    chain_blocks[blockchain.current_block->id] = blockchain.current_block;

    block_arrival_times.emplace_back(make_pair(blockchain.genesis, 0));

    /* create hash power distribution: 50% with low hash power (0.1) 
        and remaining 50% with high hash power (0.5) */
    hash_power = (unif_rand_real(rng64) < 0.5) ? LOW_HASH_POWER : HIGH_HASH_POWER;
}

/* update hash power to normalized value and initialize block mining distribution */
void Peer::initialize_block_mining_distribution(ld hash_power){
    this->hash_power = hash_power;
    // more hash power: less Tk
    block_mining_time = exponential_distribution<ld>(hash_power / Tk);
}

/* returns the name of this peer */
string Peer::get_name() {
    return "Peer" + to_string(id + 1);
}

/* returns the degree for this peer */
int Peer::get_degree() {
    return degree;
}

/* creates a link between peer a and peer b */
void Peer::add_edge(Peer *a, Peer *b) {
    // increase the degree
    a->degree++;
    b->degree++;

    // add b in a's adj list
    Link ab(b, a->is_fast);
    (a->adj).emplace_back(ab);

    // add a in b's adj list
    Link ba(a, b->is_fast);
    (b->adj).emplace_back(ba);
}

// ================== TRANSACTION =============================
void Peer::schedule_next_transaction(Simulator* sim) {
    ld interArrivalTime = txn_interarrival_time(rng64);
    Event* ev = new GenerateTransaction(interArrivalTime, this);
    sim->add_event(ev);
}

Transaction* Peer::generate_transaction(Simulator* sim) {
    bool generate_invalid = (unif_rand_real(rng64) < sim->invalid_txn_prob);
    int coins = -1, cur_balance = balances[id];
    if (generate_invalid) {
        uniform_int_distribution unif_coins_dist(1, 100);
        coins = cur_balance + unif_coins_dist(rng64);
    } else if (cur_balance == 0) {
        coins = 0;
    } else {
        uniform_int_distribution unif_coins_dist(1, cur_balance);
        coins = unif_coins_dist(rng64);
    }
    Transaction* txn = NULL;

    if (coins > 0) {
        // todo: check if uniform distribution is correct for sampling receiver & no of coins
        int receiver = unif_dist_peer(rng64);
        while (receiver == id)
            receiver = unif_dist_peer(rng64);

        txn = new Transaction(sim->current_timestamp, this, sim->peers[receiver], coins);

        // todo: add transaction in transaction/recv pool
        recv_pool.insert(txn->id);
        txn_pool.insert(txn);

        // forward the transaction to peers
        Event* ev = new ForwardTransaction(0, this, this, txn);
        sim->add_event(ev);
    }
    // generate new transaction
    schedule_next_transaction(sim);
    return txn;
}

void Peer::forward_transaction(Simulator* sim, Peer* source, Transaction* txn) {
    // send transaction to peers
    for (Link& link : adj) {
        if (link.peer->id == source->id) continue;  // source already has the txn, loop-less forwarding
        ld delay = link.get_delay(TRANSACTION_SIZE);
        Event* ev = new ReceiveTransaction(delay, this, link.peer, txn);
        sim->add_event(ev);
    }
}

void Peer::receive_transaction(Simulator* sim, Peer* sender, Transaction *txn) {
    // if already received the transaction then ignore
    if (recv_pool.find(txn->id) != recv_pool.end())
        return;

    recv_pool.insert(txn->id);
    txn_pool.insert(txn);

    // forward the txn to other peers
    Event* ev = new ForwardTransaction(0, this, sender, txn);
    sim->add_event(ev);
}

bool Peer::validate_txn(Transaction* txn, vector<int>& custom_balances) {
    int balance = custom_balances[txn->sender->id];
    return (balance >= txn->amount);
}

// ================== BLOCK =============================
bool Peer::validate_block(Block* block, vector<int>& custom_balances) {
    if (block->size > Block::max_size)
        return false;
    vector<int> balances_copy = custom_balances;
    for (Transaction* txn : block->txns) {
        if (!validate_txn(txn, balances_copy))
            return false;
        balances_copy[txn->sender->id] -= txn->amount;
        balances_copy[txn->receiver->id] += txn->amount;
    }
    return true;
}

/* generate new block: equivalent of mining a block and add corresponding event */
Block* Peer::generate_new_block(Simulator* sim) {
    bool generate_invalid = (unif_rand_real(rng64) < sim->invalid_block_prob);
    Block* block = new Block(this);
    block->set_parent(blockchain.current_block);
    bool is_invalid = false;
    vector<int> balances_copy = balances;
    if (generate_invalid) {
        for (Transaction* txn : txn_pool) {
            if (block->size + TRANSACTION_SIZE > Block::max_size) {
                if (!is_invalid) {
                    block->add(txn);
                    is_invalid = true;
                }
                break;
            }
            if (!validate_txn(txn, balances_copy))
                is_invalid = true;
            balances_copy[txn->sender->id] -= txn->amount;
            balances_copy[txn->receiver->id] += txn->amount;
            block->add(txn);
        }
        return block;
    } 
    for (Transaction* txn : txn_pool) {
        if (block->size + TRANSACTION_SIZE > Block::max_size) 
            break;
        if (validate_txn(txn, balances_copy)) {
            block->add(txn);
            balances_copy[txn->sender->id] -= txn->amount;
            balances_copy[txn->receiver->id] += txn->amount;
        }
    }
    return block;
}

void Peer::schedule_next_block(Simulator* sim) {
    next_mining_block = generate_new_block(sim);
    ld miningTime = block_mining_time(rng64);
    next_mining_event = new BroadcastMinedBlock(miningTime, this);
    sim->add_event(next_mining_event);
}

void Peer::broadcast_mined_block(Simulator *sim){
    Block* block = next_mining_block;
	block->set_id();
	
	assert(blockchain.current_block->id == block->parent->id);
	bool is_valid = validate_block(block, balances);

	// do not add invalid block, only transmit it to other peers
	string validity = "INVALID";
	if (is_valid) {
		add_block(block, true);
		validity = "VALID";
	}
	sim->log(cout, get_name() + " mines and broadcasts " + validity + " block " + block->get_name());
    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));

	Event* ev = new ForwardBlock(0, this, this, block->clone());
	sim->add_event(ev);

	schedule_next_block(sim);
}

void Peer::forward_block(Simulator* sim, Peer* source, Block* block) {
    // send block to peers
    // this block is a copy, memory needs to be freed
    for (Link& link : adj) {
        if (link.peer->id == source->id) continue;  // source already has the blpck, loop-less forwarding
        Block* new_block = block->clone();
        ld delay = link.get_delay(new_block->size);
        Event* ev = new ReceiveBlock(delay, this, link.peer, new_block);
        sim->add_event(ev);
    }
    delete block;
}

/* add block to the blockchain and update balances */
void Peer::add_block(Block* block, bool update_balances) {
    blockchain.add(block);
    chain_blocks[block->id] = block;
    if (update_balances) {
        set<Transaction*>::iterator it;
        for (Transaction* txn : block->txns) {
            balances[txn->sender->id] -= txn->amount;
            balances[txn->receiver->id] += txn->amount;
            it = txn_pool.find(txn);
            if (it != txn_pool.end())
                txn_pool.erase(it);
        }
        balances[block->owner->id] += MINING_FEE;
        blockchain.current_block = block;
    }
}

void Peer::delete_invalid_free_blocks(Block* block, Simulator* sim) {
    custom_map<int, vector<Block*>>::iterator it;
    it = free_block_parents.find(block->id);
    
    // add block to rejected_blocks
    rejected_blocks.insert(block->id);
    sim->log(cout, get_name() + " REJECTS block " + block->get_name());
    
    // dont delete block to maintain block arrivals consistency
    // delete block;

    if (it == free_block_parents.end()) 
        return;

    // recursive call to delete child blocks
    for (Block* child : it->second) {
        assert(child->parent == NULL);
        child->set_parent(block);
        free_blocks.erase(child->id);
        delete_invalid_free_blocks(child, sim);
    }
    free_block_parents.erase(it);
}

void Peer::free_blocks_dfs(Block* block, vector<int>& cur_balances, custom_unordered_set<Block*>& blocks_to_add, Block*& deepest_block, Simulator* sim) {
    if (!validate_block(block, cur_balances)) {
        delete_invalid_free_blocks(block, sim);
        return;
    }

    blocks_to_add.insert(block);
    if (deepest_block == NULL || block->depth > deepest_block->depth)
        deepest_block = block;

    custom_map<int, vector<Block*>>::iterator it;
    it = free_block_parents.find(block->id);
    if (it == free_block_parents.end()) 
        return;

    // update balance array
    for (Transaction* txn : block->txns) {
        cur_balances[txn->sender->id] -= txn->amount;
        cur_balances[txn->receiver->id] += txn->amount;
    }
    cur_balances[block->owner->id] += MINING_FEE; 

    // recursive call to chld blocks
    for (Block* child : it->second) {
        assert(child->parent == NULL);
        child->set_parent(block);
        free_blocks.erase(child->id);
        free_blocks_dfs(child, cur_balances, blocks_to_add, deepest_block, sim);
    }
    free_block_parents.erase(it);

    // reset balance array
    for (Transaction* txn : block->txns) {
        cur_balances[txn->sender->id] += txn->amount;
        cur_balances[txn->receiver->id] -= txn->amount;
    }
    cur_balances[block->owner->id] -= MINING_FEE;
}

void Peer::receive_block(Simulator* sim, Peer* sender, Block* block) {
    custom_map<int, Block*>::iterator chain_it, free_it;
    custom_unordered_set<int>::iterator reject_it;

    chain_it = chain_blocks.find(block->id);
    free_it = free_blocks.find(block->id);
    reject_it = rejected_blocks.find(block->id);

    // already received this block
    if (chain_it != chain_blocks.end() || free_it != free_blocks.end() || reject_it != rejected_blocks.end()) 
        return;

    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));
    // forward every received block regardless of validity
    Event* ev = new ForwardBlock(0, this, sender, block->clone());
    sim->add_event(ev);
    
    chain_it = chain_blocks.find(block->parent_id);

    // block parent not in our blockchain
    if (chain_it == chain_blocks.end()) {
        block->reset_parent();
        free_blocks[block->id] = block;
        free_block_parents[block->parent_id].emplace_back(block);
        return;
    }

    block->set_parent(chain_it->second);

    Block* current_block = blockchain.current_block; // last block in the blockchain
    Block* branch_block = block->parent; // add the new block as a child of branch block

    // balances to update in case longest chain changes
    vector<int> current_balance_change(total_peers, 0); 
    // txns to add to the txn pool in case longest chain changes
    vector<Transaction*> txns_to_add; 
    // find lca
    while (current_block->depth > branch_block->depth)
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
    
    // balances to update in case longest chain changes
    vector<int> branch_balance_change(total_peers, 0);
    // txns to remove from the txn pool in case longest chain changes
    vector<Transaction*> txns_to_remove;
    while (branch_block->depth > current_block->depth)
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);

    while (branch_block->id != current_block->id) {
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);
    }

    // current_balance_change = balances just before block insertion point
    for (int i = 0; i < total_peers; i++)
        current_balance_change[i] += balances[i] - branch_balance_change[i];

    custom_unordered_set<Block*> blocks_to_add;
    Block* deepest_block = NULL;

    free_blocks_dfs(block, current_balance_change, blocks_to_add, deepest_block, sim);

    // block is invalid
    if (deepest_block == NULL)
        return;

    // now block gets added to blockchain
    // balances will be updated only if branch was changed
    if (deepest_block->depth > blockchain.current_block->depth) {

        // change peer state to just before block insertion
        balances = current_balance_change;
        for (Transaction* txn : txns_to_add)
            txn_pool.insert(txn);
        for (Transaction* txn : txns_to_remove)
            txn_pool.erase(txn);

        stack<Block*> order;
        order.push(deepest_block);

        while (order.top() != block)
            order.push(order.top()->parent);

        while (!order.empty()) {
            Block* b = order.top();
            order.pop();
            add_block(b, true);
            blocks_to_add.erase(b);
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

        if (next_mining_event != NULL) {
            sim->delete_event(next_mining_event);
            delete next_mining_block;
            schedule_next_block(sim);
        }
    } else {
        for (Block* b : blocks_to_add)
            add_block(b, false);
    }
}

/* output the edges in blockchain in os and update deepest_block */
void Peer::traverse_blockchain(Block* b, ostream& os, Block*& deepest_block, vector<int>& total_blocks) {
    // for canonicalization
    sort(all(b->next), [](Block* a1, Block* a2) {
        return (a1->id) < (a2->id);
    });
    if (b->depth > deepest_block->depth)
        deepest_block = b;

    // genesis id is -2
    if (b->parent_id >= -1) 
        total_blocks[b->owner->id]++;
    
    for (Block* c : b->next) {
        os << (b->id + 1) << ' ' << (c->id + 1) << '\n';
        traverse_blockchain(c, os, deepest_block, total_blocks);
    }
}

/* output the arrival times of blocks to os */
void Peer::export_arrival_times(ostream& os) {
    os << get_name() << '\n';
    for (pair<Block*, ld>& p : block_arrival_times) {
        Block* b = p.first;
        ld timestamp = p.second;
        os << b->get_name() << ", ";
        os << (b->depth) << ", ";
        os << fixed << setprecision(5) << timestamp << ", ";
        if (b->parent == NULL)
            os << "NO_PARENT" << '\n';
        else
            os << b->parent->get_name() << '\n';
    }
    os << '\n';
}

/* output the final statistics to a file */
void Peer::analyse_and_export_blockchain(Simulator* sim) {
    string filename = "output/blockchain_edgelist_"+get_name()+".txt";
    ofstream outfile(filename);
    Block* deepest_block = blockchain.genesis;
    vector<int> total_blocks(total_peers, 0);
    traverse_blockchain(blockchain.genesis, outfile, deepest_block, total_blocks);
    outfile.close();
    
    vector<int> blocks_in_chain(total_peers, 0);
    while (deepest_block->id != blockchain.genesis->id) {
        blocks_in_chain[deepest_block->owner->id]++;
        deepest_block = deepest_block->parent;
    }

    if(id!=0) return; // this part should execute only once
    filename = "output/peer_attributes.txt";
    outfile = ofstream(filename);
    outfile << "id,chain_blocks,generated_blocks,is_fast,hash_power\n";
    for (int i = 0; i < total_peers; i++) {
        outfile << (i + 1) << ',';
        outfile << blocks_in_chain[i] << ',';
        outfile << total_blocks[i] << ',';
        outfile << (int)sim->peers[i]->is_fast << ',';
        outfile << sim->peers[i]->hash_power << '\n';
    }
    outfile.close();
}
