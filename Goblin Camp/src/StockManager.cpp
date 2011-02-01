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

#include <boost/algorithm/string.hpp>

#include "Random.hpp"
#include "StockManager.hpp"
#include "Item.hpp"
#include "Construction.hpp"
#include "NatureObject.hpp"
#include "JobManager.hpp"
#include "Map.hpp"

#ifdef DEBUG
#include <iostream>
#endif

StockManager* StockManager::instance = 0;
StockManager* StockManager::Inst() {
	if (!instance) {
		instance = new StockManager();
	}
	return instance;
}

StockManager::StockManager(void){
	Init();
}


StockManager::~StockManager(void){
}

void StockManager::Init() {
	//Initialize all quantities to -1 (== not visible in stocks screen)
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		categoryQuantities.insert(std::pair<ItemCategory,int>(i,-1));
	}
	for (unsigned int i = 0; i < Item::Presets.size(); ++i) {
		typeQuantities.insert(std::pair<ItemType,int>(i,-1));
#ifdef DEBUG
		std::cout<<"Initializing typeQuantity "<<Item::ItemTypeToString(i)<<" to "<<typeQuantities[i]<<"\n";
#endif
	}

	//Figure out which items are producable, this is so that we won't show _all_ item types in the
	//stock manager screen. Things such as trash and plant mid-growth types won't show this way
	for (unsigned int item = 0; item < Item::Presets.size(); ++item) {

		//Now figure out if this item is producable either from a workshop, or designations
		bool producerFound = false;

		//Bog iron is a hard coded special case, for now. TODO: Think about this
		if (boost::iequals(Item::Presets[item].name, "Bog iron")) {
			producables.insert(item);
			fromEarth.insert(item);
			producerFound = true;
			UpdateQuantity(item, 0);
		}

		for (unsigned int cons = 0; cons < Construction::Presets.size(); ++cons) { //Look through all constructions
			for (unsigned int prod = 0; prod < Construction::Presets[cons].products.size(); ++prod) { //Products
				if (Construction::Presets[cons].products[prod] == item) {
					//This construction has this itemtype as a product
					producables.insert(item);
					producers.insert(std::pair<ItemType, ConstructionType>(item, cons));
					producerFound = true;
#ifdef DEBUG
					std::cout<<"Found producer for "<<Item::Presets[item].name<<": "<<Construction::Presets[cons].name<<"\n";
#endif
					break;
				}
			}
		}

		if (!producerFound) {//Haven't found a producer, so check NatureObjects if a tree has this item as a component
#ifdef DEBUG
			std::cout<<"Checking trees for production of "<<Item::Presets[item].name<<"\n";
#endif
			for (unsigned int natObj = 0; natObj < NatureObject::Presets.size(); ++natObj) {
				if (NatureObject::Presets[natObj].tree) {
					for (std::list<ItemType>::iterator compi = NatureObject::Presets[natObj].components.begin(); 
						compi != NatureObject::Presets[natObj].components.end(); ++compi) {
#ifdef DEBUG
							std::cout<<"Is "<<Item::ItemTypeToString(*compi)<<" = "<<Item::Presets[item].name<<"\n";
#endif
							if (*compi == item) {
								producables.insert(item);
								fromTrees.insert(item);
								UpdateQuantity(item, 0);
							}
					}
				}
			}
#ifdef DEBUG
			std::cout<<"No producer found for "<<Item::Presets[item].name<<"\n";
#endif
			//Add items that aren't produced, but are still handy to see on the stockmanager screen
			if (Item::Presets[item].categories.find(Item::StringToItemCategory("Seed")) != Item::Presets[item].categories.end() ||
				Item::Presets[item].categories.find(Item::StringToItemCategory("Food")) != Item::Presets[item].categories.end() ||
				Item::Presets[item].categories.find(Item::StringToItemCategory("Fiber")) != Item::Presets[item].categories.end() ||
				Item::Presets[item].categories.find(Item::StringToItemCategory("Bone")) != Item::Presets[item].categories.end()) {
#ifdef DEBUG
					std::cout<<"Adding "<<Item::Presets[item].name<<" to stocks anyway\n";
#endif
					producables.insert(item);
			}
		}

	}
}

