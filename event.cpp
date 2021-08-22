#include "declarations.h"
using namespace std;

Event::Event(ld timestamp_){
	timestamp = timestamp_;
}

bool Event::operator<(const Event& other) {
	return timestamp < other.timestamp;
}

void Event::run(Simulator* sim) {
	
}

GenerateTransaction::GenerateTransaction(ld timestamp_, int peer_id_): Event(timestamp_){
	peer_id = peer_id_;
}

void GenerateTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->peer_id].generate_transaction(this->timestamp);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}

ForwardTransaction::ForwardTransaction(ld timestamp_, Peer* peer_, Peer* source_, Transaction* txn_): Event(timestamp_){
	peer = peer_;
	source = source_;
	txn = txn_;
}

void ForwardTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->peer_id].forward_transaction(this->timestamp, this->source, this->txn);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}

ReceiveTransaction::ReceiveTransaction(ld timestamp_, Peer* sender_, Peer* receiver_, Transaction* txn_): Event(timestamp_){
	sender = sender_;
	receiver = receiver_;
	txn = txn_;
}

void ReceiveTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->peer_id].receive_transaction(this->timestamp, this->sender, this->txn);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}