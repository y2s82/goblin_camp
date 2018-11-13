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

#include <boost/serialization/shared_ptr.hpp>

#include "Random.hpp"
#include "SpawningPool.hpp"
#include "UI/Button.hpp"
#include "GCamp.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"
#include "Announce.hpp"
#include "Stats.hpp"
#include "Camp.hpp"

SpawningPool::SpawningPool(ConstructionType type, const Coordinate& target) : Construction(type, target),
	dumpFilth(false),
	dumpCorpses(false),
	a(target),
	b(target),
	expansion(0),
	filth(0),
	corpses(0),
	spawns(0),
	expansionLeft(0),
	corruptionLeft(0),
	spawnsLeft(0),
	corpseContainer(boost::shared_ptr<Container>()),
	jobCount(0),
	burn(0)
{
	container = new UIContainer(std::vector<Drawable*>(), 0, 0, 16, 11);
	dialog = new Dialog(container, "Spawning Pool", 16, 10);
	container->AddComponent(new ToggleButton("Dump filth", boost::bind(&SpawningPool::ToggleDumpFilth, this), 
		boost::bind(&SpawningPool::DumpFilth, this), 2, 2, 12));
	container->AddComponent(new ToggleButton("Dump corpses", boost::bind(&SpawningPool::ToggleDumpCorpses, this), 
		boost::bind(&SpawningPool::DumpCorpses, this), 1, 6, 14));
	corpseContainer = boost::shared_ptr<Container>(new Container(target, 0, 1000, -1));
}

Panel* SpawningPool::GetContextMenu() {
	return dialog;
}

bool SpawningPool::DumpFilth(SpawningPool* sp) { return sp->dumpFilth; }
void SpawningPool::ToggleDumpFilth(SpawningPool* sp) { sp->dumpFilth = !sp->dumpFilth; }
bool SpawningPool::DumpCorpses(SpawningPool* sp) { return sp->dumpCorpses; }
void SpawningPool::ToggleDumpCorpses(SpawningPool* sp) { sp->dumpCorpses = !sp->dumpCorpses; }

Coordinate SpawningPool::SpawnLocation()
{
	Direction dirs[4] = { WEST, EAST, NORTH, SOUTH };
	std::random_shuffle(dirs,dirs+4); //shuffle to avoid predictability

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			Coordinate p(x,y);
			if (map->GetConstruction(p) == uid) {				
				for (int i = 0; i < 4; ++i) {
					Coordinate candidate = p + Coordinate::DirectionToCoordinate(dirs[i]);
					if (map->IsWalkable(candidate))
						return candidate;
				}
			}
		}
	}
	return undefined;
}

