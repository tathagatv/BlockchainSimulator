#include "declarations.h"
using namespace std;

int Peer::counter;

Peer::Peer() {  
    balances.resize(total_peers,0);
}

// adds a link between peer a and peer b
void Peer::add_edge(Peer* a, Peer* b) {
    Link ba(a, b->is_fast && a->is_fast);
    (b->adj).emplace_back(ba);

    Link ab(b, a->is_fast && b->is_fast);
    (a->adj).emplace_back(ab);
}

set<Event> Peer::send_transaction(ld cur_time){

    // choose receiver uniform randomly
    Transaction txn(a, b, coins);

}




