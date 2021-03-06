#include "declarations.h"
#include "argparse.h"

/* random number generators */
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
mt19937_64 rng64(chrono::steady_clock::now().time_since_epoch().count());

int main(int argc, char *argv[]) {
	
	// https://github.com/p-ranav/argparse
	argparse::ArgumentParser argparser("./blockchain_simulator", "1.0");

	argparser.add_argument("--peers", "-n")
	.default_value((int)40)
	.required()
	.help("Number of peers in the network")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--edges", "-e")
	.default_value((int)150)
	.required()
	.help("Number of edges in the peer network")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--slowpeers", "-z")
	.default_value((ld)0.4)
	.required()
	.help("Fraction of slow peers in the network")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--time_limit", "-t")
	.default_value((ld)DBL_MAX)
	.required()
	.help("Run the simulation upto a time limit (in seconds)")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--txn_interarrival", "-Ttx")
	.default_value((ld)40)
	.required()
	.help("Mean of exponential distribution of interarrival time between transactions")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--mining_time", "-Tk")
	.default_value((ld)1000.0)
	.required()
	.help("Mean of exponential distribution of time to mine a block")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--seed", "-s")
	.default_value((int)42)
	.required()
	.help("Seed for random number generator")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--max_txns", "-txn")
	.default_value((int)0)
	.required()
	.help("Run simulation till max transactions are generated, 0 indicates infinity")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--max_blocks", "-blk")
	.default_value((int)100)
	.required()
	.help("Run simulation till max blocks are generated, 0 indicates infinity")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--verbose", "-v")
	.default_value(false)
	.implicit_value(true)
	.help("Print output log");

	argparser.add_argument("--invalid_txn_prob", "-it")
	.default_value((ld)0.05)
	.required()
	.help("Probability of generating an invalid transaction")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--invalid_block_prob", "-ib")
	.default_value((ld)0.05)
	.required()
	.help("Probability of generating an invalid block")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--attacker_connection", "-zeta")
	.default_value((ld)0.5)
	.help("Fraction of honest nodes adversary is connected to")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--alpha", "-a")
	.default_value((ld)0.35)
	.help("Fraction of hash power belonging to attacker")
	.action([](const string& value) { return stold(value); });

	argparser.add_argument("--adversary_type", "-adv")
	.default_value((string)"none")
	.help("Type of adversary, choose from (none, selfish, stubborn)")
	.action([](const string& value) { 
		static const vector<string> choices = { "none", "selfish", "stubborn" };
		if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
			return value;
		}
		return std::string{ "none" };
	});

	argparser.parse_args(argc, argv);

	ld z = argparser.get<ld>("-z");
	ld t = argparser.get<ld>("-t");
	ld Ttx = argparser.get<ld>("-Ttx");
	ld Tk = argparser.get<ld>("-Tk");
	int n = argparser.get<int>("-n");
	int seed = argparser.get<int>("-s");
	int edges = argparser.get<int>("-e");
	int max_txns = argparser.get<int>("-txn");
	int max_blocks = argparser.get<int>("-blk");
	bool verbose = argparser.get<bool>("-v");
	ld invalid_txn_prob = argparser.get<ld>("-it");
	ld invalid_block_prob = argparser.get<ld>("-ib");
	ld zeta = argparser.get<ld>("-zeta");
	ld alpha = argparser.get<ld>("-a");
	string adv = argparser.get<string>("-adv");

	rng.seed(seed);
	rng64.seed(seed);

	Simulator simulator(n, z, Ttx, Tk, edges, verbose, invalid_txn_prob, invalid_block_prob, zeta, adv, alpha);
	simulator.run(t, max_txns, max_blocks);

	return 0;
}