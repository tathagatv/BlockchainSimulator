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

ForwardTransaction::ForwardTransaction(ld timestamp_, int peer_id_, int source_id_, Transaction* txn_): Event(timestamp_){
	peer_id = peer_id_;
	source_id = source_id_;
	txn = txn_;
}

void ForwardTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->peer_id].forward_transaction(this->timestamp, this->source_id, this->txn);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}

ReceiveTransaction::ReceiveTransaction(ld timestamp_, int sender_id_, int receiver_id_, Transaction* txn_): Event(timestamp_){
	sender_id = sender_id_;
	receiver_id = receiver_id_;
	txn = txn_;
}

void ReceiveTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->receiver_id].receive_transaction(this->timestamp, this->sender_id, this->txn);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}