void StockManager::Update() {
	for (std::set<ItemType>::iterator prodi = producables.begin(); prodi != producables.end(); ++prodi) {
		if (minimums[*prodi] > 0) {
			int difference = minimums[*prodi] - typeQuantities[*prodi];
			if (difference > 0) {
				difference = std::max(1, difference / Item::Presets[*prodi].multiplier);
				//Difference is now equal to how many jobs are required to fulfill the deficit
				if (fromTrees.find(*prodi) != fromTrees.end()) { //Item is a component of trees
					//Subtract the amount of active tree felling jobs from the difference
					difference -= treeFellingJobs.size();
					//Pick a designated tree and go chop it
					for (std::list<boost::weak_ptr<NatureObject> >::iterator treei = designatedTrees.begin();
						treei != designatedTrees.end() && difference > 0; ++treei) {
							if (!treei->lock()) continue;

							bool componentInTree = false;
							for (std::list<ItemType>::iterator compi = NatureObject::Presets[treei->lock()->Type()].components.begin(); 
								compi != NatureObject::Presets[treei->lock()->Type()].components.end(); ++compi) {
									if (*compi==*prodi) {
										componentInTree = true;
										break;
									}
							}
							if (componentInTree) {
								boost::shared_ptr<Job> fellJob(new Job("Fell tree", MED, 0, true));
								fellJob->Attempts(50);
								fellJob->ConnectToEntity(*treei);
								fellJob->DisregardTerritory();
								fellJob->SetRequiredTool(Item::StringToItemCategory("Axe"));
								fellJob->tasks.push_back(Task(MOVEADJACENT, treei->lock()->Position(), *treei));
								fellJob->tasks.push_back(Task(FELL, treei->lock()->Position(), *treei));
								JobManager::Inst()->AddJob(fellJob);
								--difference;
								treeFellingJobs.push_back(
									std::pair<boost::weak_ptr<Job>, boost::weak_ptr<NatureObject> >(fellJob, *treei));
								treei = designatedTrees.erase(treei);
							}
					}
				} else if (fromEarth.find(*prodi) != fromEarth.end()) {
					difference -= bogIronJobs.size();
					if (designatedBog.size() > 0) {
						for (int i = bogIronJobs.size(); i < std::max(1, (int)(designatedBog.size() / 100)) && difference > 0; ++i) {
							unsigned cIndex = Random::ChooseIndex(designatedBog);
							Coordinate coord = *boost::next(designatedBog.begin(), cIndex);
							boost::shared_ptr<Job> ironJob(new Job("Gather bog iron", MED, 0, true));
							ironJob->tasks.push_back(Task(MOVE, coord));
							ironJob->tasks.push_back(Task(BOGIRON));
							ironJob->tasks.push_back(Task(STOCKPILEITEM));
							JobManager::Inst()->AddJob(ironJob);
							bogIronJobs.push_back(ironJob);
							--difference;
						}
					}
				} else {
					//First get all the workshops capable of producing this product
					std::pair<std::multimap<ConstructionType, boost::weak_ptr<Construction> >::iterator,
						std::multimap<ConstructionType, boost::weak_ptr<Construction> >::iterator> 
						workshopRange = workshops.equal_range(producers[*prodi]);
					//By dividing the difference by the amount of workshops we get how many jobs each one should handle
					int workshopCount = std::distance(workshopRange.first, workshopRange.second);
					if (workshopCount > 0) {
						//We clamp this value to 10, no point in queuing up more at a time
						int jobCount = std::min(std::max(1, difference / workshopCount), 10);
						//Now we just check that each workshop has 'jobCount' amount of jobs for this product
						for (std::multimap<ConstructionType, boost::weak_ptr<Construction> >::iterator worki =
							workshopRange.first; worki != workshopRange.second && difference > 0; ++worki) {
								int jobsFound = 0;
								for (int jobi = 0; jobi < (signed int)worki->second.lock()->JobList()->size(); ++jobi) {
									if ((*worki->second.lock()->JobList())[jobi] == *prodi) {
										++jobsFound;
										--difference;
									}
								}
								if (jobsFound < jobCount) {
									for (int i = 0; i < jobCount - jobsFound; ++i) {
										worki->second.lock()->AddJob(*prodi);
										if (Random::Generate(9) < 8) --difference; //Adds a bit of inexactness (see orcyness) to production
									}
								}
						}
					}
				}
			}
		}
	}

	//We need to check our treefelling jobs for successes and cancellations
	for (std::list<std::pair<boost::weak_ptr<Job>, boost::weak_ptr<NatureObject> > >::iterator jobi =
		treeFellingJobs.begin(); jobi != treeFellingJobs.end();) { //*Phew*
			if (!jobi->first.lock()) {
				//Job no longer exists, so remove it from our list
				//If the tree still exists, it means that the job was cancelled, so add
				//the tree back to our designations.
				if (jobi->second.lock()) {
					designatedTrees.push_back(jobi->second);
				}
				jobi = treeFellingJobs.erase(jobi);
			} else {
				++jobi;
			}
	}

	for (std::list<boost::weak_ptr<Job> >::iterator jobi = bogIronJobs.begin(); jobi != bogIronJobs.end();) {
		if (!jobi->lock()) {
			jobi = bogIronJobs.erase(jobi);
		} else {
			++jobi;
		}
	}

}

