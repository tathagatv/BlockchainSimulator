#include "declarations.h"
using namespace std;

Link::Link(Peer* p, bool is_fast) {
    // 5000kbps = 625KBps
    // 5000kbps = 12500KBps
    peer = p;
    uniform_real_distribution<ld> unif(0.01, 0.5);
    c = (is_fast && p->is_fast) ? 12500 : 625; 
	exp = exponential_distribution<ld>(12 / c);
    ro = unif(rng64);
}

ld Link::get_delay(int length) {
    // length is in KB
    ld d = exp(rng64);
    return ro + (ld)length / c + d;
}
