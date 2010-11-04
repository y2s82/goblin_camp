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

#include "SpawningPool.hpp"
#include "UI\Button.hpp"
#include "GCamp.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"
#include "Announce.hpp"

SpawningPool::SpawningPool(ConstructionType type, Coordinate target) : Construction(type, target),
	dumpFilth(false),
	dumpCorpses(false),
	a(target),
	b(target),
	expansion(0),
	filth(0),
	corpses(0),
	spawns(0),
	corpseContainer(boost::shared_ptr<Container>())
{
	container = new UIContainer(std::vector<Drawable*>(), 0, 0, 16, 11);
	dialog = new Dialog(container, "Spawning Pool", 16, 10);
	container->AddComponent(new ToggleButton("Dump filth", boost::bind(&SpawningPool::ToggleDumpFilth, this), 
		boost::bind(&SpawningPool::DumpFilth, this), 2, 2, 12));
	container->AddComponent(new ToggleButton("Dump corpses", boost::bind(&SpawningPool::ToggleDumpCorpses, this), 
		boost::bind(&SpawningPool::DumpCorpses, this), 1, 6, 14));
	corpseContainer = boost::shared_ptr<Container>(new Container(target));
}

Panel* SpawningPool::GetContextMenu() {
	return dialog;
}

bool SpawningPool::DumpFilth(SpawningPool* sp) { return sp->dumpFilth; }
void SpawningPool::ToggleDumpFilth(SpawningPool* sp) { sp->dumpFilth = !sp->dumpFilth; }
bool SpawningPool::DumpCorpses(SpawningPool* sp) { return sp->dumpCorpses; }
void SpawningPool::ToggleDumpCorpses(SpawningPool* sp) { sp->dumpCorpses = !sp->dumpCorpses; }

