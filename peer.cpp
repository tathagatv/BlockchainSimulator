#include "declarations.h"
using namespace std;

int Peer::counter;
int Peer::total_peers;
ld Peer::Ttx;
ld Peer::Tk;

Peer::Peer() {
    balances.resize(total_peers, 0);
    assert(Ttx > 0);
    assert(Tk > 0);
    // avg of expo dist = 1/lambda
    txn_interarrival_time = exponential_distribution<ld>(1.0 / Ttx);
    block_mining_time = exponential_distribution<ld>(1.0 / Tk);
    unif_dist_peer = uniform_int_distribution<int>(0, total_peers - 1);

    blockchain = new Blockchain(this);
}

string Peer::get_name() {
    return "Peer" + to_string(id + 1);
}

// adds a link between peer a and peer b
void Peer::add_edge(Peer *a, Peer *b) {
    Link ab(b, a->is_fast);
    (a->adj).emplace_back(ab);

    Link ba(a, b->is_fast);
    (b->adj).emplace_back(ba);
}

void Peer::schedule_next_transaction(Simulator* sim) {
    ld interArrivalTime = txn_interarrival_time(rng64);
    Event* ev = new GenerateTransaction(interArrivalTime, this);
    sim->add_event(ev);
}

Transaction* Peer::generate_transaction(Simulator* sim) {
    if (balances[id] > 0) {
        uniform_int_distribution unif_coins_dist(1, balances[id]);
        int coins = unif_coins_dist(rng64);

        // todo: check if uniform distribution is correct for sampling receiver & no of coins
        int receiver = unif_dist_peer(rng64);
        while (receiver == id)
            receiver = unif_dist_peer(rng64);

        Transaction *txn = new Transaction(sim->current_timestamp, this, &sim->peers[receiver], coins);

        // todo: add transaction in transaction/recv pool
        recv_pool.insert(txn->id);
        txn_pool.insert(txn);

        // forward the transaction to peers
        Event* ev = new ForwardTransaction(0, this, this, txn);
        sim->add_event(ev);

        // generate new transaction
        schedule_next_transaction(sim);
        
        return txn;
    } else {
        // generate new transaction
        schedule_next_transaction(sim);
        return NULL;
    }
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

// ================== BLOCK =============================
bool Peer::validate_txn(Transaction* txn, vector<int>& custom_balances) {
    int balance = custom_balances[txn->sender->id];
    return (balance >= txn->amount);
}

bool Peer::validate_block(Block* block, vector<int>& custom_balances) {
    if (block->size > Block::max_size)
        return false;
    for (Transaction* txn : block->txns) 
        if (!validate_txn(txn, custom_balances))
            return false;
    return true;
}

Block* Peer::generate_new_block(Simulator* sim) {
    Block* block = new Block(blockchain->current_block, this);
    for (Transaction* txn : txn_pool) {
        if (block->size + TRANSACTION_SIZE > Block::max_size) 
            break;
        if (validate_txn(txn, balances))
            block->add(txn);
    }
    return block;
}

void Peer::schedule_next_block(Simulator* sim) {
    next_mining_block = generate_new_block(sim);
    ld miningTime = block_mining_time(rng64);
    next_mining_event = new BroadcastMinedBlock(miningTime, this);
    sim->add_event(next_mining_event);
}

void Peer::forward_block(Simulator* sim, Peer* source, Block* block) {
    // send block to peers
    for (Link& link : adj) {
        if (link.peer->id == source->id) continue;  // source already has the txn, loop-less forwarding
        ld delay = link.get_delay(block->size);
        Event* ev = new ReceiveBlock(delay, this, link.peer, block);
        sim->add_event(ev);
    }
}

void Peer::add_block(Block* block, bool update_balances) {
    blockchain->add(block);
    chain_blocks.insert(block);
    if (update_balances) {
        set<Transaction*>::iterator it;
        for (Transaction* txn : block->txns) {
            balances[txn->sender->id] -= txn->amount;
            balances[txn->receiver->id] += txn->amount;
            it = txn_pool.find(txn);
            if (it != txn_pool.end())
                txn_pool.erase(it);
        }
        blockchain->current_block = block;
    }
}

void Peer::receive_block(Simulator* sim, Peer* sender, Block* block) {
    set<Block*>::iterator chain_it, free_it;

    chain_it = chain_blocks.find(block);
    free_it = free_blocks.find(block);

    // already received this block
    if (chain_it != chain_blocks.end() || free_it != free_blocks.end()) 
        return;
    
    chain_it = chain_blocks.find(block->parent);

    // block parent not in our blockchain
    if (chain_it == chain_blocks.end()) {
        // forward the block in this case?
        Event* ev = new ForwardBlock(0, this, sender, block);
        sim->add_event(ev);

        free_blocks.insert(block);
        free_block_parents[block->parent].emplace_back(block);
        return;
    }

    Block* current_block = blockchain->current_block;
    Block* branch_block = block->parent;

    // if we will stop mining and change branch
    bool branch_change = block->depth > current_block->depth;

    vector<int> current_balance_change(total_peers, 0);
    vector<Transaction*> txns_to_add;
    while (current_block->depth > branch_block->depth)
        current_block = Blockchain::backward(current_block, branch_change, current_balance_change, txns_to_add);
    
    vector<int> branch_balance_change(total_peers, 0);
    vector<Transaction*> txns_to_remove;
    while (branch_block->depth > current_block->depth)
        branch_block = Blockchain::backward(branch_block, branch_change, branch_balance_change, txns_to_remove);

    while (branch_block->id != current_block->id) {
        current_block = Blockchain::backward(current_block, branch_change, current_balance_change, txns_to_add);
        branch_block = Blockchain::backward(branch_block, branch_change, branch_balance_change, txns_to_remove);
    }

    // current_balance_change = balances just before block insertion point
    for (int i = 0; i < total_peers; i++)
        current_balance_change[i] += balances[i] - branch_balance_change[i];

    // validate block
    if (!validate_block(block, current_balance_change)) 
        return;

    map<Block*, vector<Block*>>::iterator parent_it;
    parent_it = free_block_parents.find(block);

    bool another_branch_change = (parent_it != free_block_parents.end()) && (block->depth + 1 > blockchain->current_block->depth);

    // now block gets added to blockchain
    // balances will be updated only if branch was changed
    if (another_branch_change || branch_change) {
        balances = current_balance_change;
        for (Transaction* txn : txns_to_add)
            txn_pool.insert(txn);
        for (Transaction* txn : txns_to_remove)
            txn_pool.erase(txn);
        
        add_block(block, true);

        if (another_branch_change) {
            Block* child_block = parent_it->second.back();
            parent_it->second.pop_back();
            add_block(child_block, true);
            free_blocks.erase(child_block);
        }

        sim->delete_event(next_mining_event);
        schedule_next_block(sim);
    } else {
        add_block(block, false);
    }
    
    Event* ev = new ForwardBlock(0, this, sender, block);
    sim->add_event(ev);

	if (parent_it == free_block_parents.end())
        return;

    for (Transaction* txn : block->txns) {
        current_balance_change[txn->sender->id] -= txn->amount;
        current_balance_change[txn->receiver->id] += txn->amount;
    }

    for (Block* child : parent_it->second) {
        // validate block here
        if (validate_block(child, current_balance_change)) 
            add_block(child, false);
        free_blocks.erase(child);
    }
    free_block_parents.erase(parent_it);
}