void SpawningPool::Update() {
	if (condition > 0) {

		//Generate jobs
		if (jobCount < 4) {
			if (dumpFilth && Random::Generate(UPDATES_PER_SECOND * 4) == 0) {
				if (Game::Inst()->filthList.size() > 0) {
					boost::shared_ptr<Job> filthDumpJob(new Job("Dump filth", MED));
					filthDumpJob->SetRequiredTool(Item::StringToItemCategory("Bucket"));
					filthDumpJob->Attempts(1);
					Coordinate filthLocation = Game::Inst()->FindFilth(Position());
					filthDumpJob->tasks.push_back(Task(MOVEADJACENT, filthLocation));
					filthDumpJob->tasks.push_back(Task(FILL, filthLocation));

					if (filthLocation != undefined) {
						filthDumpJob->tasks.push_back(Task(MOVEADJACENT, Position()));
						filthDumpJob->tasks.push_back(Task(POUR, Position()));
						filthDumpJob->tasks.push_back(Task(STOCKPILEITEM));
						filthDumpJob->ConnectToEntity(shared_from_this());
						++jobCount;
						JobManager::Inst()->AddJob(filthDumpJob);
					}
				}
			}
			if (dumpCorpses && StockManager::Inst()->CategoryQuantity(Item::StringToItemCategory("Corpse")) > 0 &&
				Random::Generate(UPDATES_PER_SECOND * 4) == 0) {
					boost::shared_ptr<Job> corpseDumpJob(new Job("Dump corpse", MED));
					corpseDumpJob->tasks.push_back(Task(FIND, Position(), boost::weak_ptr<Entity>(), Item::StringToItemCategory("Corpse")));
					corpseDumpJob->tasks.push_back(Task(MOVE));
					corpseDumpJob->tasks.push_back(Task(TAKE));
					corpseDumpJob->tasks.push_back(Task(FORGET)); 
					corpseDumpJob->tasks.push_back(Task(MOVEADJACENT, corpseContainer->Position()));
					corpseDumpJob->tasks.push_back(Task(PUTIN, corpseContainer->Position(), corpseContainer));
					corpseDumpJob->ConnectToEntity(shared_from_this());
					++jobCount;
					JobManager::Inst()->AddJob(corpseDumpJob);
			}
		}

		//Spawn / Expand
		if (map->GetFilth(pos).lock() && map->GetFilth(pos).lock()->Depth() > 0) {
			boost::shared_ptr<FilthNode> filthNode = map->GetFilth(pos).lock();
			filth += filthNode->Depth();
			Stats::Inst()->AddPoints(filthNode->Depth());
			corruptionLeft += filthNode->Depth() * std::min(100 * filth, 10000U);
			filthNode->Depth(0);
		}
		while (!corpseContainer->empty()) {
			boost::weak_ptr<Item> corpse = corpseContainer->GetFirstItem();
			if (boost::shared_ptr<Item> actualItem = corpse.lock()) {
				if (actualItem->IsCategory(Item::StringToItemCategory("corpse"))) {
					++corpses;
					Stats::Inst()->AddPoints(100);
				}
			}
			corpseContainer->RemoveItem(corpse);
			Game::Inst()->RemoveItem(corpse);
			for (int i = 0; i < Random::Generate(1, 2); ++i) 
				corruptionLeft += 1000 * std::min(std::max(1U, corpses), 50U);
		}

		if ((corpses*10) + filth >= 30U) {
			unsigned int newSpawns = (corpses * 10U + filth) / 30U;
			spawnsLeft += newSpawns;
			expansionLeft += newSpawns;
			corpses = 0;
			filth = 0;
		}
	}
	if (burn > 0) {
		if (Random::Generate(2) == 0) --burn;
		if (burn > 5000) {
			Expand(false);
			Game::Inst()->CreateFire(Random::ChooseInRectangle(a,b));
			if (Random::Generate(9) == 0) {
				Coordinate spawnLocation = SpawningPool::SpawnLocation();
				if (spawnLocation != undefined && Random::Generate(20) == 0)
					Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("fire elemental"));
			}
		}
	}

	if (corruptionLeft > 0) {
		map->Corrupt(Position(), 10);
		corruptionLeft -= 10;
		if (Random::Generate(500) == 0)
			map->Corrupt(Position(), 5000); //Random surges to make "tentacles" of corruption appear
	}

	if (Random::Generate(UPDATES_PER_SECOND*5) == 0) {
		if (expansionLeft > 0) {
			Expand(true);
			--expansionLeft;
		}
		if (spawnsLeft > 0) {
			Spawn();
			--spawnsLeft;
		}
	}
}