void StockManager::UpdateQuantity(ItemType type, int quantity) {
	//If we receive an update about a new type, it should be made available
	//Constructions issue a 0 quantity update when they are built
#ifdef DEBUG
	std::cout<<"Quantity update "<<Item::ItemTypeToString(type)<<"\n";
#endif

	if (typeQuantities[type] == -1) typeQuantities[type] = 0;

	typeQuantities[type] += quantity;
#ifdef DEBUG
	std::cout<<"Type "<<type<<" quantity now "<<typeQuantities[type]<<"\n";
#endif
	for (std::set<ItemCategory>::iterator cati = Item::Presets[type].categories.begin(); cati != Item::Presets[type].categories.end(); ++cati) {
		if (categoryQuantities[*cati] == -1) categoryQuantities[*cati] = 0;
		categoryQuantities[*cati] += quantity;
#ifdef DEBUG
		std::cout<<Item::ItemCategoryToString(*cati)<<" "<<quantity<<"\n";
#endif
	}
#ifdef DEBUG
	std::cout<<Item::ItemTypeToString(type)<<" "<<quantity<<"\n";
#endif
}

int StockManager::CategoryQuantity(ItemCategory cat) { return categoryQuantities[cat]; }
int StockManager::TypeQuantity(ItemType type) { return typeQuantities[type]; }

std::set<ItemType>* StockManager::Producables() { return &producables; }

void StockManager::UpdateWorkshops(boost::weak_ptr<Construction> cons, bool add) {
	if (add) {
		workshops.insert(std::pair<ConstructionType, boost::weak_ptr<Construction> >(cons.lock()->Type(), cons));
	} else {
		//Because it is being removed, this has been called from a destructor which means
		//that the construction no longer exists, and the weak_ptr should give !lock
		for (std::multimap<ConstructionType, boost::weak_ptr<Construction> >::iterator worki = workshops.begin();
			worki != workshops.end(); ++worki) {
				if (!worki->second.lock()) {
					workshops.erase(worki);
					break;
				}
		}
	}
}

int StockManager::Minimum(ItemType item) {return minimums[item];}

void StockManager::AdjustMinimum(ItemType item, int value) {
	minimums[item] += value;
	if (minimums[item] < 0) minimums[item] = 0;
}

void StockManager::SetMinimum(ItemType item, int value) {
	minimums[item] = std::max(0, value);
}

void StockManager::UpdateTreeDesignations(boost::weak_ptr<NatureObject> nObj, bool add) {
	if (boost::shared_ptr<NatureObject> natObj = nObj.lock()) {
		if (add) {
			designatedTrees.push_back(natObj);
		} else {
			for (std::list<boost::weak_ptr<NatureObject> >::iterator desi = designatedTrees.begin();
				desi != designatedTrees.end();) {
					//Now that we're iterating through the designations anyway, might as well
					//do some upkeeping
					if (!desi->lock()) desi = designatedTrees.erase(desi);
					else if (desi->lock() == natObj) {
						designatedTrees.erase(desi);
						break;
					} else ++desi;
			}
		}
	}
}

void StockManager::UpdateBogDesignations(Coordinate coord, bool add) {
	if (add) {
		if (Map::Inst()->Type(coord.X(), coord.Y()) == TILEBOG) {
			designatedBog.insert(coord);
		}
	} else if (!add && designatedBog.find(coord) != designatedBog.end()) {
		designatedBog.erase(coord);
	}
}

void StockManager::Reset() {
	categoryQuantities.clear();
	typeQuantities.clear();
	minimums.clear();
	producables.clear();
	producers.clear();
	workshops.clear();
	fromTrees.clear();
	fromEarth.clear();
	designatedTrees.clear();
	treeFellingJobs.clear();
	designatedBog.clear();
	Init();
}
