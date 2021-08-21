#include "declarations.h"
using namespace std;

int Peer::counter;

Peer::Peer() {}

void Peer::add_edge(Peer* a, Peer* b) {
    Link ba(a, b->is_fast);
    (b->adj).emplace_back(ba);

    Link ab(b, a->is_fast);
    (a->adj).emplace_back(ab);
}
