#include "declarations.h"
using namespace std;

int Transaction::counter;

Transaction::Transaction(int a, int b, int coins){
    id = counter;
    counter++;
    senderId = a;
    receiverId = b;
    amount = coins;
}
