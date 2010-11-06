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

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <libtcod.hpp>
#ifdef DEBUG
#	include <iostream>
#endif
#include <algorithm>

#include "Construction.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "JobManager.hpp"
#include "GCamp.hpp"
#include "Camp.hpp"
#include "StockManager.hpp"
#include "UI.hpp"
#include "UI/ConstructionDialog.hpp"
#include "Item.hpp"

Coordinate Construction::Blueprint(ConstructionType construct) {
	return Construction::Presets[construct].blueprint;
}

Coordinate Construction::ProductionSpot(ConstructionType construct) {
	return Construction::Presets[construct].productionSpot;
}

std::vector<int> Construction::AllowedAmount = std::vector<int>();
std::set<std::string> Construction::Categories = std::set<std::string>();
std::vector<ConstructionPreset> Construction::Presets = std::vector<ConstructionPreset>();

Construction::Construction(ConstructionType vtype, Coordinate target) : Entity(),
	color(TCODColor::white),
	type(vtype),
	producer(false),
	progress(0),
	container(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, 0, 1000, -1))),
	materialsUsed(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, 0, Construction::Presets[type].materials.size(), -1))),
	dismantle(false),
	time(0),
	built(false)
{
	x = target.X();
	y = target.Y();

	graphic = Construction::Presets[type].graphic;
	maxCondition = Construction::Presets[type].maxCondition;
	materials = Construction::Presets[type].materials;
	name = Construction::Presets[type].name;
	walkable = Construction::Presets[type].walkable;
	producer = Construction::Presets[type].producer;
	products = Construction::Presets[type].products;
	stockpile = Construction::Presets[type].tags[STOCKPILE];
	farmplot = Construction::Presets[type].tags[FARMPLOT];
	condition = 0-maxCondition;
	if (Construction::Presets[type].color != TCODColor::black) color = Construction::Presets[type].color;
}

Construction::~Construction() {
	for (int ix = x; ix < (signed int)x + Construction::Blueprint(type).X(); ++ix) {
		for (int iy = y; iy < (signed int)y + Construction::Blueprint(type).Y(); ++iy) {
			Map::Inst()->Buildable(ix,iy,true);
			Map::Inst()->SetWalkable(ix,iy,true);
			Map::Inst()->SetConstruction(ix,iy,-1);
			Map::Inst()->BlocksLight(ix,iy,false);
		}
	}
	
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		if (itemi->lock()) {
			itemi->lock()->SetFaction(0); //Return item to player faction
			itemi->lock()->PutInContainer(boost::weak_ptr<Item>()); //Set container to none
		}
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }

	if (producer) StockManager::Inst()->UpdateWorkshops(boost::weak_ptr<Construction>(), false);

	if (Construction::Presets[type].tags[WALL]) { UpdateWallGraphic(); }
	else if (Construction::Presets[type].tags[DOOR]) { UpdateWallGraphic(true, false); }

	if (Construction::AllowedAmount[type] >= 0) {
		++Construction::AllowedAmount[type];
	}
	
	if (built) {
		if (!HasTag(CENTERSCAMP)) Camp::Inst()->UpdateCenter(Center(), false);
		else Camp::Inst()->UnlockCenter();
	}
}


void Construction::Condition(int value) {condition = value;}
int Construction::Condition() {return condition;}

void Construction::Draw(Coordinate upleft, TCODConsole* console) {
	int screeny = y - upleft.Y();
	int screenx = x - upleft.X();
	int ychange = 0;
	int width = graphic[0];
	int height = (graphic.size() - 1) / width;
	if (screenx + width - 1 >= 0 && screenx < console->getWidth() && screeny + height - 1 >= 0 && screeny < console->getHeight()) {
		for (int i = 1; i < (signed int)graphic.size(); ++i) {
			if(screenx + i - 1 >= 0 && screeny >= 0) {
				if (dismantle) console->setCharBackground(screenx+i-1,screeny, TCODColor::darkGrey);
				console->setCharForeground(screenx+i-1,screeny, color);
				if (condition > i*-10) console->setChar(screenx+i-1,screeny, (graphic[i]));
				else console->setChar(screenx+i-1,screeny, TCOD_CHAR_BLOCK2);
			}
			++ychange;
			if (ychange == width) { ++screeny; screenx -= width; ychange = 0; }
		}
	}
}

Coordinate Construction::Center() {
	int width = graphic[0];
	int height = (graphic.size() - 1) / width;
	return Coordinate(Position().X() + (width - 1) / 2, Position().Y() + (height - 1) / 2);
}

