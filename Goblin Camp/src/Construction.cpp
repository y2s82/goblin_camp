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
#include "Job.hpp"
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
    materialsUsed(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, 0, Construction::Presets[type].materials.size(), -1)))
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
	stockpile = Construction::Presets[type].stockpile;
	farmplot = Construction::Presets[type].farmPlot;
	_condition = 0-maxCondition;
}

Construction::~Construction() {
    for (int ix = _x; ix <= (signed int)_x + Construction::Blueprint(_type).x(); ++ix) {
        for (int iy = _y; iy <= (signed int)_y + Construction::Blueprint(_type).y(); ++iy) {
            Map::Inst()->Buildable(ix,iy,true);
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

void Construction::Draw(Coordinate center) {
	int y = _y - center.y() + Game::Inst()->ScreenHeight() / 2;
	int x = _x - center.x() + Game::Inst()->ScreenWidth() / 2;
	int ychange = 0;
	if (x >= 0 && x < Game::Inst()->ScreenWidth() && y >= 0 && y < Game::Inst()->ScreenHeight()) {
		for (int i = 1; i < (signed int)graphic.size(); ++i) {
			TCODConsole::root->setFore(x+i-1,y, color);
			if (_condition > i*-10) TCODConsole::root->setChar(x+i-1,y, (graphic[i]));
			else TCODConsole::root->setChar(x+i-1,y, TCOD_CHAR_BLOCK2);
			++ychange;
			if (ychange == graphic[0]) { ++y; x -= graphic[0]; ychange = 0; }
		}
	}
}

int Construction::Build() {
	++_condition;
	if (_condition > 0) {
		//Check that all the required materials are inside the building
		//This is cheating <- TODO
		if ((signed int)materials.size() != materialsUsed->size()) return BUILD_NOMATERIAL;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		    color = TCODColor::lerp(color, itemi->lock()->Color(), 0.5f);
			itemi->lock()->Faction(-1); //Remove from player faction so it doesn't show up in stocks
		}

		_condition = maxCondition;
		for (unsigned int ix = _x; ix < _x + Construction::Blueprint(_type).x(); ++ix) {
			for (unsigned int iy = _y; iy < _y + Construction::Blueprint(_type).y(); ++iy) {
				Map::Inst()->Walkable(ix, iy, walkable);
				Map::Inst()->BlocksWater(ix, iy, !walkable);
				Map::Inst()->BlocksLight(ix, iy, walkable);
			}
		}

		if (Construction::Presets[_type].wall) { UpdateWallGraphic(); }
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
    jobList.push_back(item);
    if (jobList.size() == 1) {
        SpawnProductionJob();
    }
}

void Construction::CancelJob(int index) {
    if (index == 0 && index < (signed int)jobList.size()) {
        jobList.erase(jobList.begin());
        if (!jobList.empty()) SpawnProductionJob();
    } else if (index > 0 && index < (signed int)jobList.size()) { jobList.erase(jobList.begin() + index); }
    else if (_condition <= 0) Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
}

int Construction::Use() {
    if (jobList.size() > 0) {
        ++progress;
        if (progress >= 100) {

            bool allComponentsFound;

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

            for (int compi = 0; compi < (signed int)Item::Components(jobList[0]).size(); ++compi) {
                for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
                    if (itemi->lock()->IsCategory(Item::Components(jobList[0], compi))) {
                        components.push_back(*itemi);
                        container->RemoveItem(*itemi);
                        break;
                    }
                }
            }

			for (unsigned int i = 0; i < components.size(); ++i) {
				if (components[i].lock()) {
					for (std::list<ItemType>::iterator fruiti = Item::Presets[components[i].lock()->Type()].fruits.begin(); fruiti != Item::Presets[components[i].lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}
				}
			}


            for (int i = 0; i < Item::Presets[jobList[0]].multiplier; ++i) {
                Game::Inst()->CreateItem(Position()+Construction::Presets[_type].productionSpot, jobList[0], true, 0, components);
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
                        Presets.back().wall = true;
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
                        Presets.back().stockpile = true;
                    } else if (child->Value() == "farmplot") {
                        Presets.back().farmPlot = true;
                    } else if (child->Value() == "productionSpot") {
                        ticpp::Iterator<ticpp::Node> coord;
                        int x = -1, y;
                        for (coord = coord.begin(child->ToElement()); coord != coord.end(); ++coord) {
                                coord->ToElement()->GetTextOrDefault(&intVal, 1);
                                if (x == -1) x = intVal;
                                else y = intVal;
                        }
                        Presets.back().productionSpot = Coordinate(x,y);
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

void Construction::UpdateWallGraphic(bool recurse) {
    bool n = false,s = false,e = false,w = false;

    if (Map::Inst()->Construction(_x - 1, _y) > -1 && Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x - 1, _y)).lock()->type()].wall)
        w = true;
    if (Map::Inst()->Construction(_x + 1, _y) > -1 && Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x + 1, _y)).lock()->type()].wall)
        e = true;
    if (Map::Inst()->Construction(_x, _y - 1) > -1 && Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y - 1)).lock()->type()].wall)
        n = true;
    if (Map::Inst()->Construction(_x, _y + 1) > -1 && Construction::Presets[Game::Inst()->GetConstruction(Map::Inst()->Construction(_x, _y + 1)).lock()->type()].wall)
        s = true;

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
    else graphic[1] = 224;

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

