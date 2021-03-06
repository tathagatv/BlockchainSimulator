#include "declarations.h"
using namespace std;

// ======================================================================= //
/* constructor */
Event::Event(ld timestamp_) {
    timestamp = timestamp_;
    is_generate_type_event = false;  // default value false
}

/* compare based on timestamp */
bool Event::operator<(const Event& other) {
    if (timestamp != other.timestamp) {
        return timestamp < other.timestamp;
    } else {
        // two different events with same timestamp should not be considered as equal
        return this < &other;
    }
}

/* execute the event, definition in derived classes */
void Event::run(Simulator* sim) {
    assert(false);
}

// ======================================================================= //
GenerateTransaction::GenerateTransaction(ld timestamp_, Peer* p) : Event(timestamp_) {
    payed_by = p;
    is_generate_type_event = true;
}

void GenerateTransaction::run(Simulator* sim) {
    Transaction* txn = payed_by->generate_transaction(sim);
    if (txn != NULL) {
        // sim->log(cout, payed_by->get_name() + " generates and emits transaction " + txn->get_name());
    }
}

// ======================================================================= //
ForwardTransaction::ForwardTransaction(ld timestamp_, Peer* peer_, Peer* source_, Transaction* txn_) : Event(timestamp_) {
    peer = peer_;
    source = source_;
    txn = txn_;
}

void ForwardTransaction::run(Simulator* sim) {
    // if (peer->id != source->id)
    // sim->log(cout, peer->get_name() + " forwards transaction " + txn->get_name() + " received from " + source->get_name());
    peer->forward_transaction(sim, source, txn);
}

// ======================================================================= //
ReceiveTransaction::ReceiveTransaction(ld timestamp_, Peer* sender_, Peer* receiver_, Transaction* txn_) : Event(timestamp_) {
    sender = sender_;
    receiver = receiver_;
    txn = txn_;
}

void ReceiveTransaction::run(Simulator* sim) {
    // sim->log(cout, receiver->get_name() + " receives transaction " + txn->get_name() + " from " + sender->get_name());
    receiver->receive_transaction(sim, sender, txn);
}

// ======================================================================= //
BroadcastMinedBlock::BroadcastMinedBlock(ld timestamp, Peer* p) : Event(timestamp) {
    owner = p;
    is_generate_type_event = true;
}

void BroadcastMinedBlock::run(Simulator* sim) {
    owner->broadcast_mined_block(sim);
}

// ======================================================================= //
ForwardBlock::ForwardBlock(ld timestamp_, Peer* peer_, Peer* source_, Block* block_) : Event(timestamp_) {
    peer = peer_;
    source = source_;
    block = block_;
}

void ForwardBlock::run(Simulator* sim) {
    // if (peer->id != source->id)
    // sim->log(cout, peer->get_name() + " forwards block " + block->get_name() + " received from " + source->get_name());
    peer->forward_block(sim, source, block);
}

// ======================================================================= //
ReceiveBlock::ReceiveBlock(ld timestamp_, Peer* sender_, Peer* receiver_, Block* block_) : Event(timestamp_) {
    sender = sender_;
    receiver = receiver_;
    block = block_;
}

void ReceiveBlock::run(Simulator* sim) {
    // sim->log(cout, receiver->get_name() + " receives block " + block->get_name() + " from " + sender->get_name());
    receiver->receive_block(sim, sender, block);
}