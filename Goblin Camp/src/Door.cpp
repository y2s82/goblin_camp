#include "Door.hpp"
#include "Map.hpp"
#include "GCamp.hpp"

Door::Door(ConstructionType type, Coordinate target) : Construction(type, target),
timer(0)
{
	closedGraphic = graphic[1];
}

void Door::Update() {
	if (!Map::Inst()->NPCList(_x, _y)->empty()) {
		graphic[1] = 224;
		timer = (UPDATES_PER_SECOND * 1);
	} else {
		if (timer == 0) graphic[1] = closedGraphic;
		else --timer;
	}
}