bool Construction::IsStockpile() { return stockpile; }
bool Construction::IsFarmplot() { return farmplot; }

ConstructionPreset::ConstructionPreset() :
    maxCondition(0),
    graphic(std::vector<int>()),
    walkable(true),
    materials(std::list<ItemCategory>()),
    producer(false),
    products(std::vector<ItemType>()),
    name("???"),
    blueprint(Coordinate(1,1)),
    wall(false),
    stockpile(false),
    farmPlot(false),
    productionSpot(Coordinate(0,0))
{}

Stockpile::Stockpile(ConstructionType type, int newSymbol, Coordinate target) :
	Construction(type, target),
	symbol(newSymbol),
	a(target),
	b(target)
{
	_condition = maxCondition;
    reserved.insert(std::pair<Coordinate,bool>(target,false));
    containers.insert(std::pair<Coordinate,boost::shared_ptr<Container> >(target, boost::shared_ptr<Container>(new Container(target, 0, 1, -1))));
    for (int i = 0; i < Game::ItemCatCount; ++i) {
        amount.insert(std::pair<ItemCategory, int>(i,0));
        allowed.insert(std::pair<ItemCategory, bool>(i,true));
    }
}

int Stockpile::Build() {return 1;}

boost::weak_ptr<Item> Stockpile::FindItemByCategory(ItemCategory cat, int flags) {
    for (std::map<Coordinate, boost::shared_ptr<Container> >::iterator conti = containers.begin(); conti != containers.end(); ++conti) {
        if (!conti->second->empty()) {
            boost::weak_ptr<Item> item = *conti->second->begin();
            if (item.lock()->IsCategory(cat) && !item.lock()->Reserved()) {
                if (flags & NOTFULL && boost::dynamic_pointer_cast<Container>(item.lock())) {
                    if (!boost::static_pointer_cast<Container>(item.lock())->Full()) return item;
                } else return item;
            }
            if (boost::dynamic_pointer_cast<Container>(item.lock())) {
                boost::weak_ptr<Container> cont = boost::static_pointer_cast<Container>(item.lock());
                for (std::set<boost::weak_ptr<Item> >::iterator itemi = cont.lock()->begin(); itemi != cont.lock()->end(); ++itemi) {
                    if (itemi->lock() && itemi->lock()->IsCategory(cat) && !itemi->lock()->Reserved())
                        return *itemi;
                }
            }
        }
	}
	return boost::weak_ptr<Item>();
}

boost::weak_ptr<Item> Stockpile::FindItemByType(ItemType typeValue, int flags) {
    for (std::map<Coordinate, boost::shared_ptr<Container> >::iterator conti = containers.begin(); conti != containers.end(); ++conti) {
        if (!conti->second->empty()) {
            boost::weak_ptr<Item> item = *conti->second->begin();
            if (item.lock()->Type() == typeValue && !item.lock()->Reserved()) {
                if (flags & NOTFULL && boost::dynamic_pointer_cast<Container>(item.lock())) {
                    if (!boost::static_pointer_cast<Container>(item.lock())->Full()) return item;
                } else return item;
            }
            if (boost::dynamic_pointer_cast<Container>(item.lock())) {
                boost::weak_ptr<Container> cont = boost::static_pointer_cast<Container>(item.lock());
                for (std::set<boost::weak_ptr<Item> >::iterator itemi = cont.lock()->begin(); itemi != cont.lock()->end(); ++itemi) {
                    if (itemi->lock() && itemi->lock()->Type() == typeValue && !itemi->lock()->Reserved())
                        return *itemi;
                }
            }
        }
	}
	return boost::weak_ptr<Item>();
}

