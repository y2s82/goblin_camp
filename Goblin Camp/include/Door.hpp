#pragma once

#include "Construction.hpp"

class Door : public Construction {
private:
	int closedGraphic;
	int timer;
public:
	Door(ConstructionType, Coordinate);
	virtual void Update();
};