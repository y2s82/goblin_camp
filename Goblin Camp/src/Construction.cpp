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

#include <boost/format.hpp>
#include <libtcod.hpp>
#include <ticpp.h>
#ifdef DEBUG
#include <iostream>
#endif

#include "Construction.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "JobManager.hpp"
#include "GCamp.hpp"
#include "StockManager.hpp"

Coordinate Construction::Blueprint(ConstructionType construct) {
	return Construction::Presets[construct].blueprint;
}

Coordinate Construction::ProductionSpot(ConstructionType construct) {
    return Construction::Presets[construct].productionSpot;
}

Construction::Construction(ConstructionType type, Coordinate target) : Entity(),
    color(TCODColor::white),
    _type(type),
    producer(false),
    progress(0),
    container(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, 0, 1000, -1))),
    materialsUsed(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, 0, Construction::Presets[type].materials.size(), -1))),
	dismantle(false)
{
    _x = target.x();
    _y = target.y();

    graphic = Construction::Presets[type].graphic;
    maxCondition = Construction::Presets[type].maxCondition;
    materials = Construction::Presets[type].materials;
    name = Construction::Presets[type].name;
    walkable = Construction::Presets[type].walkable;
    producer = Construction::Presets[type].producer;
    products = Construction::Presets[type].products;
	stockpile = Construction::Presets[type].tags[STOCKPILE];
	farmplot = Construction::Presets[type].tags[FARMPLOT];
	_condition = 0-maxCondition;
}

Construction::~Construction() {
    for (int ix = _x; ix <= (signed int)_x + Construction::Blueprint(_type).x(); ++ix) {
        for (int iy = _y; iy <= (signed int)_y + Construction::Blueprint(_type).y(); ++iy) {
            Map::Inst()->Buildable(ix,iy,true);
			Map::Inst()->Walkable(ix,iy,true);
            Map::Inst()->Construction(ix,iy,-1);
        }
    }

	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		itemi->lock()->Faction(0); //Return item to player faction
		itemi->lock()->PutInContainer(boost::weak_ptr<Item>()); //Set container to none
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }

	if (producer) StockManager::Inst()->UpdateWorkshops(boost::weak_ptr<Construction>(), false);
}


void Construction::condition(int value) {_condition = value;}
int Construction::condition() {return _condition;}

void Construction::Draw(Coordinate upleft, TCODConsole* console) {
	int y = _y - upleft.y();
	int x = _x - upleft.x();
	int ychange = 0;
	if (x >= 0 && x < console->getWidth() && y >= 0 && y < console->getHeight()) {
		for (int i = 1; i < (signed int)graphic.size(); ++i) {
			console->setFore(x+i-1,y, color);
			if (_condition > i*-10) console->setChar(x+i-1,y, (graphic[i]));
			else console->setChar(x+i-1,y, TCOD_CHAR_BLOCK2);
			++ychange;
			if (ychange == graphic[0]) { ++y; x -= graphic[0]; ychange = 0; }
		}
	}
}

int Construction::Build() {
	++_condition;
	if (_condition > 0) {
		//Check that all the required materials are inside the building
		//It only checks that the amount of materials equals whats expected, but not what those materials are
		//Theoretically it'd be possible to build a construction out of the wrong materials, but this should not
		//be possible in practice.
		if ((signed int)materials.size() != materialsUsed->size()) return BUILD_NOMATERIAL;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		    color = TCODColor::lerp(color, itemi->lock()->Color(), 0.75f);
			itemi->lock()->Faction(-1); //Remove from player faction so it doesn't show up in stocks
		}

		//TODO: constructions should have the option of having both walkable and unwalkable tiles
		_condition = maxCondition;
		for (unsigned int ix = _x; ix < _x + Construction::Blueprint(_type).x(); ++ix) {
			for (unsigned int iy = _y; iy < _y + Construction::Blueprint(_type).y(); ++iy) {
				Map::Inst()->Walkable(ix, iy, walkable);
				Map::Inst()->BlocksWater(ix, iy, !walkable);
				Map::Inst()->BlocksLight(ix, iy, !walkable);
			}
		}

		if (Construction::Presets[_type].tags[WALL]) { UpdateWallGraphic(); }
		else if (Construction::Presets[_type].tags[DOOR]) { UpdateWallGraphic(true, false); }
		if (producer) {
			StockManager::Inst()->UpdateWorkshops(boost::static_pointer_cast<Construction>(shared_from_this()), true);
			for (unsigned int prod = 0; prod < Construction::Presets[_type].products.size(); ++prod) {
				StockManager::Inst()->UpdateQuantity(Construction::Presets[_type].products[prod], 0);
			}
		}
	}
	return _condition;
}

