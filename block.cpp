#include "declarations.h"
using namespace std;

int Block::max_size;
int Block::counter;

Block::Block(Block* parent_, Peer* owner_) {
    owner = owner_;
    parent = parent_;
    depth = (parent == NULL) ? 0 : parent->depth + 1;
    size = 1; // coinbase
    id = counter++;
}

void Block::add(Transaction* txn) {
    txns.emplace_back(txn);
    size += 1;
}

string Block::get_name() {
    return "Blk" + to_string(id + 1);
}