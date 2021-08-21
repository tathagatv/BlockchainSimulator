#include "declarations.h"
using namespace std;

int Transaction::counter;

Transaction::Transaction(Peer* a, Peer* b, int coins){
    id = counter;
    counter++;
    sender = a;
    receiver = b;
    amount = coins;
}