ConstructionType Construction::type() { return _type; }

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
        if (!jobList.empty()) SpawnProductionJob();
    } else if (index > 0 && index < (signed int)jobList.size()) { jobList.erase(jobList.begin() + index); }
    else if (_condition <= 0) Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
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
                if (itemContainer) Game::Inst()->CreateItem(Position()+Construction::Presets[_type].productionSpot, jobList[0], false, 0, components, itemContainer);
				else Game::Inst()->CreateItem(Position()+Construction::Presets[_type].productionSpot, jobList[0], true, 0, components);
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

std::vector<ConstructionPreset> Construction::Presets = std::vector<ConstructionPreset>();

void Construction::LoadPresets(ticpp::Document doc) {
    std::string strVal;
    int intVal = 0;
    ticpp::Element* parent = doc.FirstChildElement();

    Logger::Inst()->output<<"Reading constructions.xml\n";
    try {
        ticpp::Iterator<ticpp::Node> node;
        for (node = node.begin(parent); node != node.end(); ++node) {
            if (node->Value() == "construction") {
#ifdef DEBUG
				std::cout<<"Construction\n";
#endif
                Presets.push_back(ConstructionPreset());
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
#ifdef DEBUG
					std::cout<<"Children\n";
#endif
                    if (child->Value() == "wall") {
                        Presets.back().graphic.push_back(1);
                        Presets.back().graphic.push_back('W');
                        Presets.back().tags[WALL] = true;
                    } else if (child->Value() == "name") {
                        Presets.back().name = child->ToElement()->GetText();
                    } else if (child->Value() == "graphic") {
                        ticpp::Iterator<ticpp::Node> g;
                        for (g = g.begin(child->ToElement()); g != g.end(); ++g) {
                            g->ToElement()->GetTextOrDefault(&intVal, 1);
                            Presets.back().graphic.push_back(intVal);
                        }
                    } else if (child->Value() == "producer") {
                        Presets.back().producer = (child->ToElement()->GetText() == "true") ? true : false;
                    } else if (child->Value() == "products") {
#ifdef DEBUG
                        std::cout<<"Products\n";
#endif
                        ticpp::Iterator<ticpp::Node> prods;
                        for (prods = prods.begin(child->ToElement());
                            prods != prods.end(); ++prods) {
                                Presets.back().products.push_back(Item::StringToItemType(prods->ToElement()->GetText()));
                        }
                    } else if (child->Value() == "walkable") {
                        Presets.back().walkable = (child->ToElement()->GetText() == "true") ? true : false;
                    } else if (child->Value() == "materials") {
#ifdef DEBUG
                        std::cout<<"Materials\n";
#endif
                        ticpp::Iterator<ticpp::Node> mats;
                        for (mats = mats.begin(child->ToElement()); mats != mats.end(); ++mats) {
                                Presets.back().materials.push_back(Item::StringToItemCategory(mats->ToElement()->GetText()));
                        }
                    } else if (child->Value() == "maxCondition") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().maxCondition = intVal;
                    } else if (child->Value() == "stockpile") {
						Presets.back().tags[STOCKPILE] = true;
                    } else if (child->Value() == "farmplot") {
                        Presets.back().tags[FARMPLOT] = true;
						Presets.back().dynamic = true;
                    } else if (child->Value() == "productionSpot") {
                        ticpp::Iterator<ticpp::Node> coord;
                        int x = -1, y;
                        for (coord = coord.begin(child->ToElement()); coord != coord.end(); ++coord) {
                                coord->ToElement()->GetTextOrDefault(&intVal, 1);
                                if (x == -1) x = intVal;
                                else y = intVal;
                        }
                        Presets.back().productionSpot = Coordinate(x,y);
                    } else if (child->Value() == "door") {
						Presets.back().tags[DOOR] = true;
						Presets.back().dynamic = true;
					} else if (child->Value() == "bed") {
						Presets.back().tags[BED] = true;
					}
                }
                Presets.back().blueprint = Coordinate(Presets.back().graphic[0],
                    (Presets.back().graphic.size()-1)/Presets.back().graphic[0]);
            }
        }
    } catch (ticpp::Exception& ex) {
        Logger::Inst()->output<<"Failed reading constructions.xml!\n";
        Logger::Inst()->output<<ex.what()<<'\n';
        Game::Inst()->Exit();
    }

    Logger::Inst()->output<<"Finished reading constructions.xml\nConstructions: "<<Presets.size()<<'\n';
}

