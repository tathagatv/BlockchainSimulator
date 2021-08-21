#include "declarations.h"
#include "argparse.h"

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
mt19937_64 rng64(chrono::steady_clock::now().time_since_epoch().count());


int main(int argc, char *argv[]) {
	
	// https://github.com/p-ranav/argparse
	argparse::ArgumentParser argparser("Blockchain Simulator");

	argparser.add_argument("--peers", "-n")
	.default_value(10)
	.required()
	.help("Number of peers in network")
	.action([](const string& value) { return stoi(value); });

	argparser.add_argument("--verbose", "-v")
	.default_value(false)
	.implicit_value(true)
	.help("Print output log");

	argparser.parse_args(argc, argv);

	int n = argparser.get<int>("-n");
	cout << "Number of peers = " << n << endl;
}