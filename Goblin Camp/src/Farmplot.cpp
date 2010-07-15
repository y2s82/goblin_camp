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

#include "Farmplot.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "GCamp.hpp"
#include "JobManager.hpp"

FarmPlot::FarmPlot(ConstructionType type, int symbol, Coordinate target) : Stockpile(type, symbol, target),
	tilled(false),
	growth(std::map<Coordinate, int>())
{
	for (int i = 0; i < Game::ItemCatCount; ++i) {
		allowed[i] = false;
	}

	for (int i = 0; i < Game::ItemTypeCount; ++i) {
		if (Item::Presets[i].categories.find(Item::StringToItemCategory("Seed")) != Item::Presets[i].categories.end()) {
			allowedSeeds.insert(std::pair<ItemType,bool>(i, true));
		}
	}
}

void FarmPlot::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx, screeny;

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (Map::Inst()->Construction(x,y) == uid) {
				screenx = x - upleft.X();
				screeny = y - upleft.Y();
				if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 &&
					screeny < console->getHeight()) {
						console->setFore(screenx, screeny, TCODColor::white);
						console->setChar(screenx,	screeny, (graphic[1]));

						if (!containers[Coordinate(x,y)]->empty()) {
							boost::weak_ptr<Item> item = containers[Coordinate(x,y)]->GetFirstItem();
							if (item.lock()) {
								console->putCharEx(screenx, screeny, item.lock()->Graphic(), item.lock()->Color(), TCODColor::black);
							}
						}
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
		if (!containerIt->second->empty() && growth[containerIt->first] > MONTH_LENGTH * 2 && rand() % 5 == 0) {
			boost::weak_ptr<OrganicItem> plant(boost::static_pointer_cast<OrganicItem>(containerIt->second->GetFirstItem().lock()));
			if (plant.lock() && !plant.lock()->Reserved()) {
				if (rand() % 10 == 0) { //Chance for the plant to die
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
			while (!containerIt->second->empty() && !reserved[containerIt->first]) {
				++containerIt;
				if (containerIt == containers.end()) return 100;
			}
			growth[containerIt->first] = 0;
			seedsLeft = false;
			for (std::map<ItemType, bool>::iterator seedi = allowedSeeds.begin(); seedi != allowedSeeds.end(); ++seedi) {
				boost::weak_ptr<Item> seed = Game::Inst()->FindItemByTypeFromStockpiles(seedi->first);
				if (seed.lock()) {
					boost::shared_ptr<Job> plantJob(new Job("Plant"));
					plantJob->ReserveEntity(seed);
					plantJob->ReserveSpot(boost::static_pointer_cast<Stockpile>(shared_from_this()), containerIt->first);
					plantJob->tasks.push_back(Task(MOVE, seed.lock()->Position()));
					plantJob->tasks.push_back(Task(TAKE, seed.lock()->Position(), seed));
					plantJob->tasks.push_back(Task(MOVE, containerIt->first));
					plantJob->tasks.push_back(Task(PUTIN, containerIt->first, containerIt->second));
					JobManager::Inst()->AddJob(plantJob);
					++containerIt;
					seedsLeft = true;
				}
			}
		}
		return 100;
	}
	return progress;
}
