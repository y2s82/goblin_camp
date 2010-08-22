/* Copyright 2010 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#pragma once

#include <queue>
#include <string>
#include <sstream>

#include <libtcod.hpp>

#include "Coordinate.hpp"

#define ANNOUNCE_MAX_LENGTH 71
#define ANNOUNCE_HEIGHT 10

class AnnounceMessage {
public:
	AnnounceMessage(std::string, TCODColor = TCODColor::white, Coordinate = Coordinate(-1,-1));
	std::stringstream result;
	std::string msg;
	int counter;
	TCODColor color;
	Coordinate target;
	std::string ToString();
};

class Announce {
private:
	Announce();
	static Announce* instance;
	std::deque<AnnounceMessage*> messageQueue;
	std::deque<AnnounceMessage*> history;
	int timer;
	unsigned int length, height;
public:
	static Announce* Inst();
	void Reset();
	void AddMsg(std::string, TCODColor = TCODColor::white);
	void Update();
	void Draw(TCODConsole*);
	void Draw(Coordinate, int from, int amount, TCODConsole*);
	int AnnounceAmount();
	void EmptyMessageQueue();
	Coordinate CurrentCoordinate();
};
