#include "declarations.h"
using namespace std;

int Block::max_size;
int Block::counter;

/* constructor */
Block::Block(Peer* owner_) {
    owner = owner_;
    parent = NULL;
    parent_id = -1;
    depth = -1;
    size = 1; // block size with coinbase = 1 KB
    id = -1;
}

/* set id for this block and increment static counter */
void Block::set_id() {
    id = counter++;
}

/* add txn in this block */
void Block::add(Transaction* txn) {
    txns.emplace_back(txn);
    size += TRANSACTION_SIZE; // size of every txn = 1 KB
}

/* return name of current block */
string Block::get_name() {
    return "Blk" + to_string(id + 1);
}

/* return a copy of this block */
Block* Block::clone() {
    Block* ret = new Block(*this);
    return ret;
}

/* set parent of this block to b */
void Block::set_parent(Block* b) {
    parent = b;
    // b == NULL indicates genesis block
    parent_id = (b == NULL) ? -2 : b->id;
    depth = (b == NULL) ? 0 : b->depth + 1;
}

/* set parent pointer to NULL */
void Block::reset_parent() {
    parent = NULL;
    depth = -1;
}