#include "declarations.h"

vector<Peer> Peer::peers;
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
mt19937_64 rng64(chrono::steady_clock::now().time_since_epoch().count());


int main() {
	// int n = 10;
	cout << "hello world" << endl;
}