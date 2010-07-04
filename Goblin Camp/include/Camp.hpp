#pragma once

#include "Coordinate.hpp"

class Camp {
private:
	Camp();
	static Camp* instance;
	Coordinate center;
	int buildings;
public:
	static Camp* Inst();
	Coordinate Center();
	void UpdateCenter(Coordinate);
};