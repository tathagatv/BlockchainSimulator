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

SendTransaction::SendTransaction(ld timestamp_, int peer_id_): Event(timestamp_){
	peer_id = peer_id_;
}

void SendTransaction::run(Simulator *sim){
	set<Event*> new_events = sim->peers[this->peer_id].send_transaction(this->timestamp);
	for(Event* ev: new_events){
		sim->events.insert(ev);
	}
	return;
}

ReceiveTransaction::ReceiveTransaction(ld timestamp_, Peer* peer_, Transaction* txn_): Event(timestamp_){
	peer = peer_;
	txn = txn_;
}

void ReceiveTransaction::run(Simulator *sim){
	// set<Event*> new_events = sim->peers[this->peer_id].send_transaction(this->timestamp);
	// for(Event* ev: new_events){
	// 	sim->events.insert(ev);
	// }
	return;
}
