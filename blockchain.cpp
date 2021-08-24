#include "declarations.h"
using namespace std;

Blockchain::Blockchain(Peer* p) {
    genesis = new Block(NULL, p);
    current_block = genesis;
}

void Blockchain::add(Block* block) {
    assert(block->parent != NULL);
    (block->parent->next).emplace_back(block);
}

Block* Blockchain::backward(Block* b, bool collect_txn, vector<int>& balances, vector<Transaction*>& txns) {
    for (Transaction* txn : b->txns) {
        balances[txn->sender->id] += txn->amount;
        balances[txn->receiver->id] -= txn->amount;
        if (collect_txn) txns.emplace_back(txn);
    }
    balances[b->owner->id] -= MINING_FEE;
    return b->parent;
}