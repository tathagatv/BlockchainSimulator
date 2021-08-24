#include "declarations.h"
using namespace std;

Simulator::Simulator(int n_, ld z_, ld Ttx_, ld Tk_, int edges_) {
    n = n_;
    z_ = min((ld)1, max(z_, (ld)0));
    slow_peers = z_ * n;
    Ttx = Ttx_; Tk = Tk_;
    edges = edges_;
    current_timestamp = START_TIME;

    Transaction::counter = 0;
    Block::max_size = 1000;
    Block::counter = 0;
    Peer::counter = 0;
    Peer::total_peers = n;
    Peer::Ttx = Ttx;
    Peer::Tk = Tk;
}

void Simulator::get_new_peers() {
    peers.resize(n);
    for (int i = 0; i < slow_peers; i++)
        peers[i].is_fast = false;
    for (int i = slow_peers; i < n; i++)
        peers[i].is_fast = true;

    random_shuffle(peers);
    for (int i = 0; i < n; i++) {
        peers[i].balances[0] = 1000;
        peers[i].id = Peer::counter++;
    }
}

void Simulator::form_random_network() {
    assert(edges >= n - 1);
    int n = peers.size();
    uniform_int_distribution<int> unif(0, n - 1);

    // https://stackoverflow.com/a/14618505
    int current_node = unif(rng64);
    set<int> s, t;
    for (int i = 0; i < n; i++)
        s.insert(i);

    s.erase(current_node);
    t.insert(current_node);

    set<pair<int, int>> edges_log;
    while (!s.empty()) {
        int neighbour_node = unif(rng64);
        if (t.find(neighbour_node) == t.end()) {
            Peer::add_edge(&peers[current_node], &peers[neighbour_node]);
            edges_log.insert(make_pair(current_node, neighbour_node));
            s.erase(neighbour_node);
            t.insert(neighbour_node);
            edges--;
        }
        current_node = neighbour_node;
    }
    set<pair<int, int>>::iterator it;
    while (edges > 0) {
        int a = unif(rng64);
        int b = unif(rng64);
        while (a == b)
            b = unif(rng64);
        it = edges_log.find(make_pair(a, b));
        if (it == edges_log.end()) {
            Peer::add_edge(&peers[a], &peers[b]);
            edges--;
        }
    }
}

void Simulator::init_events() {
    for (Peer& peer : peers) {
        peer.schedule_next_transaction(this);
        peer.schedule_next_block(this);
    }
}

void Simulator::add_event(Event* event) {
    event->timestamp += current_timestamp;
    events.insert(event);
}

void Simulator::delete_event(Event* event) {
    events.erase(event);
    delete event;
}

void Simulator::run(ld end_time) {
    get_new_peers();
    form_random_network();
    init_events();

    while (!events.empty()) {
        current_event = *events.begin();
        current_timestamp = current_event->timestamp;

        if (current_event->timestamp > end_time)
            break;

        current_event->run(this);

        delete_event(current_event);
    }
}

void Simulator::log_time(ostream& os) {
    os << "Time " << fixed << setprecision(5) << current_timestamp << ": ";
}