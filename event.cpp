#include "declarations.h"
using namespace std;

// ======================================================================= //
Event::Event(ld timestamp_) {
    timestamp = timestamp_;
}

bool Event::operator<(const Event& other) {
    return timestamp < other.timestamp;
}

void Event::run(Simulator* sim) {
	assert(false);
}

// ======================================================================= //
GenerateTransaction::GenerateTransaction(ld timestamp_, Peer* p) : Event(timestamp_) {
    payed_by = p;
}

void GenerateTransaction::run(Simulator* sim) {
    Transaction* txn = payed_by->generate_transaction(sim);
	if (txn != NULL) {
		sim->log_time(cout);
		cout << payed_by->get_name() << " generates and emits transaction " << txn->get_name() << '\n';
	}
    // cout << "GenerateTransaction at peer " << payed_by->id << " " << "at time " << timestamp << "\n";
}

// ======================================================================= //
ForwardTransaction::ForwardTransaction(ld timestamp_, Peer* peer_, Peer* source_, Transaction* txn_) : Event(timestamp_) {
    peer = peer_;
    source = source_;
    txn = txn_;
}

void ForwardTransaction::run(Simulator* sim) {
	if (peer->id != source->id) {
		sim->log_time(cout);
		cout << peer->get_name() << " forwards transaction " << txn->get_name() << " received from " << source->get_name() << '\n';
    }
	// cout << "ForwardTransaction at peer " << peer->id << " with source from " << source->id << " at time " << timestamp << "\n";
	peer->forward_transaction(sim, source, txn);
}

// ======================================================================= //
ReceiveTransaction::ReceiveTransaction(ld timestamp_, Peer* sender_, Peer* receiver_, Transaction* txn_) : Event(timestamp_) {
    sender = sender_;
    receiver = receiver_;
    txn = txn_;
}

void ReceiveTransaction::run(Simulator* sim) {
	sim->log_time(cout);
	cout << receiver->get_name() << " receives transaction " << txn->get_name() << " from " << sender->get_name() << '\n';
    // cout << "ReceiveTransaction at peer " << receiver->id << " with source from " << sender->id << " at time " << timestamp << "\n";
    receiver->receive_transaction(sim, sender, txn);
}

// ======================================================================= //
BroadcastMinedBlock::BroadcastMinedBlock(ld timestamp, Peer* p) : Event(timestamp) {
	owner = p;
}

void BroadcastMinedBlock::run(Simulator* sim) {
	Block* block = owner->next_mining_block;
	assert(owner->blockchain->current_block->id == block->parent->id);
	bool is_valid = owner->validate_block(block, owner->balances);
	if (is_valid)
		owner->blockchain->add(block);

	sim->log_time(cout);
	cout << owner->get_name() << " mines and broadcasts block " << block->get_name() << '\n';

	Event* ev = new ForwardBlock(0, owner, owner, block);
	sim->add_event(ev);

	owner->schedule_next_block(sim);
}

// ======================================================================= //
ForwardBlock::ForwardBlock(ld timestamp_, Peer* peer_, Peer* source_, Block* block_) : Event(timestamp_) {
    peer = peer_;
    source = source_;
    block = block_;
}

void ForwardBlock::run(Simulator* sim) {
	if (peer->id != source->id) {
		sim->log_time(cout);
		cout << peer->get_name() << " forwards block " << block->get_name() << " received from " << source->get_name() << '\n';
	}
	peer->forward_block(sim, source, block);
}

// ======================================================================= //
ReceiveBlock::ReceiveBlock(ld timestamp_, Peer* sender_, Peer* receiver_, Block* block_) : Event(timestamp_) {
    sender = sender_;
    receiver = receiver_;
    block = block_;
}

void ReceiveBlock::run(Simulator* sim) {
	sim->log_time(cout);
	cout << receiver->get_name() << " receives block " << block->get_name() << " from " << sender->get_name() << '\n';
    receiver->receive_block(sim, sender, block);
}