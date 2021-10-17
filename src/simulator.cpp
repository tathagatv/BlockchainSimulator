#include "declarations.h"
using namespace std;

Simulator::Simulator(int n_, ld z_, ld Ttx_, ld Tk_, int edges_, bool verbose_, ld invalid_txn_prob_, ld invalid_block_prob_, ld zeta_, string adversary_, ld alpha_) {
    n = n_;
    // ensure z (fraction of slow peers) is between 0 and 1
    z_ = min((ld)1, max(z_, (ld)0));
    slow_peers = z_ * n;
    Ttx = Ttx_;
    Tk = Tk_;
    edges = edges_;
    current_timestamp = START_TIME;
    invalid_txn_prob = invalid_txn_prob_;
    invalid_block_prob = invalid_block_prob_;
    verbose = verbose_;
    has_simulation_ended = false;
    zeta = zeta_;
    adversary = adversary_;
    alpha = alpha_;

    Transaction::counter = 0;
    Block::max_size = MAX_BLOCK_SIZE;  // 1000 KB
    Block::counter = 0;
    Blockchain::global_genesis = new Block(NULL);
    Blockchain::global_genesis->set_parent(NULL);
    Peer::counter = 0;
    Peer::total_peers = n;
    Peer::Ttx = Ttx;
    Peer::Tk = Tk;
}

Simulator::~Simulator() {
    for (int i = 0; i < n; i++) {
        delete peers[i];
    }
}

/* initialize peers */
void Simulator::get_new_peers() {

    if (adversary != "none")
        n--;

    peers.resize(n);

    for (int i = 0; i < n; i++) {
        peers[i] = new Peer;
    }
    for (int i = 0; i < slow_peers; i++)
        peers[i]->is_fast = false;
    for (int i = slow_peers; i < n; i++)
        peers[i]->is_fast = true;

    random_shuffle(peers);

    if (adversary != "none") {
        n++;
        // delete peers[n - 1];
        // peers.pop_back();
        if (adversary == "selfish") {
            peers.push_back(new SelfishAttacker);
        } else {
            peers.push_back(new StubbornAttacker);
        }
        peers[n - 1]->is_fast = true;
        ld honest_power = 0.0;
        for (int i = 0; i < n - 1; i++)
            honest_power += peers[i]->hash_power;
        peers[n - 1]->hash_power = (honest_power * alpha) / (1 - alpha);
    }

    for (int i = 0; i < n; i++)
        peers[i]->id = Peer::counter++;

    // normalization factor: to normalize the hash power
    ld normalization_factor = 0;
    for (Peer* p : peers)
        normalization_factor += p->hash_power / n;
    for (Peer* p : peers)
        p->initialize_block_mining_distribution(p->hash_power / normalization_factor);
}