void SpawningPool::Update() {
	if (condition > 0) {

		//Generate jobs

		if (dumpFilth && rand() % (UPDATES_PER_SECOND * 5) == 0) {
			if (Game::Inst()->filthList.size() > 0) {
				boost::shared_ptr<Job> filthDumpJob(new Job("Dump filth", LOW));
				filthDumpJob->tasks.push_back(Task(FIND, Coordinate(0,0), boost::weak_ptr<Entity>(), Item::StringToItemCategory("Barrel"), EMPTY));
				filthDumpJob->tasks.push_back(Task(MOVE));
				filthDumpJob->tasks.push_back(Task(TAKE));
				filthDumpJob->tasks.push_back(Task(FORGET)); //Otherwise MOVEADJACENT will try to move adjacent to the container
				Coordinate filthLocation = Game::Inst()->FindFilth(Position());
				filthDumpJob->tasks.push_back(Task(MOVEADJACENT, filthLocation));
				filthDumpJob->tasks.push_back(Task(FILL, filthLocation));
				Coordinate target(-1,-1);
				for (int x = a.X(); x <= b.X(); ++x) {
					for (int y = a.Y(); y <= b.Y(); ++y) {
						if (Map::Inst()->GetConstruction(x,y) == uid) {
							if (Map::Inst()->Walkable(x-1,y)) {
								target = Coordinate(x-1,y);
								break;
							} else if (Map::Inst()->Walkable(x+1,y)) {
								target = Coordinate(x+1,y);
								break;
							} else if (Map::Inst()->Walkable(x,y+1)) {
								target = Coordinate(x,y+1);
								break;
							} else if (Map::Inst()->Walkable(x,y-1)) {
								target = Coordinate(x,y-1);
								break;
							}
						}
					}
				}

				if (filthLocation.X() != -1 && filthLocation.Y() != -1 && target.X() != -1 && target.Y() != -1) {
					filthDumpJob->tasks.push_back(Task(MOVE, target));
					filthDumpJob->tasks.push_back(Task(POUR, Position()));
					filthDumpJob->tasks.push_back(Task(STOCKPILEITEM));
					JobManager::Inst()->AddJob(filthDumpJob);
				}
			}
		}
		if (dumpCorpses && StockManager::Inst()->CategoryQuantity(Item::StringToItemCategory("Corpse")) > 0 &&
			rand() % (UPDATES_PER_SECOND * 5) == 0) {
				boost::shared_ptr<Job> corpseDumpJob(new Job("Dump corpse", LOW));
				corpseDumpJob->tasks.push_back(Task(FIND, Coordinate(0,0), boost::weak_ptr<Entity>(), Item::StringToItemCategory("Corpse")));
				corpseDumpJob->tasks.push_back(Task(MOVE));
				corpseDumpJob->tasks.push_back(Task(TAKE));

				Coordinate target(-1,-1);
				for (int x = a.X(); x <= b.X(); ++x) {
					for (int y = a.Y(); y <= b.Y(); ++y) {
						if (Map::Inst()->GetConstruction(x,y) == uid) {
							if (Map::Inst()->Walkable(x-1,y)) {
								target = Coordinate(x-1,y);
								break;
							} else if (Map::Inst()->Walkable(x+1,y)) {
								target = Coordinate(x+1,y);
								break;
							} else if (Map::Inst()->Walkable(x,y+1)) {
								target = Coordinate(x,y+1);
								break;
							} else if (Map::Inst()->Walkable(x,y-1)) {
								target = Coordinate(x,y-1);
								break;
							}
						}
					}
				}

				if (target.X() != -1 && target.Y() != -1) {
					corpseDumpJob->tasks.push_back(Task(MOVE, target));
					corpseDumpJob->tasks.push_back(Task(PUTIN, target, corpseContainer));
					JobManager::Inst()->AddJob(corpseDumpJob);
				}
		}

		//Spawn / Expand
		if (Map::Inst()->GetFilth(x, y).lock() && Map::Inst()->GetFilth(x, y).lock()->Depth() > 0) {
			boost::shared_ptr<FilthNode> filthNode = Map::Inst()->GetFilth(x,y).lock();
			filth += filthNode->Depth();
			for (int i = 0; i < 1 + rand() % filthNode->Depth(); ++i) Map::Inst()->Corrupt(x, y);
			filthNode->Depth(0);
		}
		while (!corpseContainer->empty()) {
			++corpses;
			boost::weak_ptr<Item> corpse = corpseContainer->GetFirstItem();
			corpseContainer->RemoveItem(corpse);
			Game::Inst()->RemoveItem(corpse);
			for (int i = 0; i < 1 + rand() % 10; ++i) Map::Inst()->Corrupt(x, y);
		}

		if (corpses + filth > std::min(2 + 2*spawns, (unsigned int)10)) {
			Coordinate spawnLocation(-1,-1);
			for (int x = a.X(); x <= b.X(); ++x) {
				for (int y = a.Y(); y <= b.Y(); ++y) {
					if (Map::Inst()->GetConstruction(x,y) == uid) {
						if (Map::Inst()->Walkable(x-1,y)) {
							spawnLocation = Coordinate(x-1,y);
							break;
						} else if (Map::Inst()->Walkable(x+1,y)) {
							spawnLocation = Coordinate(x+1,y);
							break;
						} else if (Map::Inst()->Walkable(x,y+1)) {
							spawnLocation = Coordinate(x,y+1);
							break;
						} else if (Map::Inst()->Walkable(x,y-1)) {
							spawnLocation = Coordinate(x,y-1);
							break;
						}
					}
				}
			}

			if (spawnLocation.X() != -1 && spawnLocation.Y() != -1) {
				++spawns;
				if (filth >= corpses*2) {
					filth -= 2;
					if (rand() % 3 < 2) {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("goblin"));
						Announce::Inst()->AddMsg("A goblin crawls out of the spawning pool", TCODColor::green, spawnLocation);
					} else {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("orc"));
						Announce::Inst()->AddMsg("An orc claws its way out of the spawning pool", TCODColor::green, spawnLocation);
					}
				} else if (filth >= corpses) {
					--filth;
					--corpses;
					if (rand() % 2) {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("goblin"));
						Announce::Inst()->AddMsg("A goblin crawls out of the spawning pool", TCODColor::green, spawnLocation);
					} else {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("orc"));
						Announce::Inst()->AddMsg("An orc claws its way out of the spawning pool", TCODColor::green, spawnLocation);
					}
				} else {
					corpses -= 2;
					if (rand() % 3 < 2) {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("orc"));
						Announce::Inst()->AddMsg("An orc claws its way out of the spawning pool", TCODColor::green, spawnLocation);
					} else {
						Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("goblin"));
						Announce::Inst()->AddMsg("A goblin crawls out of the spawning pool", TCODColor::green, spawnLocation);
					}
				}

				if (rand() % int(std::sqrt((double)spawns)) == 0) Expand();
			}
		}
	}
}

