#include "declarations.h"
using namespace std;

/* constructor */
Link::Link(Peer* p, bool is_fast) {
    // 5Mbps = 625KBps
    // 100Mbps = 12500KBps
    peer = p;
    uniform_real_distribution<ld> unif(0.01, 0.5);
    c = (p->is_fast && is_fast) ? 12500 : 625;  // KBps
    // exponential distribution with mean 96kbits/c = 12KB/c , lambda = c/12
    exp = exponential_distribution<ld>(c / 12);
    // ro chosen from uniform distribution between 10ms and 500ms
    ro = unif(rng64);
}

/* returns the delay for a message of size length KB */
ld Link::get_delay(int length) {
    ld d = exp(rng64);
    return ro + (ld)length / c + d;
}