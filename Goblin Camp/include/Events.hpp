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

#include "Map.hpp"

/*Events will include all the hardcoded events. Once Python is figured
out events will be able to be defined in py code, and the hardcoded ones
should be possible to turn off if desired.
I want to have the basic ones coded in c++, python will necessarily be
a bit slower, and it might make a crucial difference on low-end machines. */

class Events {
private:
	Map *map;
	std::vector<int> hostileSpawningMonsters;
	int timeSinceHostileSpawn;
	std::vector<int> peacefulAnimals;
public:
	Events(Map*);
	void Update(bool safe = false);
	void SpawnHostileMonsters();
	void SpawnBenignFauna();
};