void SpawningPool::Expand() {
	Coordinate location(-1,-1);
	for (int i = 0; i < 10; ++i) {
		location = Coordinate((a.X()-1) + rand() % ((b.X()-a.X())+3), (a.Y()-1) + rand() % ((b.Y()-a.Y())+3));
		if (Map::Inst()->GetConstruction(location.X(), location.Y()) != uid) {
			if (Map::Inst()->GetConstruction(location.X()-1, location.Y()) == uid) break;
			if (Map::Inst()->GetConstruction(location.X()+1, location.Y()) == uid) break;		
			if (Map::Inst()->GetConstruction(location.X(), location.Y()-1) == uid) break;		
			if (Map::Inst()->GetConstruction(location.X(), location.Y()+1) == uid) break;
		}
		location = Coordinate(-1,-1);
	}

	if (location.X() != -1 && location.Y() != -1) {
		++expansion;
		Announce::Inst()->AddMsg("The spawning pool expands", TCODColor::darkGreen, location);
		if (location.X() < a.X()) a.X(location.X());
		if (location.Y() < a.Y()) a.Y(location.Y());
		if (location.X() > b.X()) b.X(location.X());
		if (location.Y() > b.Y()) b.Y(location.Y());

		//Swallow nature objects
		if (Map::Inst()->NatureObject(location.X(), location.Y()) >= 0) {
			Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[Map::Inst()->NatureObject(location.X(), location.Y())]);
		}
		//Destroy buildings
		if (Map::Inst()->GetConstruction(location.X(), location.Y()) >= 0) {
			Attack attack;
			attack.Type(DAMAGE_MAGIC);
			TCOD_dice_t damage;
			damage.nb_dices = 100;
			damage.nb_faces = 100;
			damage.multiplier = 100;
			damage.addsub = 1000;
			attack.Amount(damage);
			if (Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(location.X(), location.Y())).lock()) 
				Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(location.X(), location.Y())).lock()->Damage(&attack);

		}

		Map::Inst()->SetConstruction(location.X(), location.Y(), uid);

		for (int i = 0; i < 5 + rand() % 5; ++i) Map::Inst()->Corrupt(location.X(), location.Y());

	} else {
		Announce::Inst()->AddMsg("The spawning pool bubbles ominously", TCODColor::darkGreen, Position());
		for (int i = 0; i < 5; ++i) Map::Inst()->Corrupt(x, y);
	}

}

void SpawningPool::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx, screeny;

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (Map::Inst()->GetConstruction(x,y) == uid) {
				screenx = x - upleft.X();
				screeny = y - upleft.Y();
				if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 &&
					screeny < console->getHeight()) {
						console->setCharForeground(screenx, screeny, color);
						if (condition > 0) console->setChar(screenx,screeny, (graphic[1]));
						else console->setChar(screenx,screeny, TCOD_CHAR_BLOCK2);
				}
			}
		}
	}
}
