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
    for (Transaction* txn : block->txns) 
        if (!validate_txn(txn, custom_balances))
            return false;
    return true;
}

Block* Peer::generate_new_block(Simulator* sim) {
    Block* block = new Block(blockchain->current_block, this);
    for (Transaction* txn : txn_pool) {
        if (block->size >= Block::max_size) 
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

void Peer::receive_block(Simulator* sim, Peer* sender, Block* block) {
    // add prrof of work logic
}