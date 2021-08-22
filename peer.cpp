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

set<Event*> Peer::generate_transaction(ld cur_time){

    ld interArrivalTime = exp_dist_time(rng64);
    set<Event*> events;
    Event *ev = new SendTransaction(cur_time+interArrivalTime, this->id);
    events.insert(ev);

    return events;

}

set<Event*> Peer::send_transaction(ld cur_time){

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
        
        // send transation to peers
        for(Link link: adj){
            ld delay = link.get_delay(TRANSACTION_SIZE);
            Event *ev = new ReceiveTransaction(cur_time+delay, link.peer, txn);
            events.insert(ev);
        }
    }

    // generate new transaction
    this->generate_transaction(cur_time);

    return events;

}




