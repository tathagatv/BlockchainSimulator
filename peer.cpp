#include "declarations.h"
using namespace std;

int Peer::counter;

Peer::Peer() {}

void Peer::get_new_peers(int n, int num_slow) {
    Peer::peers.resize(n);
    for (int i = 0; i < num_slow; i++)
        Peer::peers[i].is_fast = false;
    for (int i = num_slow; i < n; i++)
        Peer::peers[i].is_fast = true;
    
    random_shuffle(Peer::peers);
    for (int i = 0; i < n; i++)
        Peer::peers[i].id = Peer::counter++;
}
