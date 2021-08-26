#include "declarations.h"
using namespace std;

int Transaction::counter;

Transaction::Transaction(ld timestamp_, Peer* sender_, Peer* receiver_, int coins) {
    id = counter++;
    sender = sender_;
    receiver = receiver_;
    amount = coins;
    timestamp = timestamp_;
}

string Transaction::get_name() {
    return "Txn" + to_string(id + 1);
}