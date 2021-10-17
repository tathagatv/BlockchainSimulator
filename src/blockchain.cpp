#include "declarations.h"
using namespace std;

Block* Blockchain::global_genesis;

/* constructor */
Blockchain::Blockchain() {
    // initialize every blockchain with a copy of genesis block */
    genesis = global_genesis->clone();
    current_block = genesis;
}

/* add block to the children of its parent block in the blockchain */
void Blockchain::add(Block* block) {
    assert(block->parent != NULL);
    (block->parent->next).push_back(block);
}

/* returns the parent while updating the balances array and storing the transactions */
Block* Blockchain::backward(Block* b, vector<int>& balances, vector<Transaction*>& txns) {
    // used when the longest branch changes to some other branch
    // equaivalent to rolling back the addition of block b
    for (Transaction* txn : b->txns) {
        balances[txn->sender->id] += txn->amount;
        balances[txn->receiver->id] -= txn->amount;
        txns.emplace_back(txn);
    }
    balances[b->owner->id] -= MINING_FEE;
    return b->parent;
}