void Construction::SpawnProductionJob() {
    boost::shared_ptr<Job> newProductionJob(new Job("Produce "+Item::ItemTypeToString(jobList.front()), MED, 0, false));
    newProductionJob->ConnectToEntity(shared_from_this());

	//First check that the requisite items actually exist
	std::list<boost::weak_ptr<Item> > componentList;
	for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::Components(jobList.front(), compi));
		if (item.lock()) {
			componentList.push_back(item);
			item.lock()->Reserve(true);
		} else {
			//Not all items available, cancel job and unreserve the reserved items.
			for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
				resi->lock()->Reserve(false);
			}
			jobList.pop_front();
			return;
		}
	}

	//Unreserve the items now, because the individual jobs will reserve them for themselves
	for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
		resi->lock()->Reserve(false);
	}

    for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
        boost::shared_ptr<Job> newPickupJob(new Job("Pickup materials"));
        newPickupJob->tasks.push_back(Task(FIND, Coordinate(0,0), boost::shared_ptr<Entity>(), Item::Components(jobList.front(), compi)));
        newPickupJob->tasks.push_back(Task(MOVE));
        newPickupJob->tasks.push_back(Task(TAKE));
        newPickupJob->tasks.push_back(Task(MOVE, container->Position(), container));
        newPickupJob->tasks.push_back(Task(PUTIN, container->Position(), container));

        newProductionJob->PreReqs()->push_back(newPickupJob);
        newPickupJob->Parent(newProductionJob);

        JobManager::Inst()->AddJob(newPickupJob);
    }
    newProductionJob->tasks.push_back(Task(MOVE, Position()+Construction::ProductionSpot(_type)));
    newProductionJob->tasks.push_back(Task(USE, Position()+Construction::ProductionSpot(_type), shared_from_this()));
    JobManager::Inst()->AddJob(newProductionJob);
}

boost::weak_ptr<Container> Construction::Storage() {
    if (_condition > 0) return container;
    else return materialsUsed;
}

void Construction::UpdateWallGraphic(bool recurse, bool self) {
    bool n = false,s = false,e = false,w = false;

	if (Map::Inst()->Construction(_x - 1, _y) > -1 && (Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x - 1, _y)).lock()->type()].tags[WALL]
		|| Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x - 1, _y)).lock()->type()].tags[DOOR]))
        w = true;
    if (Map::Inst()->Construction(_x + 1, _y) > -1 && (Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x + 1, _y)).lock()->type()].tags[WALL]
		|| Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x + 1, _y)).lock()->type()].tags[DOOR]))
        e = true;
    if (Map::Inst()->Construction(_x, _y - 1) > -1 && (Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y - 1)).lock()->type()].tags[WALL]
		|| Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y - 1)).lock()->type()].tags[DOOR]))
        n = true;
    if (Map::Inst()->Construction(_x, _y + 1) > -1 && (Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y + 1)).lock()->type()].tags[WALL]
		|| Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y + 1)).lock()->type()].tags[DOOR]))
        s = true;

	if (self && Construction::Presets[_type].tags[WALL]) {
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
            Game::Inst()->GetConstruction(Map::Inst()->Construction(_x - 1, _y)).lock()->UpdateWallGraphic(false);
        if (e)
            Game::Inst()->GetConstruction(Map::Inst()->Construction(_x + 1, _y)).lock()->UpdateWallGraphic(false);
        if (n)
            Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y - 1)).lock()->UpdateWallGraphic(false);
        if (s)
            Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y + 1)).lock()->UpdateWallGraphic(false);
    }
}

bool Construction::HasTag(ConstructionTag tag) { return Construction::Presets[_type].tags[tag]; }

void Construction::Update() {}

void Construction::Dismantle() {
	if (!dismantle) {
		dismantle = true;
		if (producer) {
			jobList.clear();
		}

		boost::shared_ptr<Job> dismantleJob(new Job((boost::format("Dismantle %s") % name).str()));
		dismantleJob->ConnectToEntity(shared_from_this());
		dismantleJob->tasks.push_back(Task(MOVEADJACENT, Position(), shared_from_this()));
		dismantleJob->tasks.push_back(Task(DISMANTLE, Position(), shared_from_this()));
		JobManager::Inst()->AddJob(dismantleJob);
	}
}

ConstructionPreset::ConstructionPreset() :
    maxCondition(0),
    graphic(std::vector<int>()),
    walkable(true),
    materials(std::list<ItemCategory>()),
    producer(false),
    products(std::vector<ItemType>()),
    name("???"),
    blueprint(Coordinate(1,1)),
    productionSpot(Coordinate(0,0)),
	dynamic(false)
{
	for (int i = 0; i < TAGCOUNT; ++i) { tags[i] = false; }
}
