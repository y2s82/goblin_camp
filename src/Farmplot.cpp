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

#include <boost/serialization/map.hpp>

#include "Random.hpp"
#include "Farmplot.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "GCamp.hpp"
#include "JobManager.hpp"
#include "StockManager.hpp"

FarmPlot::FarmPlot(ConstructionType type, int symbol, Coordinate target) : Stockpile(type, symbol, target),
	tilled(false),
	growth(std::map<Coordinate, int>())
{
	//Farmplots are a form of stockpile, disallow all items so they don't get stored here
	for (int i = 0; i < Game::ItemCatCount; ++i) {
		allowed[i] = false;
	}

	//Allow all discovered seeds
	for (int i = 0; i < Game::ItemTypeCount; ++i) {
		if (Item::Presets[i].categories.find(Item::StringToItemCategory("Seed")) != Item::Presets[i].categories.end()) {
			if (StockManager::Inst()->TypeQuantity((ItemType)i) >= 0)
				allowedSeeds.insert(std::pair<ItemType,bool>(i, false));
		}
	}
}

void FarmPlot::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx, screeny;

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			Coordinate p(x,y);
			if (map->GetConstruction(p) == uid) {
				screenx = x - upleft.X();
				screeny = y - upleft.Y();
				if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 &&
					screeny < console->getHeight()) {
						console->setCharForeground(screenx, screeny, TCODColor::darkAmber);
						console->setChar(screenx, screeny, (graphic[1]));

						if (!containers[p]->empty()) {
							boost::weak_ptr<Item> item = containers[p]->GetFirstItem();
							if (item.lock()) {
								console->putCharEx(screenx, screeny, item.lock()->GetGraphic(), item.lock()->Color(), TCODColor::black);
							}
						}

						int gray = static_cast<int>(50 - cos(strobe) * 50);
						console->setCharBackground(screenx, screeny, TCODColor(gray, gray, gray));

				}
			}
		}
	}
}

void FarmPlot::Update() {
	if (!tilled) graphic[1] = 176;
	for (std::map<Coordinate, boost::shared_ptr<Container> >::iterator containerIt = containers.begin(); containerIt != containers.end(); ++containerIt) {
		++growth[containerIt->first];
		//Normal plants ought to grow seed -> young plant -> mature plant -> fruits, which means 3
		//growths before giving fruit. 3 * 2 months means 6 months from seed to fruits
		if (!containerIt->second->empty() && growth[containerIt->first] > MONTH_LENGTH * 2 && Random::Generate(4) == 0) {
			boost::weak_ptr<OrganicItem> plant(boost::static_pointer_cast<OrganicItem>(containerIt->second->GetFirstItem().lock()));
			if (plant.lock() && !plant.lock()->Reserved()) {
				if (Random::Generate(9) == 0) { //Chance for the plant to die
					containerIt->second->RemoveItem(plant);
					Game::Inst()->CreateItem(plant.lock()->Position(), Item::StringToItemType("Dead plant"), true);
					Game::Inst()->RemoveItem(plant);
					growth[containerIt->first] = 0;
				} else {
					if (plant.lock()->Growth() > -1) { //Plant is stil growing
						int newPlant = Game::Inst()->CreateItem(plant.lock()->Position(), plant.lock()->Growth());
						containerIt->second->RemoveItem(plant);
						containerIt->second->AddItem(Game::Inst()->GetItem(newPlant));
						Game::Inst()->RemoveItem(plant);
						growth[containerIt->first] = 0;
					} else { //Plant has grown to full maturity, and should be harvested
						boost::shared_ptr<Job> harvestJob(new Job("Harvest", HIGH, 0, true));
						harvestJob->ReserveEntity(plant);
						harvestJob->tasks.push_back(Task(MOVE, plant.lock()->Position()));
						harvestJob->tasks.push_back(Task(TAKE, plant.lock()->Position(), plant));
						harvestJob->tasks.push_back(Task(HARVEST, plant.lock()->Position(), plant));
						JobManager::Inst()->AddJob(harvestJob);
						growth[containerIt->first] = 0;
					}
				}
			}
		}
	}
}