void Stockpile::Expand(Coordinate from, Coordinate to) {
	//We can assume that from < to
	if (from.x() > b.x() || to.x() < a.x()) return;
	if (from.y() > b.y() || to.y() < a.y()) return;

	//The algorithm: Check each tile inbetween from and to, and if a tile is adjacent to this
	//stockpile, add it. Do this max(width,height) times.
	int repeats = std::max(to.x() - from.x(), to.y() - from.y());
	for (int repeatCount = 0; repeatCount <= repeats; ++repeatCount) {
		for (int ix = from.x(); ix <= to.x(); ++ix) {
			for (int iy = from.y(); iy <= to.y(); ++iy) {
				if (Map::Inst()->Construction(ix,iy) == -1 && Map::Inst()->Walkable(ix,iy)) {
					if (Map::Inst()->Construction(ix-1,iy) == uid ||
					    Map::Inst()->Construction(ix+1,iy) == uid ||
					    Map::Inst()->Construction(ix,iy-1) == uid ||
					    Map::Inst()->Construction(ix,iy+1) == uid) {
						//Current tile is walkable, buildable, and adjacent to the current stockpile
						Map::Inst()->Construction(ix,iy,uid);
						Map::Inst()->Buildable(ix,iy,false);
						//Update corner values
						if (ix < a.x()) a.x(ix);
						if (ix > b.x()) b.x(ix);
						if (iy < a.y()) a.y(iy);
						if (iy > b.y()) b.y(iy);
                        reserved.insert(std::pair<Coordinate,bool>(Coordinate(ix,iy),false));
                        containers.insert(std::pair<Coordinate,boost::shared_ptr<Container> >(Coordinate(ix,iy), boost::shared_ptr<Container>(new Container(Coordinate(ix,iy), 0, 1, -1))));
					}
				}
			}
		}
	}
}

void Stockpile::Draw(Coordinate center) {
    int screenx, screeny;

	for (int x = a.x(); x <= b.x(); ++x) {
		for (int y = a.y(); y <= b.y(); ++y) {
			if (Map::Inst()->Construction(x,y) == uid) {
			    screenx = x  - center.x() + Game::Inst()->ScreenWidth() / 2;
			    screeny = y - center.y() + Game::Inst()->ScreenHeight() / 2;
			    if (screenx >= 0 && screenx < Game::Inst()->ScreenWidth() && screeny >= 0 &&
                    screeny < Game::Inst()->ScreenHeight()) {
                    TCODConsole::root->setFore(screenx, screeny, TCODColor::white);
                    TCODConsole::root->setChar(screenx,	screeny, (graphic[1]));

                    if (!containers[Coordinate(x,y)]->empty()) {
                        boost::weak_ptr<Item> item = *containers[Coordinate(x,y)]->begin();
                        if (item.lock()) {
                            TCODConsole::root->putCharEx(screenx, screeny, item.lock()->Graphic(), item.lock()->Color(), TCODColor::black);
                        }
                    }
                }
			}
		}
	}
}

bool Stockpile::Allowed(ItemCategory cat) {
    return allowed[cat];
}

bool Stockpile::Allowed(std::set<ItemCategory> cats) {
    for (std::set<ItemCategory>::iterator cati = cats.begin(); cati != cats.end(); ++cati) {
        if (Allowed(*cati)) return true;
    }
    return false;
}

bool Stockpile::Full()
{
    for (int ix = a.x(); ix <= b.x(); ++ix) {
        for (int iy = a.y(); iy <= b.y(); ++iy) {
            if (Map::Inst()->Construction(ix,iy) == uid) {
                if (containers[Coordinate(ix,iy)]->empty()) return false;
            }
        }
    }
    return true;
}

Coordinate Stockpile::FreePosition() {
    for (int ix = a.x(); ix <= b.x(); ++ix) {
        for (int iy = a.y(); iy <= b.y(); ++iy) {
            if (Map::Inst()->Construction(ix,iy) == uid) {
                if (containers[Coordinate(ix,iy)]->empty() && !reserved[Coordinate(ix,iy)]) return Coordinate(ix,iy);
            }
        }
    }
    return Coordinate(-1,-1);
}