int Construction::Build() {
	++condition;
	if (condition > 0) {
		//Check that all the required materials are inside the building
		//It only checks that the amount of materials equals whats expected, but not what those materials are
		//Theoretically it'd be possible to build a construction out of the wrong materials, but this should
		//be impossible in practice.
		if ((signed int)materials.size() != materialsUsed->size()) return BUILD_NOMATERIAL;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
			color = TCODColor::lerp(color, itemi->lock()->Color(), 0.75f);
			itemi->lock()->SetFaction(-1); //Remove from player faction so it doesn't show up in stocks
		}

		//TODO: constructions should have the option of having both walkable and unwalkable tiles
		condition = maxCondition;
		for (unsigned int ix = x; ix < x + Construction::Blueprint(type).X(); ++ix) {
			for (unsigned int iy = y; iy < y + Construction::Blueprint(type).Y(); ++iy) {
				Map::Inst()->SetWalkable(ix, iy, walkable);
				Map::Inst()->BlocksWater(ix, iy, !walkable);
				Map::Inst()->BlocksLight(ix, iy, Construction::Presets[type].blocksLight);
			}
		}

		if (Construction::Presets[type].tags[WALL]) { UpdateWallGraphic(); }
		else if (Construction::Presets[type].tags[DOOR]) { UpdateWallGraphic(true, false); }
		if (producer) {
			StockManager::Inst()->UpdateWorkshops(boost::static_pointer_cast<Construction>(shared_from_this()), true);
			for (unsigned int prod = 0; prod < Construction::Presets[type].products.size(); ++prod) {
				StockManager::Inst()->UpdateQuantity(Construction::Presets[type].products[prod], 0);
			}
		}
		built = true;
		if (!HasTag(CENTERSCAMP)) Camp::Inst()->UpdateCenter(Center(), true);
		else Camp::Inst()->LockCenter(Center());
	}
	return condition;
}

ConstructionType Construction::Type() { return type; }

std::list<ItemCategory>* Construction::MaterialList() {return &materials;}

bool Construction::Producer() {return producer;}

std::vector<ItemType>* Construction::Products() { return &products; }
ItemType Construction::Products(int index) { return products[index]; }

std::deque<ItemType>* Construction::JobList() { return &jobList; }
ItemType Construction::JobList(int index) { return jobList[index]; }

void Construction::AddJob(ItemType item) {
	if (!dismantle) {
		jobList.push_back(item);
		if (jobList.size() == 1) {
			SpawnProductionJob();
		}
	}
}

void Construction::CancelJob(int index) {
	if (index == 0 && index < (signed int)jobList.size()) {
		jobList.erase(jobList.begin());
		//Empty container in case some pickup jobs got done
		while (!container->empty()) {
			boost::weak_ptr<Item> item = container->GetFirstItem();
			container->RemoveItem(item);
			if (item.lock()) item.lock()->PutInContainer();
		}
		while (!jobList.empty() && !reserved && !SpawnProductionJob());
	} else if (index > 0 && index < (signed int)jobList.size()) { 
		jobList.erase(jobList.begin() + index); 
	}
	else if (condition <= 0) {
		Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
	}
	else if (dismantle) dismantle = false; //Stop trying to dismantle
}

int Construction::Use() {
	if (jobList.size() > 0) {
		++progress;
		if (progress >= 100) {

			bool allComponentsFound = true;

			for (int compi = 0; compi < (signed int)Item::Components(jobList[0]).size(); ++compi) {
				allComponentsFound = false;
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
					if (itemi->lock()->IsCategory(Item::Components(jobList[0], compi))) {
						allComponentsFound = true;
						break;
					}
				}
			}
			if (!allComponentsFound) return -1;

			std::vector<boost::weak_ptr<Item> > components;
			boost::shared_ptr<Container> itemContainer;

			for (int compi = 0; compi < (signed int)Item::Components(jobList[0]).size(); ++compi) {
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
					if (itemi->lock()->IsCategory(Item::Components(jobList[0], compi))) {
						if (itemi->lock()->IsCategory(Item::Presets[jobList[0]].containIn)) {
							//This component is the container our product should be placed in
							itemContainer = boost::static_pointer_cast<Container>(itemi->lock());
						} else {
							//Just a component of the product
							components.push_back(*itemi);
						}
						container->RemoveItem(*itemi);
						break;
					}
				}
			}

			//Create the "fruit" of the components. Basically fruits have their seeds as their fruits,
			//this makes berries give their seeds when made into wine, for example.
			for (unsigned int i = 0; i < components.size(); ++i) {
				if (components[i].lock()) {
					for (std::list<ItemType>::iterator fruiti = Item::Presets[components[i].lock()->Type()].fruits.begin(); fruiti != Item::Presets[components[i].lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}
				}
			}

			//Create the items
			if (itemContainer) itemContainer->PutInContainer(); 
			for (int i = 0; i < Item::Presets[jobList[0]].multiplier; ++i) {
				if (itemContainer) Game::Inst()->CreateItem(Position()+Construction::Presets[type].productionSpot, jobList[0], false, 0, components, itemContainer);
				else Game::Inst()->CreateItem(Position()+Construction::Presets[type].productionSpot, jobList[0], true, 0, components);
			}

			//Destroy the components
			for (unsigned int i = 0; i < components.size(); ++i) {
				if (components[i].lock()) {
					Game::Inst()->RemoveItem(components[i]);
				}
			}

			progress = 0;
			//~Job removes the job from the jobList
			return 100;
		}
		return progress;
	}
	return -1;
}

