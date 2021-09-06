#include "declarations.h"
using namespace std;

int Block::max_size;
int Block::counter;

Block::Block(Peer* owner_) {
    owner = owner_;
    parent = NULL;
    parent_id = -1;
    depth = -1;
    size = 1; // coinbase
    id = -1;
}

void Block::set_id() {
    id = counter++;
}

void Block::add(Transaction* txn) {
    txns.emplace_back(txn);
    size += 1;
}

string Block::get_name() {
    return "Blk" + to_string(id + 1);
}

Block* Block::clone() {
    Block* ret = new Block(*this);
    return ret;
}

void Block::set_parent(Block* b) {
    parent = b;
    parent_id = (b == NULL) ? -2 : b->id;
    depth = (b == NULL) ? 0 : b->depth + 1;
}

void Block::reset_parent() {
    parent = NULL;
    depth = -1;
}