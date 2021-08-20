#include "declarations.h"
using namespace std;

bool Event::operator<(const Event& other) {
	return timestamp < other.timestamp;
}