void SpawningPool::Expand(bool message) {
	Direction dirs[4] = { WEST, EAST, NORTH, SOUTH };
	std::random_shuffle(dirs,dirs+4); //shuffle to avoid predictability
	Coordinate location = undefined;
	for (int i = 0; location == undefined && i < 10; ++i) {
		Coordinate candidate = Random::ChooseInRectangle(a-1, b+1);
		//TODO factorize with IsAdjacent(p,uid) in StockPile; could go in Construction
		if (map->GetConstruction(candidate) != uid) {
			for (i = 0; i < 4; ++i)
				if (map->GetConstruction(candidate + Coordinate::DirectionToCoordinate(dirs[i])) == uid)
					location = candidate;
		}
	}

	if (location != undefined) {
		++expansion;
		if (message) Announce::Inst()->AddMsg("The spawning pool expands", TCODColor::darkGreen, location);
		a = Coordinate::min(a, location);
		b = Coordinate::max(b, location);

		//Swallow nature objects
		if (map->GetNatureObject(location) >= 0) {
			Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[map->GetNatureObject(location)]);
		}
		//Destroy buildings
		if (map->GetConstruction(location) >= 0) {
			if (boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(map->GetConstruction(location)).lock()) {
				if (construct->HasTag(STOCKPILE) || construct->HasTag(FARMPLOT)) {
					construct->Dismantle(location);
				} else {
					Attack attack;
					attack.Type(DAMAGE_MAGIC);
					TCOD_dice_t damage;
					damage.nb_rolls = 1;
					damage.nb_faces = 1;
					damage.multiplier = 1;
					damage.addsub = 100000;
					attack.Amount(damage);
					construct->Damage(&attack);
				}
			}
		}

		//Swallow items
		std::list<int> itemUids;
		for (std::set<int>::iterator itemi = map->ItemList(location)->begin();
			itemi != map->ItemList(location)->end(); ++itemi) {
				itemUids.push_back(*itemi);
		}
		for (std::list<int>::iterator itemi = itemUids.begin(); itemi != itemUids.end(); ++itemi) {
			Game::Inst()->RemoveItem(Game::Inst()->GetItem(*itemi));
		}

		map->SetConstruction(location, uid);
		map->SetBuildable(location, false);
		map->SetTerritory(location, true);

		corruptionLeft += 2000 * std::min(expansion, (unsigned int)100);

	} else {
		if (message) Announce::Inst()->AddMsg("The spawning pool bubbles ominously", TCODColor::darkGreen, Position());
		corruptionLeft += 4000 * std::min(expansion, (unsigned int)100);
	}

}

void SpawningPool::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx, screeny;

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			Coordinate p(x,y);
			if (map->GetConstruction(p) == uid) {
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

void SpawningPool::CancelJob(int) {
	if (jobCount > 0) --jobCount;
}

void SpawningPool::AcceptVisitor(ConstructionVisitor& visitor) {
	visitor.Visit(this);
}

void SpawningPool::Burn() {
	burn += 5;
	if (burn > 30000) {
		Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
	}
}

int SpawningPool::Build() {
	if (!Camp::Inst()->spawningPool.lock() || Camp::Inst()->spawningPool.lock() != boost::static_pointer_cast<SpawningPool>(shared_from_this())) {
		Camp::Inst()->spawningPool = boost::static_pointer_cast<SpawningPool>(shared_from_this());
	}
	map->Corrupt(Position(), 100);
	return Construction::Build();
}

boost::shared_ptr<Container>& SpawningPool::GetContainer() { return corpseContainer; }

void SpawningPool::Spawn() {
	Coordinate spawnLocation = SpawningPool::SpawnLocation();
	
	if (spawnLocation != undefined) {
		++spawns;

		float goblinRatio = static_cast<float>(Game::Inst()->GoblinCount()) / Game::Inst()->OrcCount();
		bool goblin = false;
		bool orc = false;
		if (goblinRatio < 2) goblin = true;
		else if (goblinRatio > 4) orc = true;
		else if (Random::Generate(2) < 2) goblin = true;
		else orc = true;

		if (goblin) {
			Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("goblin"));
			Announce::Inst()->AddMsg("A goblin crawls out of the spawning pool", TCODColor::green, spawnLocation);
		}

		if (orc) {
			Game::Inst()->CreateNPC(spawnLocation, NPC::StringToNPCType("orc"));
			Announce::Inst()->AddMsg("An orc claws its way out of the spawning pool", TCODColor::green, spawnLocation);
		}

	}
}

void SpawningPool::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Construction>(*this);
	ar & dumpFilth;
	ar & dumpCorpses;
	ar & a;
	ar & b;
	ar & expansion;
	ar & filth;
	ar & corpses;
	ar & spawns;
	ar & corpseContainer;
	ar & jobCount;
	ar & burn;
}

void SpawningPool::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Construction>(*this);
	ar & dumpFilth;
	ar & dumpCorpses;
	ar & a;
	ar & b;
	ar & expansion;
	ar & filth;
	ar & corpses;
	ar & spawns;
	ar & corpseContainer;
	ar & jobCount;
	ar & burn;
}
