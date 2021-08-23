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