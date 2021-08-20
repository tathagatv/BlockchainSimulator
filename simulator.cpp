#include "declarations.h"
using namespace std;

Simulator::Simulator(int n_, ld z_) {
    n = n_;
    z_ = min((ld)1, max(z_, (ld)0));
    slow_peers = z_ * n;
}

void Simulator::form_peer_network() {
    Peer::counter = 1;
    Peer::get_new_peers(n, slow_peers);
}

void Simulator::init_events() {
    // events.insert()
}