bool Construction::SpawnProductionJob() {
	//Only spawn a job if the construction isn't already reserved
	if (!reserved) {
		//First check that the requisite items actually exist
		std::list<boost::weak_ptr<Item> > componentList;
		for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
			boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::Components(jobList.front(), compi), Center(), APPLYMINIMUMS);
			if (item.lock()) {
				componentList.push_back(item);
				item.lock()->Reserve(true);
			} else {
				//Not all items available, cancel job and unreserve the reserved items.
				for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
					resi->lock()->Reserve(false);
				}
				jobList.pop_front();
				return false;
			}
		}

		//Unreserve the items now, because the individual jobs will reserve them for themselves
		for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
			resi->lock()->Reserve(false);
		}


		boost::shared_ptr<Job> newProductionJob(new Job("Produce "+Item::ItemTypeToString(jobList.front()), MED, 0, false));
		newProductionJob->ConnectToEntity(shared_from_this());
		newProductionJob->ReserveEntity(shared_from_this());

		for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
			boost::shared_ptr<Job> newPickupJob(new Job("Pickup " + Item::ItemCategoryToString(Item::Components(jobList.front(), compi)) + " for " + Presets[Type()].name));
			newPickupJob->tasks.push_back(Task(FIND, Center(), boost::shared_ptr<Entity>(), Item::Components(jobList.front(), compi), APPLYMINIMUMS | EMPTY));
			newPickupJob->tasks.push_back(Task(MOVE));
			newPickupJob->tasks.push_back(Task(TAKE));
			newPickupJob->tasks.push_back(Task(MOVE, container->Position(), container));
			newPickupJob->tasks.push_back(Task(PUTIN, container->Position(), container));

			newProductionJob->PreReqs()->push_back(newPickupJob);
			newPickupJob->Parent(newProductionJob);

			JobManager::Inst()->AddJob(newPickupJob);
		}
		newProductionJob->tasks.push_back(Task(MOVE, Position()+Construction::ProductionSpot(type)));
		newProductionJob->tasks.push_back(Task(USE, Position()+Construction::ProductionSpot(type), shared_from_this()));
		JobManager::Inst()->AddJob(newProductionJob);
		return true;
	}
	return false;
}

boost::weak_ptr<Container> Construction::Storage() {
	if (condition > 0) return container;
	else return materialsUsed;
}

void Construction::UpdateWallGraphic(bool recurse, bool self) {
	bool n = false,s = false,e = false,w = false;

	if (Map::Inst()->GetConstruction(x - 1, y) > -1) {
		boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x - 1, y)).lock();
		if (cons->Condition() > 0 && !cons->dismantle && (Construction::Presets[cons->Type()].tags[WALL] || Construction::Presets[cons->Type()].tags[DOOR])) {
			w = true;
		}
	}
	if (Map::Inst()->GetConstruction(x + 1, y) > -1) {
		boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x + 1, y)).lock();
		if (cons->Condition() > 0 && !cons->dismantle && (Construction::Presets[cons->Type()].tags[WALL] || Construction::Presets[cons->Type()].tags[DOOR])) {
			e = true;
		}
	}
	if (Map::Inst()->GetConstruction(x, y-1) > -1) {
		boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x, y-1)).lock();
		if (cons->Condition() > 0 && !cons->dismantle && (Construction::Presets[cons->Type()].tags[WALL] || Construction::Presets[cons->Type()].tags[DOOR])) {
			n = true;
		}
	}
	if (Map::Inst()->GetConstruction(x, y+1) > -1) {
		boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x, y+1)).lock();
		if (cons->Condition() > 0 && !cons->dismantle && (Construction::Presets[cons->Type()].tags[WALL] || Construction::Presets[cons->Type()].tags[DOOR])) {
			s = true;
		}
	}

	if (self && Construction::Presets[type].tags[WALL]) {
		if (n&&s&&e&&w) graphic[1] = 197;
		else if (n&&s&&e) graphic[1] = 195;
		else if (n&&s&&w) graphic[1] = 180;
		else if (n&&e&&w) graphic[1] = 193;
		else if (s&&e&&w) graphic[1] = 194;
		else if (n&&s) graphic[1] = 179;
		else if (e&&w) graphic[1] = 196;
		else if (n&&e) graphic[1] = 192;
		else if (n&&w) graphic[1] = 217;
		else if (s&&e) graphic[1] = 218;
		else if (s&&w) graphic[1] = 191;
		else if (e||w) graphic[1] = 196;
		else if (n||s) graphic[1] = 179;
		else graphic[1] = 197;
	}

	if (recurse) {
		if (w)
			Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x - 1, y)).lock()->UpdateWallGraphic(false);
		if (e)
			Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x + 1, y)).lock()->UpdateWallGraphic(false);
		if (n)
			Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x, y - 1)).lock()->UpdateWallGraphic(false);
		if (s)
			Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x, y + 1)).lock()->UpdateWallGraphic(false);
	}
}

