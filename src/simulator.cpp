#include "declarations.h"
using namespace std;

Simulator::Simulator(int n_, ld z_, ld Ttx_, ld Tk_, ld Hvar_, int edges_, bool verbose_, ld invalid_txn_prob_, ld invalid_block_prob_) {
    n = n_;
    z_ = min((ld)1, max(z_, (ld)0));
    slow_peers = z_ * n;
    Ttx = Ttx_; Tk = Tk_; Hvar = Hvar_;
    edges = edges_;
    current_timestamp = START_TIME;
    invalid_txn_prob = invalid_txn_prob_;
    invalid_block_prob = invalid_block_prob_;
    verbose = verbose_;
    has_simulation_ended = false;

    Transaction::counter = 0;
    Block::max_size = 1000;
    Block::counter = 0;
    Blockchain::global_genesis = new Block(NULL);
    Blockchain::global_genesis->set_parent(NULL);
    Peer::counter = 0;
    Peer::total_peers = n;
    Peer::Ttx = Ttx;
    Peer::Tk = Tk;
    Peer::Hvar = Hvar;
}

void Simulator::get_new_peers() {
    peers.resize(n);
    for (int i = 0; i < slow_peers; i++)
        peers[i].is_fast = false;
    for (int i = slow_peers; i < n; i++)
        peers[i].is_fast = true;

    random_shuffle(peers);
    for (int i = 0; i < n; i++) {
        peers[i].id = Peer::counter++;
    }

    // create hash power distribution
    vector<ld> hash_power_distribution(n);
    ld hash_power_norma = 0;
    for (int i = 0; i < n; i++){
        hash_power_distribution[i] = exp(-pow((i-n/2.0), 2)/(2*pow(Hvar, 2)));
        hash_power_norma += hash_power_distribution[i];
    }
    for (int i = 0; i < n; i++)
        peers[i].initialize_block_mining_distribution(hash_power_distribution[i]/hash_power_norma);
    
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
    assert(event != NULL);
    set<Event*>::iterator it;
    it = events.find(event);
    assert(it != events.end());
    events.erase(it);
    delete event;
}

void Simulator::run(ld end_time_, int max_txns_, int max_blocks_) {
    get_new_peers();
    form_random_network();
    init_events();
    has_simulation_ended = false;

    int max_txns = max_txns_ <= 0 ? INT_MAX : max_txns_;
    int max_blocks = max_blocks_ <= 0 ? INT_MAX : max_blocks_;
    ld end_time = end_time_ <= 0 ? DBL_MAX : end_time_;

    while (!events.empty()) {
        current_event = *events.begin();
        current_timestamp = current_event->timestamp;

        if (current_event->timestamp > end_time)
            break;
        if (Transaction::counter >= max_txns)
            break;
        if (Block::counter >= max_blocks)
            break;

        current_event->run(this);

        delete_event(current_event);
    }

    complete_non_generate_events();

    cout << "Total Transactions: " << (Transaction::counter) << '\n';
    cout << "Total Blocks: " << (Block::counter) << '\n';
}

void Simulator::complete_non_generate_events() {
    has_simulation_ended = true;
    
    for (Peer& p : peers) {
        p.next_mining_event = NULL;
        p.next_mining_block = NULL;
    }
    
    log(cout, "SIMULATION HAS ENDED AT THIS POINT\n");

    while (!events.empty()) {
        current_event = *events.begin();
        current_timestamp = current_event->timestamp;

        if (!current_event->is_generate_type_event) 
            current_event->run(this);

        delete_event(current_event);
    }
    peers[0].analyse_and_export_blockchain();
    for (Peer& p : peers){
        string filename = "output/block_arrivals_" + to_string(p.id+1) + ".txt";
        ofstream outfile(filename);
        p.export_arrival_times(outfile);
        outfile.close();
    }
}

void Simulator::log(ostream& os, const string& s) {
    if (!verbose) return;
    os << "Time " << fixed << setprecision(5) << current_timestamp;
    os << ": " << s << '\n' << flush;
}