#include "declarations.h"
using namespace std;

Block* SelfishAttacker::generate_new_block(Simulator* sim) {
    Block* block = new Block(this);
    block->set_parent(blockchain.current_block);
    vector<int> balances_copy = balances;
    for (Transaction* txn : txn_pool) {
        if (block->size + TRANSACTION_SIZE > Block::max_size) 
            break;
        if (validate_txn(txn, balances_copy)) {
            block->add(txn);
            balances_copy[txn->sender->id] -= txn->amount;
            balances_copy[txn->receiver->id] += txn->amount;
        }
    }
    return block;
}

Block* StubbornAttacker::generate_new_block(Simulator* sim) {
    Block* block = new Block(this);
    block->set_parent(blockchain.current_block);
    vector<int> balances_copy = balances;
    for (Transaction* txn : txn_pool) {
        if (block->size + TRANSACTION_SIZE > Block::max_size) 
            break;
        if (validate_txn(txn, balances_copy)) {
            block->add(txn);
            balances_copy[txn->sender->id] -= txn->amount;
            balances_copy[txn->receiver->id] += txn->amount;
        }
    }
    return block;
}

void SelfishAttacker::broadcast_mined_block(Simulator* sim){
    Block* block = next_mining_block;
	block->set_id();
	
	assert(blockchain.current_block->id == block->parent->id);
	bool is_valid = validate_block(block, balances);

	// do not add invalid block, only transmit it to other peers
	string validity = "INVALID";
	if (is_valid) {
		add_block(block, true);
		validity = "VALID";
	}
	sim->log(cout, get_name() + " mines and broadcasts " + validity + " block " + block->get_name());
    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));

	// Event* ev = new ForwardBlock(0, this, this, block->clone());
	// sim->add_event(ev);

	schedule_next_block(sim);
}

void StubbornAttacker::broadcast_mined_block(Simulator* sim){
    Block* block = next_mining_block;
	block->set_id();
	
	assert(blockchain.current_block->id == block->parent->id);
	bool is_valid = validate_block(block, balances);

	// do not add invalid block, only transmit it to other peers
	string validity = "INVALID";
	if (is_valid) {
		add_block(block, true);
		validity = "VALID";
	}
	sim->log(cout, get_name() + " mines and broadcasts " + validity + " block " + block->get_name());
    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));

	// Event* ev = new ForwardBlock(0, this, this, block->clone());
	// sim->add_event(ev);

	schedule_next_block(sim);
}

