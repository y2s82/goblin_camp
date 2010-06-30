#pragma once

#include "Coordinate.hpp"

class Camp {
private:
	Camp();
	static Camp* instance;
public:
	static Camp* Inst();
	Coordinate MeetingPoint();
};