void Stockpile::ReserveSpot(Coordinate pos, bool val) { reserved[pos] = val; }

boost::weak_ptr<Container> Stockpile::Storage(Coordinate pos) {
    return containers[pos];
}

void Stockpile::SwitchAllowed(ItemCategory cat) {
    allowed[cat] = !allowed[cat];
}

FarmPlot::FarmPlot(ConstructionType type, int symbol, Coordinate target) : Stockpile(FARMPLOT, symbol, target),
    tilled(false)
{
    for (int i = 0; i < Game::ItemCatCount; ++i) {
        allowed[i] = false;
    }

	for (int i = 0; i < Game::ItemTypeCount; ++i) {
	    if (Item::Presets[i].categories.find(SEED) != Item::Presets[i].categories.end()) {
	        allowedSeeds.insert(std::pair<ItemType,bool>(i, true));
	    }
	}
}

void FarmPlot::Draw(Coordinate center) {
    int screenx, screeny;

	for (int x = a.x(); x <= b.x(); ++x) {
		for (int y = a.y(); y <= b.y(); ++y) {
			if (Map::Inst()->Construction(x,y) == uid) {
			    screenx = x  - center.x() + Game::Inst()->ScreenWidth() / 2;
			    screeny = y - center.y() + Game::Inst()->ScreenHeight() / 2;
			    if (screenx >= 0 && screenx < Game::Inst()->ScreenWidth() && screeny >= 0 &&
                    screeny < Game::Inst()->ScreenHeight()) {
                    TCODConsole::root->setFore(screenx, screeny, TCODColor::white);
                    TCODConsole::root->setChar(screenx,	screeny, (graphic[1]));


                    if (!containers[Coordinate(x,y)]->empty()) {
                        boost::weak_ptr<Item> item = containers[Coordinate(x,y)]->GetFirstItem();
                        if (item.lock()) {
                            TCODConsole::root->putCharEx(screenx, screeny, item.lock()->Graphic(), item.lock()->Color(), TCODColor::black);
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
        if (!containerIt->second->empty() && rand() % (MONTH_LENGTH*2) == 0) {
            boost::weak_ptr<OrganicItem> plant(boost::static_pointer_cast<OrganicItem>(containerIt->second->GetFirstItem().lock()));
            if (plant.lock() && !plant.lock()->Reserved()) {
                if (rand() % 10 == 0) { //Chance for the plant to die
                    containerIt->second->RemoveItem(plant);
                    Game::Inst()->CreateItem(plant.lock()->Position(), Item::StringToItemType("Dead plant"), true);
                    Game::Inst()->RemoveItem(plant);
                } else {
                    if (plant.lock()->Growth() > -1) { //Plant is stil growing
                        int newPlant = Game::Inst()->CreateItem(plant.lock()->Position(), plant.lock()->Growth());
                        containerIt->second->RemoveItem(plant);
                        containerIt->second->AddItem(Game::Inst()->GetItem(newPlant));
                        Game::Inst()->RemoveItem(plant);
                    } else { //Plant has grown to full maturity, and should be harvested
                        boost::shared_ptr<Job> harvestJob(new Job("Harvest", HIGH, 0, false));
                        harvestJob->ReserveItem(plant);
                        harvestJob->tasks.push_back(Task(MOVE, plant.lock()->Position()));
                        harvestJob->tasks.push_back(Task(TAKE, plant.lock()->Position(), plant));
                        harvestJob->tasks.push_back(Task(HARVEST, plant.lock()->Position(), plant));
                        JobManager::Inst()->AddJob(harvestJob);
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
            while (!containerIt->second->empty()) {
                ++containerIt;
                if (containerIt == containers.end()) return 100;
            }
            seedsLeft = false;
            for (std::map<ItemType, bool>::iterator seedi = allowedSeeds.begin(); seedi != allowedSeeds.end(); ++seedi) {
                boost::weak_ptr<Item> seed = Game::Inst()->FindItemByTypeFromStockpiles(seedi->first);
                if (seed.lock()) {
                    boost::shared_ptr<Job> plantJob(new Job("Plant", MED, 0, false));
                    plantJob->ReserveItem(seed);
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


