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
		int monsterType = rand() % hostileSpawningMonsters.size();
		monsterType = hostileSpawningMonsters[monsterType];
		std::string msg = (boost::format("%s have been sighted outside your settlement!") 
			% NPC::Presets[monsterType].plural).str();
		Announce::Inst()->AddMsg(msg, TCODColor::red);
		for (int i = 0; i < Game::DiceToInt(NPC::Presets[monsterType].group); ++i) {
			Game::Inst()->CreateNPC(Coordinate(rand() % map->Width(),0), monsterType);
		}
	}

	if (rand() % (UPDATES_PER_SECOND * 60 * 7) == 0) {
		if (rand() % 2 == 0 && Game::Inst()->OrcCount() < 50) {
			Announce::Inst()->AddMsg("An orc has joined your camp", TCODColor::azure);
			Game::Inst()->CreateNPC(Coordinate(rand() % map->Width(),0), NPC::StringToNPCType("orc"));
		} else if (Game::Inst()->GoblinCount() < 100) {
			Announce::Inst()->AddMsg("A goblin has joined your camp", TCODColor::azure);
			Game::Inst()->CreateNPC(Coordinate(rand() % map->Width(),0), NPC::StringToNPCType("goblin"));
		}
	}
}