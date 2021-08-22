#include "declarations.h"
using namespace std;

int Peer::counter;
int Peer::total_peers;
ld Peer::Ttx;

Peer::Peer() {  
    balances.resize(total_peers,0);
    assert(Ttx>0);
    // avg of expo dist = 1/lambda
    exp_dist_time = exponential_distribution<ld> (1.0/Ttx); 
    unif_dist_peer = uniform_int_distribution<int> (0, total_peers - 1);
}

// adds a link between peer a and peer b
void Peer::add_edge(Peer* a, Peer* b) {
    Link ba(a, b->is_fast && a->is_fast);
    (b->adj).emplace_back(ba);

    Link ab(b, a->is_fast && b->is_fast);
    (a->adj).emplace_back(ab);
}

set<Event*> Peer::generate_next_transaction(ld cur_time){

    ld interArrivalTime = exp_dist_time(rng64);
    set<Event*> events;
    Event *ev = new GenerateTransaction(cur_time+interArrivalTime, this->id);
    events.insert(ev);

    return events;

}

set<Event*> Peer::forward_transaction(ld cur_time, int source_id, Transaction *txn){

    set<Event*> events;

    // send transation to peers
    for(Link link: this->adj){
        if(link.peer->id==source_id) continue; // source already has the txn, loop-less forwarding
        ld delay = link.get_delay(TRANSACTION_SIZE);
        Event *ev = new ReceiveTransaction(cur_time+delay, this->id, link.peer->id, txn);
        events.insert(ev);
    }

    return events;

}

set<Event*> Peer::generate_transaction(ld cur_time){

    set<Event*> events;

    if(this->balances[this->id]>0){
        uniform_int_distribution unif_coins_dist(1,this->balances[this->id]);
        int coins = unif_coins_dist(rng64);

        // todo: check if uniform distribution is correct for sampling receiver & no of coins
        int receiver = unif_dist_peer(rng64);
        while(receiver==this->id){
            receiver = unif_dist_peer(rng64);
        }

        Transaction *txn = new Transaction(this->id, receiver, coins);

        // todo: add transaction in tranaction/recv pool

        // forward the transaction to peers
        Event *ev = new ForwardTransaction(cur_time, this->id, this->id, txn);

        events.insert(ev);
        
    }

    // generate new transaction
    set<Event*> gen_events = this->generate_next_transaction(cur_time);
    for(Event* ev: gen_events){
        events.insert(ev);
    }

    return events;

}

set<Event*> Peer::receive_transaction(ld cur_time, int sender_id, Transaction *txn){

    set<Event*> events;

    // if already received the transaction then ignore
    if(this->recv_pool.find(txn->id)!=this->recv_pool.end()){
        return set<Event*>();
    }

    recv_pool.insert(txn->id);
    txn_pool.insert(txn);

    // forward the txn to other peers
    events.insert(new ForwardTransaction(cur_time, this->id, sender_id, txn));

    return events;
}




