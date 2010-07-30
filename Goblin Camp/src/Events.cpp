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
#include "stdafx.hpp"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "Announce.hpp"
#include "Events.hpp"
#include "Game.hpp"
#include "GCamp.hpp"

Events::Events(Map* vmap) :
hostileSpawningMonsters(std::vector<int>()),
	map(vmap)
{
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find("attackrandomly") != NPC::Presets[i].tags.end())
			hostileSpawningMonsters.push_back(i);
	}
}

//Pretty much just placeholder stuff until I get a proper events system in place

void Events::Update() {
	if (rand() % (UPDATES_PER_SECOND * 60 * 15) == 0) {
		int hostileID = rand() % hostileSpawningMonsters.size();
		NPCType monsterType = hostileSpawningMonsters[hostileID];
		int hostileSpawnCount = Game::DiceToInt(NPC::Presets[monsterType].group);

		std::string msg = (boost::format("%s have been sighted outside your settlement!") 
			% NPC::Presets[monsterType].plural).str();
		Announce::Inst()->AddMsg(msg, TCODColor::red);
		Coordinate a((rand() % map->Width())-20, 0);
		Coordinate b = a + Coordinate(20, 1);
		Game::Inst()->CreateNPCs(hostileSpawnCount, monsterType, a, b);
	}
}