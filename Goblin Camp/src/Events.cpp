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
#include "Camp.hpp"

Events::Events(Map* vmap) :
map(vmap),
	hostileSpawningMonsters(std::vector<int>()),
	timeSinceHostileSpawn(0),
	peacefulAnimals(std::vector<int>())
{
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find("attacksrandomly") != NPC::Presets[i].tags.end())
			hostileSpawningMonsters.push_back(i);
		if (NPC::Presets[i].tags.find("localwildlife") != NPC::Presets[i].tags.end())
			peacefulAnimals.push_back(i);
	}
}

void Events::Update(bool safe) {
	if (!safe) ++timeSinceHostileSpawn;
	if (!safe && (rand() % (UPDATES_PER_SECOND * 60 * 15) == 0 || timeSinceHostileSpawn > UPDATES_PER_SECOND * 60 * 25)) {
		SpawnHostileMonsters();
	}

	if (rand() % (UPDATES_PER_SECOND * 60 * 2) == 0) {
		SpawnBenignFauna();
	}
}

void Events::SpawnHostileMonsters() {
	std::vector<NPCType> possibleMonsters;
	for (std::vector<int>::iterator hosti = hostileSpawningMonsters.begin(); hosti != hostileSpawningMonsters.end(); ++hosti) {
		if (NPC::Presets[*hosti].tier <= Camp::Inst()->GetTier() - 2) {
			possibleMonsters.push_back((NPCType)*hosti);
		} else if (NPC::Presets[*hosti].tier <= Camp::Inst()->GetTier()) {
			possibleMonsters.push_back((NPCType)*hosti); // This is intentional, it raises the odds that monsters at or lower
			possibleMonsters.push_back((NPCType)*hosti); // than this tier are spawned vs. one tier higher.
		} else if (NPC::Presets[*hosti].tier == Camp::Inst()->GetTier() + 1) {
			possibleMonsters.push_back((NPCType)*hosti);
		}
	}

	NPCType monsterType = possibleMonsters[rand() % possibleMonsters.size()];
	int hostileSpawnCount = Game::DiceToInt(NPC::Presets[monsterType].group);

	std::string msg;
	if (hostileSpawnCount > 1) 
		msg = (boost::format("%s have been sighted outside your settlement!") 
		% NPC::Presets[monsterType].plural).str();
	else msg = (boost::format("A %s has been sighted outside your settlement!")
		% NPC::Presets[monsterType].name).str();

	Coordinate a,b;
	int counter = 0;
	do {
		switch (rand() % 4) {
		case 0:
			a.X(0);
			a.Y(rand() % (map->Height() - 20));
			b.X(1);
			b.Y(a.Y() + 20);
			break;

		case 1: 
			a.X(rand() % (map->Width() - 20));
			a.Y(0);
			b.X(a.X() + 20);
			b.Y(1);
			break;

		case 2:
			a.X(map->Width() - 2);
			a.Y(rand() % (map->Height() - 20));
			b.X(map->Width() - 1);
			b.Y(a.Y() + 20);
			break;

		case 3:
			a.X(rand() % (map->Width() - 20));
			a.Y(map->Height() - 2);
			b.X(a.X() + 20);
			b.Y(map->Height() - 1);
			break;
		}
		++counter;
	} while ((Map::Inst()->IsUnbridgedWater(a.X(), a.Y()) || Map::Inst()->IsUnbridgedWater(b.X(), b.Y())) && counter < 100);

	Game::Inst()->CreateNPCs(hostileSpawnCount, monsterType, a, b);
	Announce::Inst()->AddMsg(msg, TCODColor::red, Coordinate((a.X() + b.X()) / 2, (a.Y() + b.Y()) / 2));
	timeSinceHostileSpawn = 0;
}

void Events::SpawnBenignFauna() {
	if (peacefulAnimals.size() > 0 && Game::Inst()->PeacefulFaunaCount() < 20) {
		//Generate benign fauna
		for (int i = 0; i < (rand() % 10) + 1; ++i) {
			int type = rand() % peacefulAnimals.size();
			Coordinate target;
			do {
				target.X(rand() % map->Width());
				target.Y(rand() % map->Height());
			} while (!map->Walkable(target.X(), target.Y()) || Distance(Camp::Inst()->Center(), target) < 100
				|| map->Type(target.X(), target.Y()) != TILEGRASS);
			Game::Inst()->CreateNPC(target, peacefulAnimals[type]);
		}
	}
}