/* generate scale free network between the peers */
void Simulator::form_random_network(ostream& os) {
    assert(edges >= n - 1);
    int n = peers.size();

    if (adversary != "none")
        n--;

    assert(n >= 2);
    uniform_int_distribution<int> unif(0, n - 1);

    // preferential attachment algorithm - scale free network
    int node_1 = unif(rng64);
    int node_2 = unif(rng64);
    while (node_2 == node_1)
        node_2 = unif(rng64);

    if (node_1 > node_2)
        swap(node_1, node_2);

    // s: nodes not yet added in network
    // t: nodes already added in network
    set<int> s, t;

    for (int i = 0; i < n; i++)
        s.insert(i);

    s.erase(node_1), t.insert(node_1);
    s.erase(node_2), t.insert(node_2);

    set<pair<int, int>> edges_log;

    // add edge between node_1 and node_2
    vector<int> degrees(n, 0);
    Peer::add_edge(peers[node_1], peers[node_2], os);
    edges_log.insert(make_pair(node_1, node_2));
    edges--, degrees[node_1]++, degrees[node_2]++;

    while (!s.empty()) {
        int next_node = unif(rng64);
        if (t.find(next_node) == t.end()) {
            discrete_distribution<int> disc(degrees.begin(), degrees.end());
            int neighbour_node = disc(rng64);
            while (neighbour_node == next_node)
                neighbour_node = disc(rng64);

            s.erase(next_node);
            t.insert(next_node);

            if (next_node > neighbour_node)
                swap(next_node, neighbour_node);

            Peer::add_edge(peers[next_node], peers[neighbour_node], os);
            edges_log.insert(make_pair(next_node, neighbour_node));
            edges--, degrees[next_node]++, degrees[neighbour_node]++;
        }
    }

    set<pair<int, int>>::iterator it;
    while (edges > 0) {
        int a = unif(rng64);
        discrete_distribution<int> disc(degrees.begin(), degrees.end());
        int b = disc(rng64);
        while (b == a)
            b = disc(rng64);

        if (a > b) swap(a, b);

        if (!edges_log.count(make_pair(a, b))) {
            Peer::add_edge(peers[a], peers[b], os);
            edges--, degrees[a]++, degrees[b]++;
        }
    }

    if (adversary != "none") {
        // add edges to adversary
        edges = (int)(zeta * n);

        n += 1;
        while (edges > 0) {
            int a = unif(rng64);
            int b = n - 1;

            if (!edges_log.count(make_pair(a, b))) {
                Peer::add_edge(peers[a], peers[b], os);
                edges--, degrees[a]++, degrees[b]++;
            }
        }
    }
}

void Simulator::init_events() {
    for (Peer* peer : peers) {
        peer->schedule_next_transaction(this);
        peer->schedule_next_block(this);
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

void Simulator::reset(const fs::path& dir_path) {
    fs::remove_all(dir_path);
    fs::create_directories(dir_path);
}

void Simulator::run(ld end_time_, int max_txns_, int max_blocks_) {
    get_new_peers();

    reset("output");
    string filename = "output/peer_network_edgelist.txt";
    ofstream outfile(filename);
    form_random_network(outfile);
    outfile.close();
    
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

    // log(cout, "SIMULATION HAS ENDED AT THIS POINT\n");
    
    reset("output/termination_blockchains");
    for (Peer* p : peers) {
        string filename = "output/termination_blockchains/" + p->get_name() + ".txt";
        ofstream outfile(filename);
        p->export_blockchain(outfile);
        outfile.close();
    }

    complete_non_generate_events();

    reset("output/block_arrivals");
    for (Peer* p : peers) {
        string filename = "output/block_arrivals/" + p->get_name() + ".txt";
        ofstream outfile(filename);
        p->export_arrival_times(outfile);
        outfile.close();
    }

    reset("output/peer_stats");
    for (Peer* p : peers) {
        string filename = "output/peer_stats/" + p->get_name() + ".txt";
        ofstream outfile(filename);
        p->export_stats(this, outfile);
        outfile.close();
    }

    cout << "Total Transactions: " << (Transaction::counter) << '\n';
    cout << "Total Blocks: " << (Block::counter) << '\n';
}

void Simulator::complete_non_generate_events() {
    has_simulation_ended = true;

    for (Peer* p : peers) {
        p->next_mining_event = NULL;
        p->next_mining_block = NULL;
    }

    while (!events.empty()) {
        current_event = *events.begin();
        current_timestamp = current_event->timestamp;

        if (!current_event->is_generate_type_event)
            current_event->run(this);

        delete_event(current_event);
    }

    reset("output/final_blockchains");
    for (Peer* p : peers) {
        string filename = "output/final_blockchains/" + p->get_name() + ".txt";
        ofstream outfile(filename);
        p->export_blockchain(outfile);
        outfile.close();
    }
}

void Simulator::log(ostream& os, const string& s) {
    if (!verbose) return;
    os << "Time " << fixed << setprecision(5) << current_timestamp;
    os << ": " << s << '\n' << flush;
}