bool Construction::HasTag(ConstructionTag tag) { return Construction::Presets[type].tags[tag]; }

void Construction::Update() {
	if (Construction::Presets[type].spawnCreaturesTag != "" && condition > 0) {
		if (rand() % Construction::Presets[type].spawnFrequency == 0) {
			NPCType monsterType = Game::Inst()->GetRandomNPCTypeByTag(Construction::Presets[type].spawnCreaturesTag);
			TCODColor announceColor = NPC::Presets[monsterType].tags.find("friendly") != 
				NPC::Presets[monsterType].tags.end() ? TCODColor::green : TCODColor::red;
			int amount = Game::DiceToInt(NPC::Presets[monsterType].group);
			if (amount == 1) {
				Announce::Inst()->AddMsg("A "+NPC::NPCTypeToString(monsterType)+" emerges from the "+name+"!", announceColor, Position());
			} else {
				Announce::Inst()->AddMsg(NPC::Presets[monsterType].plural+" emerge from the "+name+"!", announceColor, Position());
			}
			for (int i = 0; i < amount; ++i) {
				Game::Inst()->CreateNPC(Position() + ProductionSpot(type), monsterType);
			}
		}
	}
}

void Construction::Dismantle() {
	if (!Construction::Presets[type].permanent && !dismantle) {
		dismantle = true;
		if (producer) {
			jobList.clear();
		}

		boost::shared_ptr<Job> dismantleJob(new Job((boost::format("Dismantle %s") % name).str(), HIGH, 0, false));
		dismantleJob->ConnectToEntity(shared_from_this());
		dismantleJob->tasks.push_back(Task(MOVEADJACENT, Position(), shared_from_this()));
		dismantleJob->tasks.push_back(Task(DISMANTLE, Position(), shared_from_this()));
		JobManager::Inst()->AddJob(dismantleJob);
	}
}

Panel *Construction::GetContextMenu() {
	return ConstructionDialog::ConstructionInfoDialog(this);
}
	
void Construction::Damage(Attack* attack) {
	double damageModifier = 1.0;

	switch (attack->Type()) {
	case DAMAGE_SLASH: damageModifier = 0.5; break;

	case DAMAGE_PIERCE: damageModifier = 0.25; break;

	case DAMAGE_BLUNT:
	case DAMAGE_MAGIC:
	case DAMAGE_FIRE:
	case DAMAGE_COLD: damageModifier = 1.0; break;

	case DAMAGE_POISON: damageModifier = 0.1; break;

	default: damageModifier = 1.0; break;
	}

	int damage = (int)(Game::DiceToInt(attack->Amount()) * damageModifier);
	condition -= damage;

	#ifdef DEBUG
	std::cout<<"Damagemod: "<<damageModifier<<"\n";
	std::cout<<name<<"("<<uid<<") inflicted "<<damage<<" damage\n";
	#endif

	if (condition <= 0) {
		Explode();
		Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
	}
}

void Construction::Explode() {
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		if (boost::shared_ptr<Item> item = itemi->lock()) {
			item->SetFaction(0); //Return item to player faction
			item->PutInContainer(); //Set container to none
			Coordinate randomTarget;
			randomTarget.X(Position().X() + ((rand() % 11) - 5));
			randomTarget.Y(Position().Y() + ((rand() % 11) - 5));
			item->CalculateFlightPath(randomTarget, 50, GetHeight());
		}
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }

}

bool Construction::CheckMaterialsPresent() { 
	if ((signed int)materials.size() != materialsUsed->size()) { return false; }
	return true;
}

bool Construction::DismantlingOrdered() { return dismantle; }

ConstructionPreset::ConstructionPreset() :
	maxCondition(0),
	graphic(std::vector<int>()),
	walkable(false),
	materials(std::list<ItemCategory>()),
	producer(false),
	products(std::vector<ItemType>()),
	name("???"),
	blueprint(Coordinate(1,1)),
	productionSpot(Coordinate(0,0)),
	dynamic(false),
	spawnCreaturesTag(""),
	spawnFrequency(10),
	placementType(UIPLACEMENT),
	blocksLight(true),
	permanent(false),
	color(TCODColor::black)
{
	for (int i = 0; i < TAGCOUNT; ++i) { tags[i] = false; }
}
	