void SelfishAttacker::receive_block(Simulator* sim, Peer* sender, Block* block) {
    custom_map<int, Block*>::iterator chain_it, free_it;
    custom_unordered_set<int>::iterator reject_it;

    chain_it = chain_blocks.find(block->id);
    free_it = free_blocks.find(block->id);
    reject_it = rejected_blocks.find(block->id);

    // already received this block
    if (chain_it != chain_blocks.end() || free_it != free_blocks.end() || reject_it != rejected_blocks.end()) 
        return;

    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));
    
    // attacker doesn't forward honest blocks
    // Event* ev = new ForwardBlock(0, this, sender, block->clone());
    // sim->add_event(ev);
    
    chain_it = chain_blocks.find(block->parent_id);

    // block parent not in our blockchain
    if (chain_it == chain_blocks.end()) {
        block->reset_parent();
        free_blocks[block->id] = block;
        free_block_parents[block->parent_id].emplace_back(block);
        return;
    }

    block->set_parent(chain_it->second);

    Block* current_block = blockchain.current_block; // last block in the blockchain
    Block* branch_block = block->parent; // add the new block as a child of branch block

    // balances to update in case longest chain changes
    vector<int> current_balance_change(total_peers, 0); 
    // txns to add to the txn pool in case longest chain changes
    vector<Transaction*> txns_to_add; 
    // find lca
    while (current_block->depth > branch_block->depth)
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
    
    // balances to update in case longest chain changes
    vector<int> branch_balance_change(total_peers, 0);
    // txns to remove from the txn pool in case longest chain changes
    vector<Transaction*> txns_to_remove;
    while (branch_block->depth > current_block->depth)
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);

    while (branch_block->id != current_block->id) {
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);
    }

    // current_balance_change = balances just before block insertion point
    for (int i = 0; i < total_peers; i++)
        current_balance_change[i] += balances[i] - branch_balance_change[i];

    custom_unordered_set<Block*> blocks_to_add;
    Block* deepest_block = NULL;

    free_blocks_dfs(block, current_balance_change, blocks_to_add, deepest_block, sim);

    // block is invalid
    if (deepest_block == NULL)
        return;

    // now block gets added to blockchain
    // balances will be updated only if branch was changed
    if (deepest_block->depth == blockchain.current_block->depth){

        // broadcase own block

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
            sim->add_event(ev);
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else if(deepest_block->depth + 1 == blockchain.current_block->depth){

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
            sim->add_event(ev);
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else if (deepest_block->depth  < blockchain.current_block->depth){

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            if(privateBlock->depth<=deepest_block->depth){
                Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
                sim->add_event(ev);
            }
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else{
        // change peer state to just before block insertion
        balances = current_balance_change;
        for (Transaction* txn : txns_to_add)
            txn_pool.insert(txn);
        for (Transaction* txn : txns_to_remove)
            txn_pool.erase(txn);

        stack<Block*> order;
        order.push(deepest_block);

        while (order.top() != block)
            order.push(order.top()->parent);

        while (!order.empty()) {
            Block* b = order.top();
            order.pop();
            add_block(b, true);
            blocks_to_add.erase(b);
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

        if (next_mining_event != NULL) {
            sim->delete_event(next_mining_event);
            delete next_mining_block;
            schedule_next_block(sim);
        }
    }
}

void StubbornAttacker::receive_block(Simulator* sim, Peer* sender, Block* block) {
    custom_map<int, Block*>::iterator chain_it, free_it;
    custom_unordered_set<int>::iterator reject_it;

    chain_it = chain_blocks.find(block->id);
    free_it = free_blocks.find(block->id);
    reject_it = rejected_blocks.find(block->id);

    // already received this block
    if (chain_it != chain_blocks.end() || free_it != free_blocks.end() || reject_it != rejected_blocks.end()) 
        return;

    block_arrival_times.emplace_back(make_pair(block, sim->current_timestamp));
    
    // attacker doesn't forward honest blocks
    // Event* ev = new ForwardBlock(0, this, sender, block->clone());
    // sim->add_event(ev);
    
    chain_it = chain_blocks.find(block->parent_id);

    // block parent not in our blockchain
    if (chain_it == chain_blocks.end()) {
        block->reset_parent();
        free_blocks[block->id] = block;
        free_block_parents[block->parent_id].emplace_back(block);
        return;
    }

    block->set_parent(chain_it->second);

    Block* current_block = blockchain.current_block; // last block in the blockchain
    Block* branch_block = block->parent; // add the new block as a child of branch block

    // balances to update in case longest chain changes
    vector<int> current_balance_change(total_peers, 0); 
    // txns to add to the txn pool in case longest chain changes
    vector<Transaction*> txns_to_add; 
    // find lca
    while (current_block->depth > branch_block->depth)
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
    
    // balances to update in case longest chain changes
    vector<int> branch_balance_change(total_peers, 0);
    // txns to remove from the txn pool in case longest chain changes
    vector<Transaction*> txns_to_remove;
    while (branch_block->depth > current_block->depth)
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);

    while (branch_block->id != current_block->id) {
        current_block = Blockchain::backward(current_block, current_balance_change, txns_to_add);
        branch_block = Blockchain::backward(branch_block, branch_balance_change, txns_to_remove);
    }

    // current_balance_change = balances just before block insertion point
    for (int i = 0; i < total_peers; i++)
        current_balance_change[i] += balances[i] - branch_balance_change[i];

    custom_unordered_set<Block*> blocks_to_add;
    Block* deepest_block = NULL;

    free_blocks_dfs(block, current_balance_change, blocks_to_add, deepest_block, sim);

    // block is invalid
    if (deepest_block == NULL)
        return;

    // now block gets added to blockchain
    // balances will be updated only if branch was changed
    if (deepest_block->depth == blockchain.current_block->depth){

        // broadcase own block

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
            sim->add_event(ev);
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else if(deepest_block->depth + 1 == blockchain.current_block->depth){

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            if(privateBlock->depth<=deepest_block->depth){
                Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
                sim->add_event(ev);
            }
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else if (deepest_block->depth  < blockchain.current_block->depth){

        Block *privateBlock = blockchain.current_block;
        while(privateBlock->depth>=block->depth){
            if(privateBlock->depth<=deepest_block->depth){
                Event* ev = new ForwardBlock(0, this, this, privateBlock->clone());
                sim->add_event(ev);
            }
            privateBlock = privateBlock->parent;
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

    }else{
        // change peer state to just before block insertion
        balances = current_balance_change;
        for (Transaction* txn : txns_to_add)
            txn_pool.insert(txn);
        for (Transaction* txn : txns_to_remove)
            txn_pool.erase(txn);

        stack<Block*> order;
        order.push(deepest_block);

        while (order.top() != block)
            order.push(order.top()->parent);

        while (!order.empty()) {
            Block* b = order.top();
            order.pop();
            add_block(b, true);
            blocks_to_add.erase(b);
        }

        for (Block* b : blocks_to_add)
            add_block(b, false);

        if (next_mining_event != NULL) {
            sim->delete_event(next_mining_event);
            delete next_mining_block;
            schedule_next_block(sim);
        }
    }
}