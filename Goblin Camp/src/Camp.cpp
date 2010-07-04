#include "Camp.hpp"
#include "Coordinate.hpp"

Camp* Camp::instance = 0;

Camp::Camp() :
center(Coordinate(220,220)),
	buildings(0)
{}

Camp* Camp::Inst() {
	if (!instance) instance = new Camp();
	return instance;
}

Coordinate Camp::Center() {
	return center;
}

void Camp::UpdateCenter(Coordinate newBuilding) {
	center.x((center.x() * buildings) + newBuilding.x());
	center.y((center.y() * buildings) + newBuilding.y());
	++buildings;
	center.x(center.x() / buildings);
	center.y(center.y() / buildings);
}