int FarmPlot::Use() {
	++progress;
	if (progress >= 100) {
		tilled = true;
		graphic[1] = '~';
		progress = 0;

		bool seedsLeft = true;
		std::map<Coordinate, boost::shared_ptr<Container> >::iterator containerIt = containers.begin();
		while (seedsLeft && containerIt != containers.end()) {
			while (!containerIt->second->empty() || reserved[containerIt->first]) {
				++containerIt;
				if (containerIt == containers.end()) return 100;
			}
			seedsLeft = false;
			if (containerIt->first.X() >= 0 && containerIt->first.Y() >= 0 && containerIt->second) {
				for (std::map<ItemType, bool>::iterator seedi = allowedSeeds.begin(); seedi != allowedSeeds.end(); ++seedi) {
					growth[containerIt->first] = -(MONTH_LENGTH / 2) + Random::Generate(MONTH_LENGTH - 1);
					if (seedi->second) {
						boost::weak_ptr<Item> seed = Game::Inst()->FindItemByTypeFromStockpiles(seedi->first, Center());
						if (seed.lock()) {
							boost::shared_ptr<Job> plantJob(new Job("Plant " + Item::ItemTypeToString(seedi->first)));
							plantJob->ReserveEntity(seed);
							plantJob->ReserveSpot(boost::static_pointer_cast<Stockpile>(shared_from_this()), containerIt->first, seed.lock()->Type());
							plantJob->tasks.push_back(Task(MOVE, seed.lock()->Position()));
							plantJob->tasks.push_back(Task(TAKE, seed.lock()->Position(), seed));
							plantJob->tasks.push_back(Task(MOVE, containerIt->first));
							plantJob->tasks.push_back(Task(PUTIN, containerIt->first, containerIt->second));
							JobManager::Inst()->AddJob(plantJob);
							seedsLeft = true;
							break; //Break out of for-loop, we found a seed to plant
						}
					}
				}
			} else { return 100; } //Container invalid 
		}
		return 100;
	}
	return progress;
}

std::map<ItemType, bool>* FarmPlot::AllowedSeeds() { return &allowedSeeds; }

void FarmPlot::AllowSeed(ItemType type, bool value) { allowedSeeds[type] = value; }

void FarmPlot::SwitchAllowed(int index) {
	std::map<ItemType, bool>::iterator it = allowedSeeds.begin();
	for(int i = 0; i < index && it != allowedSeeds.end(); i++) {
		it++;
	}
	if(it != allowedSeeds.end()) {
		allowedSeeds[it->first] = !allowedSeeds[it->first];
	}
}

bool FarmPlot::SeedAllowed(ItemType type) { return allowedSeeds[type]; }

void FarmPlot::AcceptVisitor(ConstructionVisitor& visitor) {
	visitor.Visit(this);
}

bool FarmPlot::Full(ItemType type) {
	for (int ix = a.X(); ix <= b.X(); ++ix) {
		for (int iy = a.Y(); iy <= b.Y(); ++iy) {
			Coordinate p(ix,iy);
			if (map->GetConstruction(p) == uid) {
				//If the stockpile has hit the limit then it's full for this itemtype
				if (type != 1) {
					for (std::set<ItemCategory>::iterator cati = Item::Presets[type].categories.begin();
						cati != Item::Presets[type].categories.end(); ++cati) {
							if (GetLimit(*cati) > 0 && amount[*cati] >= GetLimit(*cati)) return true;
					}
				}

				//If theres a free space then it obviously is not full
				if (containers[p]->empty() && !reserved[p]) return false;

				//Check if a container exists for this ItemCategory that isn't full
				boost::weak_ptr<Item> item = containers[p]->GetFirstItem();
				if (item.lock() && item.lock()->IsCategory(Item::StringToItemCategory("Container"))) {
					boost::shared_ptr<Container> container = boost::static_pointer_cast<Container>(item.lock());
					if (type != -1 && container->IsCategory(Item::Presets[type].fitsin) && 
						container->Capacity() >= Item::Presets[type].bulk) return false;
				}
			}
		}
	}
	return true;
}

Coordinate FarmPlot::FreePosition() {
	if (containers.size() > 0) {
		//First attempt to find a random position
		for (int i = 0; i < std::max(1, (signed int)containers.size()/4); ++i) {
			std::map<Coordinate, boost::shared_ptr<Container> >::iterator conti = boost::next(containers.begin(), Random::ChooseIndex(containers));
			if (conti != containers.end() && conti->second->empty() && !reserved[conti->first]) 
				return conti->first;
		}
		//If that fails still iterate through each position because a free position _should_ exist
		for (int ix = a.X(); ix <= b.X(); ++ix) {
			for (int iy = a.Y(); iy <= b.Y(); ++iy) {
				Coordinate p(ix,iy);
				if (map->GetConstruction(p) == uid) {
					if (containers[p]->empty() && !reserved[p]) return p;
				}
			}
		}
	}
	return Coordinate(-1,-1);
}

void FarmPlot::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}

void FarmPlot::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}
