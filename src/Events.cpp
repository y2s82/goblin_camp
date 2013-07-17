/* Copyright 2010-2011 Ilkka Halila
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

#if DEBUG
#include "iostream"
#endif
#include "Random.hpp"
#include "Announce.hpp"
#include "Events.hpp"
#include "Game.hpp"
#include "GCamp.hpp"
#include "Camp.hpp"
#include "StockManager.hpp"
#include "data/Config.hpp"
#include "Faction.hpp"

Events::Events(Map* vmap) :
	map(vmap),
	hostileSpawningMonsters(std::vector<int>()),
	timeSinceHostileSpawn(0),
	peacefulAnimals(std::vector<int>()),
	migratingAnimals(std::vector<int>())
{
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find("attacksrandomly") != NPC::Presets[i].tags.end())
			hostileSpawningMonsters.push_back(i);
		if (NPC::Presets[i].tags.find("localwildlife") != NPC::Presets[i].tags.end())
			peacefulAnimals.push_back(i);
		if (NPC::Presets[i].tags.find("immigrant") != NPC::Presets[i].tags.end())
			immigrants.push_back(i);
		if (NPC::Presets[i].tags.find("migratory") != NPC::Presets[i].tags.end())
			migratingAnimals.push_back(i);
		}
}

void Events::Update(bool safe) {
	if (!safe) {
		++timeSinceHostileSpawn;
		if (Random::Generate(UPDATES_PER_SECOND * 60 * 15 - 1) == 0 || timeSinceHostileSpawn > (UPDATES_PER_SECOND * 60 * 25)) {
			SpawnHostileMonsters();
		}
	}

	if (Random::Generate(UPDATES_PER_SECOND * 60 * 2 - 1) == 0) {
		SpawnBenignFauna();
	}

	//Remove immigrants that have left/died
	for (std::vector<boost::weak_ptr<NPC> >::iterator immi = existingImmigrants.begin(); immi != existingImmigrants.end();) {
		if (!immi->lock()) immi = existingImmigrants.erase(immi);
		else ++immi;
	}

	if (static_cast<int>(existingImmigrants.size()) < Game::Inst()->OrcCount() / 7 && 
		Random::Generate(UPDATES_PER_SECOND * 60 * 30) == 0) {
			SpawnImmigrants();
	}
	
	Season cSeason = Game::Inst()->CurrentSeason();
	if ((cSeason == EarlySpring ||
		cSeason == Spring ||
		cSeason == LateSpring ||
		cSeason == EarlyFall ||
		cSeason == Fall ||
		cSeason == LateFall) && Random::Generate(UPDATES_PER_SECOND * 60 * 30) == 0) {
		SpawnMigratingAnimals();
	}
}

namespace {
	void GenerateEdgeCoordinates(Map* map, Coordinate& a, Coordinate& b) {
		int counter = 0;
		do {
			switch (Random::Generate(3)) {
			case 0:
				a.X(0);
				a.Y(Random::Generate(map->Height() - 21));
				b.X(1);
				b.Y(a.Y() + 20);
				break;

			case 1:
				a.X(Random::Generate(map->Width() - 21));
				a.Y(0);
				b.X(a.X() + 20);
				b.Y(1);
				break;

			case 2:
				a.X(map->Width() - 2);
				a.Y(Random::Generate(map->Height() - 21));
				b.X(map->Width() - 1);
				b.Y(a.Y() + 20);
				break;

			case 3:
				a.X(Random::Generate(map->Width() - 21));
				a.Y(map->Height() - 2);
				b.X(a.X() + 20);
				b.Y(map->Height() - 1);
				break;
			}
			++counter;
		} while ((Map::Inst()->IsUnbridgedWater(a) || Map::Inst()->IsUnbridgedWater(b)) && counter < 100);
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

	if (!possibleMonsters.empty()) {
		NPCType monsterType = Random::ChooseElement(possibleMonsters);
		int hostileSpawnCount = Game::DiceToInt(NPC::Presets[monsterType].group);

		std::string msg;
		if (hostileSpawnCount > 1) 
			msg = (boost::format("%s have been sighted outside your %s!") 
			% NPC::Presets[monsterType].plural % Camp::Inst()->GetName()).str();
		else msg = (boost::format("A %s has been sighted outside your %s!")
			% NPC::Presets[monsterType].name % Camp::Inst()->GetName()).str();

		Coordinate a, b;
		GenerateEdgeCoordinates(map, a, b);

		Game::Inst()->CreateNPCs(hostileSpawnCount, monsterType, a, b);
		Announce::Inst()->AddMsg(msg, TCODColor::red, Coordinate((a.X() + b.X()) / 2, (a.Y() + b.Y()) / 2));
		timeSinceHostileSpawn = 0;
		if (Config::GetCVar<bool>("pauseOnDanger")) 
			Game::Inst()->AddDelay(UPDATES_PER_SECOND, boost::bind(&Game::Pause, Game::Inst()));
	}
}

void Events::SpawnBenignFauna() {
	if (peacefulAnimals.size() > 0 && Game::Inst()->PeacefulFaunaCount() < 20) {
		//Generate benign fauna
		std::vector<NPCType> possibleFauna;
		for (std::vector<int>::iterator iter = peacefulAnimals.begin(); iter != peacefulAnimals.end(); ++iter) {
			if (NPC::Presets[*iter].tier <= Camp::Inst()->GetTier())
				possibleFauna.push_back((NPCType)*iter);
		}

		for (int i = 0; i < Random::Generate(1, 10); ++i) {
			unsigned type = Random::ChooseIndex(possibleFauna);
			Coordinate target;
			do {
				target = Random::ChooseInExtent(map->Extent());
			} while (!map->IsWalkable(target) || Distance(Camp::Inst()->Center(), target) < 100
					 || (map->GetType(target) != TILEGRASS && map->GetType(target) != TILESNOW && map->GetType(target) != TILEMUD));
			Game::Inst()->CreateNPC(target, possibleFauna[type]);
		}
	}
}

void Events::SpawnImmigrants() {
	std::vector<NPCType> possibleImmigrants;
	for (std::vector<int>::iterator immi = immigrants.begin(); immi != immigrants.end(); ++immi) {
		if (NPC::Presets[*immi].tier <= Camp::Inst()->GetTier()) {
			possibleImmigrants.push_back((NPCType)*immi);
		}
	}

	if (!possibleImmigrants.empty()) {
		NPCType monsterType = Random::ChooseElement(possibleImmigrants);
		int spawnCount = Game::DiceToInt(NPC::Presets[monsterType].group);

		std::string msg;
		if (spawnCount > 1) 
			msg = (boost::format("%s join your %s!") 
			% NPC::Presets[monsterType].plural % Camp::Inst()->GetName()).str();
		else msg = (boost::format("A %s joins your %s!")
			% NPC::Presets[monsterType].name % Camp::Inst()->GetName()).str();

		Coordinate a, b;
		GenerateEdgeCoordinates(map, a, b);

		for (int i = 0; i < spawnCount; ++i) {
			int npcUid = Game::Inst()->CreateNPC(Random::ChooseInRectangle(a, b), monsterType);
			existingImmigrants.push_back(Game::Inst()->GetNPC(npcUid));
		}

		Announce::Inst()->AddMsg(msg, TCODColor(0,150,255), (a + b) / 2);
	}
}

void Events::SpawnMigratingAnimals() {
	if (!migratingAnimals.empty()) {
		NPCType monsterType = Random::ChooseElement(migratingAnimals);
		int migrationSpawnCount = Game::DiceToInt(NPC::Presets[monsterType].group);
		if (migrationSpawnCount == 0) {
			return;
		}
		
		// Migrations are usually much bigger than your average group, so time number by 3
		migrationSpawnCount *= 3;
		
		int tries = 0;
		Coordinate a,b;
		do {
			GenerateEdgeCoordinates(map, a, b);
		} while (!Map::Inst()->IsWalkable(a));

#if DEBUG
		std::cout<< "Migration entry at " << a.X() << "x" << a.Y()<<"\n";
#endif
		
		int tuid = Game::Inst()->CreateNPC(a, monsterType);
		migrationSpawnCount--;
#if DEBUG
		std::cout<< "Migration spawning " << migrationSpawnCount<< " " << monsterType << "\n";
#endif
		
		NPC *firstNPC = Game::Inst()->GetNPC(tuid).get();
		int x, y;
		int halfMapWidth, halfMapHeight;
		halfMapWidth = Map::Inst()->Width() / 2;
		halfMapHeight = Map::Inst()->Height() / 2;
		
		// On the left
		if (a.X() < 5) {
			while (tries < 20) {
				// Try the right side first
				x = Map::Inst()->Width() - 1;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try the bottom side
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = 0;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try top
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = Map::Inst()->Height() - 1;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
			}
		// Right side
		} else if (a.X() > Map::Inst()->Width() - 5) {
			while (tries < 20) {
				// Try the left side first
				x = 0;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try the bottom side
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = 0;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try top
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = Map::Inst()->Height() - 1;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
			}
		// Bottom side
		} else if (a.Y() < 5) {
			while (tries < 20) {
				// Try the left side first
				x = 0;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try the right side
				x = Map::Inst()->Width() - 1;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try top
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = Map::Inst()->Height() - 1;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
			}
		// Top
		} else if (a.Y() > Map::Inst()->Height() - 5) {
			while (tries < 20) {
				// Try the left side first
				x = 0;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try the right side
				x = Map::Inst()->Width() - 1;
				y = halfMapHeight + Random::Generate(-halfMapHeight, halfMapHeight);
				firstNPC->findPath(Coordinate(x, y));
				if (firstNPC->IsPathWalkable()) break;
				tries++;
				
				// Try the bottom side
				x = halfMapWidth + Random::Generate(-halfMapWidth, halfMapWidth);
				y = 0;
				firstNPC->findPath(Coordinate(x, y));
				if(firstNPC->IsPathWalkable()) break;
				tries++;
			}
		} else {
#if DEBUG
			std::cout << "Could not run migration, coordinates from GenerateEdgeCoordinates were not on edge\n";
#endif
			return;
		}
		
		if (tries > 20) {
#if DEBUG
			std::cout << "Could not find a destination for a migration, tried "<<tries<<" times.\n";
#endif
			return;
		}
		
#if DEBUG
		std::cout << "Migration exit at " << x << "x" << y << "\n";
#endif
		
		std::vector<NPC*> migrants;
		std::vector<int> uids = Game::Inst()->CreateNPCs(migrationSpawnCount, monsterType, a, b);
		
		for(std::vector<int>::iterator uidi = uids.begin(); uidi != uids.end(); uidi++) {
			boost::shared_ptr<NPC> ptr = Game::Inst()->GetNPC(*uidi);
			if (!ptr) continue;
			migrants.push_back(ptr.get());
		}
		migrants.push_back(firstNPC);
		if (migrants.empty()) {
			return;
		}
		
		// Create jobs for the migration
		for(std::vector<NPC*>::iterator mgrnt = migrants.begin();
			mgrnt != migrants.end(); mgrnt++) {
			boost::shared_ptr<Job> migrateJob(new Job("Migrate"));
			
			// This is so they don't all disapear into one spot.
			int fx, fy;
			if (x < 5 || x > Map::Inst()->Width() - 5) {
				fx = 0;
				fy = y + Random::Generate(-5, 5);
			} else {
				fy = 0;
				fx = x + Random::Generate(-5, 5);
			}
			
			migrateJob->tasks.push_back(Task(MOVENEAR, Coordinate(fx, fy)));
			migrateJob->tasks.push_back(Task(FLEEMAP));
			(*mgrnt)->StartJob(migrateJob);
		}

		std::string msg;
		msg = (boost::format("A %s migration is occurring outside your %s.") % NPC::Presets[monsterType].name
			% Camp::Inst()->GetName()).str();
		
		Announce::Inst()->AddMsg(msg, TCODColor::green, (a+b)/2);
#if DEBUG
		std::cout << "Migration underway.\n";
#endif
	}
}
