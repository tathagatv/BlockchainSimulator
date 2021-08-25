#include "declarations.h"
using namespace std;

Block* Blockchain::global_genesis;

Blockchain::Blockchain() {
    genesis = global_genesis->clone();
    current_block = genesis;
}

void Blockchain::add(Block* block) {
    assert(block->parent != NULL);
    (block->parent->next).emplace_back(block);
}

Block* Blockchain::backward(Block* b, vector<int>& balances, vector<Transaction*>& txns) {
    for (Transaction* txn : b->txns) {
        balances[txn->sender->id] += txn->amount;
        balances[txn->receiver->id] -= txn->amount;
        txns.emplace_back(txn);
    }
    balances[b->owner->id] -= MINING_FEE;
    return b->parent;
}