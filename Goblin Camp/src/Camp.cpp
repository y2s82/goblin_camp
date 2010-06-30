#include "Camp.hpp"
#include "Coordinate.hpp"

Camp* Camp::instance = 0;

Camp::Camp() {}

Camp* Camp::Inst() {
	if (!instance) instance = new Camp();
	return instance;
}

Coordinate Camp::MeetingPoint() {
	return Coordinate(220,220);
}