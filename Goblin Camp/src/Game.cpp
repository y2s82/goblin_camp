#include <map>
#include <boost/multi_array.hpp>
#include <boost/bind.hpp>

#ifdef DEBUG
#include <iostream>
#endif

#include "Game.hpp"
#include "Tile.hpp"
#include "Coordinate.hpp"
#include "Job.hpp"
#include "Gcamp.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "Announce.hpp"
#include "GCamp.hpp"
#include "StockManager.hpp"
#include "UI.hpp"

int Game::ItemTypeCount = 0;
int Game::ItemCatCount = 0;

Game* Game::instance = 0;

Game::Game() :
    season(EarlySpring),
    time(0),
    orcCount(0),
    goblinCount(0),
	paused(false),
    upleft(Coordinate(0,0))
{
}

Game* Game::Inst() {
	if (!instance) instance = new Game();
	return instance;
}

bool Game::CheckPlacement(Coordinate target, Coordinate size) {
	for (int x = target.x(); x < target.x() + size.x(); ++x) {
		for (int y = target.y(); y < target.y() + size.y(); ++y) {
			if (x < 0 || y < 0 || x >= Map::Inst()->Width() || y >= Map::Inst()->Height() || !Map::Inst()->Buildable(x,y)) return false;
		}
	}
	return true;
}

int Game::PlaceConstruction(Coordinate target, ConstructionType construct) {
	boost::shared_ptr<Construction> newCons(new Construction(construct, target));
	Game::Inst()->constructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newCons->Uid(), newCons));
	Coordinate blueprint = Construction::Blueprint(construct);
	for (int x = target.x(); x < target.x() + blueprint.x(); ++x) {
		for (int y = target.y(); y < target.y() + blueprint.y(); ++y) {
			Map::Inst()->Buildable(x,y,false);
			Map::Inst()->Construction(x,y,newCons->Uid());
		}
	}

	boost::shared_ptr<Job> buildJob(new Job("Build construction", MED, 0, false));

	for (std::list<ItemCategory>::iterator materialIter = newCons->MaterialList()->begin(); materialIter != newCons->MaterialList()->end(); ++materialIter) {
        boost::shared_ptr<Job> pickupJob(new Job("Pickup materials", MED, 0, true));
        pickupJob->Parent(buildJob);
        buildJob->PreReqs()->push_back(pickupJob);

        pickupJob->tasks.push_back(Task(FIND, Coordinate(0,0), boost::weak_ptr<Entity>(), *materialIter));
        pickupJob->tasks.push_back(Task(MOVE));
        pickupJob->tasks.push_back(Task(TAKE));
        pickupJob->tasks.push_back(Task(MOVE, newCons->Position(), newCons));
        pickupJob->tasks.push_back(Task(PUTIN, newCons->Storage().lock()->Position(), newCons->Storage()));
        JobManager::Inst()->AddJob(pickupJob);
	}

	buildJob->tasks.push_back(Task(MOVEADJACENT, newCons->Position(), newCons));
	buildJob->tasks.push_back(Task(BUILD, newCons->Position(), newCons));
	buildJob->ConnectToEntity(newCons);

	JobManager::Inst()->AddJob(buildJob);
	return newCons->Uid();
}

int Game::PlaceStockpile(Coordinate a, Coordinate b, ConstructionType stockpile, int symbol) {
	//Placing a stockpile isn't as straightforward as just building one in each tile
	//We want to create 1 stockpile, at a, and then expand it from a to b.
	//Using the stockpile expansion function ensures that it only expands into valid tiles
	boost::shared_ptr<Stockpile> newSp( (stockpile != FARMPLOT) ? new Stockpile(stockpile, symbol, a) : new FarmPlot(stockpile, symbol, a) );
	Map::Inst()->Buildable(a.x(), a.y(), false);
	Map::Inst()->Construction(a.x(), a.y(), newSp->Uid());
	newSp->Expand(a,b);
	Game::Inst()->constructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newSp->Uid(),static_cast<boost::shared_ptr<Construction> >(newSp)));

	//Spawning a BUILD job is not required because stockpiles are created "built"
	return newSp->Uid();
}

//Returns Coordinate(<0,<0) if not found
Coordinate Game::FindClosestAdjacent(Coordinate pos, boost::weak_ptr<Entity> ent) {
	Coordinate closest(-9999, -9999);

    if (boost::dynamic_pointer_cast<Construction>(ent.lock())) {
        boost::weak_ptr<Construction> construct(boost::static_pointer_cast<Construction>(ent.lock()));
        for (int ix = construct.lock()->x()-1; ix <= construct.lock()->x() + Construction::Blueprint(construct.lock()->type()).x(); ++ix) {
            for (int iy = construct.lock()->y()-1; iy <= construct.lock()->y() + Construction::Blueprint(construct.lock()->type()).y(); ++iy) {
                if (ix == construct.lock()->x()-1 || ix == construct.lock()->x() + Construction::Blueprint(construct.lock()->type()).x() ||
                    iy == construct.lock()->y()-1 || iy == construct.lock()->y() + Construction::Blueprint(construct.lock()->type()).y()) {
                    if (Map::Inst()->Walkable(ix,iy)) {
                        if (distance(pos.x(), pos.y(), ix, iy) < distance(pos.x(), pos.y(), closest.x(), closest.y()))
                            closest = Coordinate(ix,iy);
                    }
                }
            }
        }
    } else {
        for (int ix = ent.lock()->x()-1; ix <= ent.lock()->x()+1; ++ix) {
            for (int iy = ent.lock()->y()-1; iy <= ent.lock()->y()+1; ++iy) {
                if (ix == ent.lock()->x()-1 || ix == ent.lock()->x()+1 ||
                    iy == ent.lock()->y()-1 || iy == ent.lock()->y()+1) {
                    if (Map::Inst()->Walkable(ix,iy)) {
                        if (distance(pos.x(), pos.y(), ix, iy) < distance(pos.x(), pos.y(), closest.x(), closest.y()))
                            closest = Coordinate(ix,iy);
                    }
                }
            }
        }
    }
    return closest;
}

bool Game::Adjacent(Coordinate pos, boost::weak_ptr<Entity> ent) {
    if (boost::dynamic_pointer_cast<Construction>(ent.lock())) {
        boost::weak_ptr<Construction> construct(boost::static_pointer_cast<Construction>(ent.lock()));
        for (int ix = construct.lock()->x()-1; ix <= construct.lock()->x() + Construction::Blueprint(construct.lock()->type()).x(); ++ix) {
            for (int iy = construct.lock()->y()-1; iy <= construct.lock()->y() + Construction::Blueprint(construct.lock()->type()).y(); ++iy) {
                if (pos.x() == ix && pos.y() == iy) { return true; }
            }
        }
        return false;
    } else {
        for (int ix = ent.lock()->x()-1; ix <= ent.lock()->x()+1; ++ix) {
            for (int iy = ent.lock()->y()-1; iy <= ent.lock()->y()+1; ++iy) {
                if (pos.x() == ix && pos.y() == iy) { return true; }
            }
        }
        return false;
    }
}

int Game::CreateNPC(Coordinate target, NPCType type) {
    boost::shared_ptr<NPC> npc;
    switch (type) {
        case 0:
            npc = boost::shared_ptr<NPC>(new NPC(target, boost::bind(NPC::JobManagerFinder, _1), boost::bind(NPC::PlayerNPCReact, _1)));
            npc->speed(50 + rand() % 20);
            npc->color(TCODColor::grey);
            npc->graphic('g');
            npc->name = TCODNamegen::generate("goblin");
            npc->faction = 0;
			npc->needsNutrition = true;
            ++goblinCount;
            break;

        case 1:
            npc = boost::shared_ptr<NPC>(new NPC(target, boost::bind(NPC::JobManagerFinder, _1), boost::bind(NPC::PlayerNPCReact, _1)));
            npc->speed(50 + rand() % 20);
            npc->color(TCODColor::blue);
            npc->graphic('o');
            npc->Expert(true);
            npc->name = TCODNamegen::generate("orc");
            npc->faction = 0;
			npc->needsNutrition = true;
            ++orcCount;
            break;

        case 2:
            npc = boost::shared_ptr<NPC>(new NPC(target, boost::bind(NPC::PeacefulAnimalFindJob, _1), boost::bind(NPC::PeacefulAnimalReact, _1)));
            npc->speed(75 + rand() % 20);
            npc->color(TCODColor::yellow);
            npc->graphic('a');
            npc->name = "Bee";
            npc->faction = 1;
            break;
    }

	npcList.insert(std::pair<int,boost::shared_ptr<NPC> >(npc->Uid(),npc));
	return npc->Uid();
}

int Game::OrcCount() { return orcCount; }
int Game::GoblinCount() { return goblinCount; }

void Game::BumpEntity(int uid) {
	boost::weak_ptr<Entity> entity;

	std::map<int,boost::shared_ptr<NPC> >::iterator npc = npcList.find(uid);
	if (npc != npcList.end()) {
	    entity = npc->second;
	} else {
	    std::map<int,boost::shared_ptr<Item> >::iterator item = itemList.find(uid);
	    if (item != itemList.end()) {
	        entity = item->second;
	    }
	}

	if (entity.lock()) {
		int newx = entity.lock()->x();
		int newy = entity.lock()->y();
		while (!Map::Inst()->Walkable(newx,newy)) {
			++newx;
		}
		entity.lock()->Position(Coordinate(newx,newy));
	} 
#ifdef DEBUG
	else { std::cout<<"\nTried to bump nonexistant entity."; }
#endif
}

void Game::DoNothing() {}

void Game::Exit() {
    Logger::End();
	exit(0);
}

int Game::ScreenWidth() const {	return screenWidth; }
int Game::ScreenHeight() const { return screenHeight; }

void Game::Init(int width, int height, bool fullscreen) {
    int resWidth, resHeight;
    TCODSystem::getCurrentResolution(&resWidth, &resHeight);
    TCODSystem::getCharSize(&charWidth, &charHeight);
    resWidth /= charWidth;
    resHeight /= charHeight;
	width /= charWidth;
	height /= charHeight;
    if (width < 1 || resWidth < width) width = resWidth;
    if (height < 1 || resHeight < height) height = resHeight;
    
	if (!fullscreen) {
		if (width == -1) 
			width = std::max(75, width - 100); 
		if (height == -1)
			height = std::max(75, height - 75);
	}

	srand((unsigned int)std::time(0));

    //Enabling TCOD_RENDERER_GLSL causes GCamp to crash on exit, apparently it's because of an ATI driver issue.
	TCODConsole::initRoot(width, height, "Goblin Camp", fullscreen, TCOD_RENDERER_SDL);
    TCODConsole::root->setAlignment(TCOD_LEFT);

	screenWidth = width; screenHeight = height;

	TCODMouse::showCursor(true);

	TCODConsole::setKeyboardRepeat(500, 10);

    Logger::Inst()->output<<"Opening constructions.xml\n";

    ticpp::Document constructionsXml("./constructions.xml");
    ticpp::Document itemsXml("./items.xml");
    ticpp::Document plantsXml("./wildplants.xml");
    try {
        constructionsXml.LoadFile();
        itemsXml.LoadFile();
        plantsXml.LoadFile();
    } catch (ticpp::Exception& ex) {
        Logger::Inst()->output<<"Failed opening xml!\n";
        Logger::Inst()->output<<ex.what();
        Exit();
    }

    //Item presets _must_ be loaded first because constructons.xml refers to items by name
	Item::LoadPresets(itemsXml);
	Construction::LoadPresets(constructionsXml);
	NatureObject::LoadPresets(plantsXml);
	Logger::Inst()->output<<"Finished loading presets.";
	Logger::Inst()->output.flush();
    try {
        TCODNamegen::parse("names.dat");
    } catch (...) {
        Logger::Inst()->output<<"Error loading names.dat";
        exit(0);
    }

	GenerateMap();
	buffer = new TCODConsole(screenWidth, screenHeight);
	season = LateWinter;
}

void Game::RemoveConstruction(boost::weak_ptr<Construction> cons) {
	if (cons.lock()) {
        Coordinate blueprint = Construction::Blueprint(cons.lock()->type());
        for (int x = cons.lock()->x(); x < cons.lock()->x() + blueprint.x(); ++x) {
            for (int y = cons.lock()->y(); y < cons.lock()->y() + blueprint.y(); ++y) {
                Map::Inst()->Buildable(x,y,true);
                Map::Inst()->Construction(x,y,-1);
            }
        }
        Game::Inst()->constructionList.erase(cons.lock()->Uid());
	}
}

boost::weak_ptr<Construction> Game::GetConstruction(int uid) {
    return constructionList[uid];
}

int Game::CreateItem(Coordinate pos, ItemType type, bool store, int ownerFaction, 
	std::vector<boost::weak_ptr<Item> > comps) {

    boost::shared_ptr<Item> newItem;
    if (Item::Presets[type].organic) {
        newItem.reset(static_cast<Item*>(new OrganicItem(pos, type)));
        boost::weak_ptr<OrganicItem> orgItem(boost::static_pointer_cast<OrganicItem>(newItem));
        orgItem.lock()->Season(Item::Presets[type].season);
        orgItem.lock()->Nutrition(Item::Presets[type].nutrition);
        orgItem.lock()->Growth(Item::Presets[type].growth);
    } else if (Item::Presets[type].container > 0) {
        newItem.reset(static_cast<Item*>(new Container(pos, type, Item::Presets[type].container)));
    } else {
        newItem.reset(new Item(pos, type, 0, comps));
    }

    freeItems.insert(newItem);
    Map::Inst()->ItemList(newItem->x(), newItem->y())->insert(newItem->Uid());
    itemList.insert(std::pair<int,boost::shared_ptr<Item> >(newItem->Uid(), newItem));
	if (store) StockpileItem(newItem);

	for (unsigned int i = 0; i < comps.size(); ++i) {
		if (comps[i].lock()) {
			Game::Inst()->RemoveItem(comps[i]);
		}
	}


	return newItem->Uid();
}

void Game::RemoveItem(boost::weak_ptr<Item> item) {
	if (item.lock()) {
		Map::Inst()->ItemList(item.lock()->_x, item.lock()->_y)->erase(item.lock()->uid);
		itemList.erase(item.lock()->uid);
		if (freeItems.find(item) != freeItems.end()) freeItems.erase(item);
	}
}

boost::weak_ptr<Item> Game::GetItem(int uid) {
    return itemList[uid];
}

void Game::ItemContained(boost::weak_ptr<Item> item, bool con) {
    if (!con) {
        freeItems.insert(item);
        Map::Inst()->ItemList(item.lock()->x(), item.lock()->y())->insert(item.lock()->Uid());
    }
    else {
        freeItems.erase(item);
        Map::Inst()->ItemList(item.lock()->x(), item.lock()->y())->erase(item.lock()->Uid());
    }
}

void Game::CreateWater(Coordinate pos) {
	CreateWater(pos, 10);
}

void Game::CreateWater(Coordinate pos, int amount, int time) {
	boost::weak_ptr<WaterNode> water(Map::Inst()->GetWater(pos.x(), pos.y()));
	if (!water.lock()) {
		boost::shared_ptr<WaterNode> newWater(new WaterNode(pos.x(), pos.y(), amount, time));
		waterList.push_back(boost::weak_ptr<WaterNode>(newWater));
		Map::Inst()->SetWater(pos.x(), pos.y(), newWater);
	} else {water.lock()->Depth(water.lock()->Depth()+amount);}
}

int Game::DistanceNPCToCoordinate(int uid, Coordinate pos) {
	return distance(npcList[uid]->x(), npcList[uid]->y(), pos.x(), pos.y());
}

int Game::Distance(Coordinate a, Coordinate b) {
	return distance(a.x(), a.y(), b.x(), b.y());
}

boost::weak_ptr<Item> Game::FindItemByCategoryFromStockpiles(ItemCategory category) {
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = constructionList.begin(); consIter != constructionList.end(); ++consIter) {
		if (consIter->second->stockpile && !boost::dynamic_pointer_cast<FarmPlot>(consIter->second)) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByCategory(category));
			if (item.lock() && !item.lock()->Reserved()) {
				return item;
			}
		}
	}
	return boost::weak_ptr<Item>();
}

boost::weak_ptr<Item> Game::FindItemByTypeFromStockpiles(ItemType type) {
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = constructionList.begin(); consIter != constructionList.end(); ++consIter) {
		if (boost::dynamic_pointer_cast<Stockpile>(consIter->second) && !boost::dynamic_pointer_cast<FarmPlot>(consIter->second)) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByType(type));
			if (item.lock() && !item.lock()->Reserved()) {
				return item;
			}
		}
	}
	return boost::weak_ptr<Item>();
}


//Findwater returns the coordinates to the closest Water* that has sufficient depth
Coordinate Game::FindWater(Coordinate pos) {
	Coordinate closest(-9999,-9999);
	for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end(); ++wati) {
		if (wati->lock()->Depth() > DRINKABLE_WATER_DEPTH) {
			if (Distance(wati->lock()->Position(), pos) < Distance(closest, pos)) closest = wati->lock()->Position();
		}
	}
	if (closest.x() == -9999) return Coordinate(-1,-1);
	return closest;
}

void Game::Update() {
    ++time;

    if (time == MONTH_LENGTH) {
        if (season < LateWinter) season = (Seasons)((int)season + 1);
        else season = EarlySpring;

        switch (season) {
            case EarlySpring:
                Announce::Inst()->AddMsg("Spring has begun");
                SpawnTillageJobs();
                DecayItems();
                break;
            case LateSpring:
                DecayItems();
            case LateSummer:
                DecayItems();
            case LateWinter:
                DeTillFarmPlots();
                DecayItems();
                break;
            case EarlySummer:
                Announce::Inst()->AddMsg("Summer has begun");
                SpawnTillageJobs();
                DecayItems();
                break;
            case EarlyFall:
                Announce::Inst()->AddMsg("Fall has begun");
                SpawnTillageJobs();
                DecayItems();
                break;
            case EarlyWinter:
                Announce::Inst()->AddMsg("Winter has begun");
                DecayItems();
                break;
            default: break;
        }
        time = 0;
    }

	for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end(); ++wati) {
		if (wati->lock() && rand() % 50 == 0) wati->lock()->Update();
		else if (!wati->lock()) wati = waterList.erase(wati);
	}

    std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.end();
	while (std::distance(wati, waterList.end()) < 10) {
		--wati;
		if (wati->lock()) wati->lock()->Update();
	}

	std::list<boost::weak_ptr<NPC> > npcsWaitingForRemoval;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = npcList.begin(); npci != npcList.end(); ++npci) {
		if (!npci->second->Dead()) npci->second->Think();
		else npcsWaitingForRemoval.push_back(npci->second);
	}

	for (std::list<boost::weak_ptr<NPC> >::iterator remNpci = npcsWaitingForRemoval.begin(); remNpci != npcsWaitingForRemoval.end(); ++remNpci) {
		RemoveNPC(*remNpci);
	}

	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = constructionList.begin(); consi != constructionList.end(); ++consi) {
		if (consi->second->farmplot) {
	        boost::static_pointer_cast<FarmPlot>(consi->second)->Update();
	    }
	}

	if (rand() % (UPDATES_PER_SECOND * 15) == 0) {
	    for (std::set<boost::weak_ptr<Item> >::iterator itemi = freeItems.begin(); itemi != freeItems.end(); ++itemi) {
			if (itemi->lock() && !itemi->lock()->Reserved() && itemi->lock()->Faction() == 0) StockpileItem(*itemi);
	    }
	}

	StockManager::Inst()->Update();
}

void Game::StockpileItem(boost::weak_ptr<Item> item) {
    for (std::map<int,boost::shared_ptr<Construction> >::iterator stocki = constructionList.begin(); stocki != constructionList.end(); ++stocki) {
		if (stocki->second->stockpile) {
            boost::shared_ptr<Stockpile> sp(boost::static_pointer_cast<Stockpile>(stocki->second));
            if (sp->Allowed(Item::Presets[item.lock()->Type()].categories) && !sp->Full()) {

                //Found a stockpile that both allows the item, and has space
                //Check if the item can be contained, and if so if any containers are in the stockpile

                boost::shared_ptr<Job> stockJob(new Job("Store item", LOW));
                Coordinate target = Coordinate(-1,-1);
                boost::weak_ptr<Item> container;

                if (Item::Presets[item.lock()->Type()].fitsin >= 0) {
                     container = sp->FindItemByCategory(Item::Presets[item.lock()->Type()].fitsin, NOTFULL);
                    if (container.lock()) {
                        target = container.lock()->Position();
                        stockJob->ReserveSpace(boost::static_pointer_cast<Container>(container.lock()));
                    }
                }

                if (target.x() == -1) target = sp->FreePosition();

                if (target.x() != -1) {
                    stockJob->ReserveSpot(sp, target);
                    stockJob->ReserveItem(item);
                    stockJob->tasks.push_back(Task(MOVE, item.lock()->Entity::Position()));
                    stockJob->tasks.push_back(Task(TAKE, item.lock()->Entity::Position(), item));
                    stockJob->tasks.push_back(Task(MOVE, target));
                    if (!container.lock())
                        stockJob->tasks.push_back(Task(PUTIN, target, sp->Storage(target)));
                    else
                        stockJob->tasks.push_back(Task(PUTIN, target, container));
                    JobManager::Inst()->AddJob(stockJob);
                    return;
                }
            }
        }
    }
}

void Game::Draw() {
    Game::Inst()->buffer->clear();

	Map::Inst()->Draw(upleft, buffer);

    for (std::map<int,boost::shared_ptr<Construction> >::iterator cit = constructionList.begin(); cit != constructionList.end(); ++cit) {
        cit->second->Draw(upleft, buffer);
    }
    for (std::map<int,boost::shared_ptr<Item> >::iterator iit = itemList.begin(); iit != itemList.end(); ++iit) {
        iit->second->Draw(upleft, buffer);

    }
    for (std::map<int,boost::shared_ptr<NPC> >::iterator it = npcList.begin(); it != npcList.end(); ++it) {
        it->second->Draw(upleft, buffer);
    }
    for (std::map<int,boost::shared_ptr<NatureObject> >::iterator natit = natureList.begin(); natit != natureList.end(); ++natit) {
        natit->second->Draw(upleft, buffer);
    }

	UI::Inst()->Draw(upleft, buffer);

	Announce::Inst()->Draw(5, buffer);

	FlipBuffer();

}

void Game::FlipBuffer() {
	buffer->flush();
	TCODConsole::blit(buffer, 0, 0, screenWidth, screenHeight, TCODConsole::root, 0, 0);
	TCODConsole::root->flush();
}

Seasons Game::Season() { return season; }

void Game::SpawnTillageJobs() {
   	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = constructionList.begin(); consi != constructionList.end(); ++consi) {
		if (consi->second->farmplot) {
	        boost::shared_ptr<Job> tillJob(new Job("Till farmplot"));
	        tillJob->tasks.push_back(Task(MOVE, consi->second->Position()));
	        tillJob->tasks.push_back(Task(USE, consi->second->Position(), consi->second));
	        JobManager::Inst()->AddJob(tillJob);
	    }
	}
}

void Game::DeTillFarmPlots() {
   	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = constructionList.begin(); consi != constructionList.end(); ++consi) {
		if (consi->second->farmplot) {
            boost::static_pointer_cast<FarmPlot>(consi->second)->tilled = false;
	    }
   	}
}

void Game::GenerateMap() {

	for (int x = 0; x < Map::Inst()->Width(); ++x) {
		for (int y = 260; y <= 270; ++y) {
			if (y >= 264 && y <= 266 && rand() % 5 == 0) {
				Map::Inst()->Type(x,y,TILERIVERBED);
				CreateWater(Coordinate(x,y));
			} else if ((y == 260 || y == 270) && rand() % 3 == 0) {
			} else {
				Map::Inst()->Type(x,y,TILEDITCH);
				CreateWater(Coordinate(x,y));
			}
		}
	}

    for (int x = 0; x < Map::Inst()->Width(); ++x) {
        for (int y = 0; y < Map::Inst()->Height(); ++y) {
            if (Map::Inst()->Walkable(x,y) && Map::Inst()->Type(x,y) == TILEGRASS) {
                if (rand() % 100 == 0) {
                    int r = rand() % 100;
                    for (int i = 0; i < (signed int)NatureObject::Presets.size(); ++i) {
                        int type = rand() % NatureObject::Presets.size();
                        if (NatureObject::Presets[type].rarity > r) {
                            for (int clus = 0; clus < NatureObject::Presets[type].cluster; ++clus) {
                                int ax = x + ((rand() % 5) - 2);
                                int ay = y + ((rand() % 5) - 2);
                                if (ax < 0) ax = 0; if (ax >= Map::Inst()->Width()) ax = Map::Inst()->Width()-1;
                                if (ay < 0) ay = 0; if (ay >= Map::Inst()->Height()) ay = Map::Inst()->Height()-1;
                                if (Map::Inst()->Walkable(ax,ay) && Map::Inst()->Type(ax,ay) == TILEGRASS
                                    && Map::Inst()->NatureObject(ax,ay) < 0) {
                                    boost::shared_ptr<NatureObject> natObj(new NatureObject(Coordinate(ax,ay), type));
                                    natureList.insert(std::pair<int, boost::shared_ptr<NatureObject> >(natObj->Uid(), natObj));
                                    Map::Inst()->NatureObject(ax,ay,natObj->Uid());
                                    Map::Inst()->Walkable(ax,ay,NatureObject::Presets[natObj->Type()].walkable);
                                    Map::Inst()->BlocksLight(ax,ay,NatureObject::Presets[natObj->Type()].walkable);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

bool Game::CheckTree(Coordinate, Coordinate) {
    return true;
}

void Game::FellTree(Coordinate a, Coordinate b) {
    for (int x = a.x(); x <= b.x(); ++x) {
        for (int y = a.y(); y <= b.y(); ++y) {
            int natUid = Map::Inst()->NatureObject(x,y);
            if (natUid >= 0) {
                boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
                if (natObj.lock() && natObj.lock()->Tree() && !natObj.lock()->Marked()) {
                    natObj.lock()->Mark();
                    boost::shared_ptr<Job> fellJob(new Job("Fell tree", MED, 0, false));
                    fellJob->ConnectToEntity(natObj);
                    fellJob->tasks.push_back(Task(MOVEADJACENT, natObj.lock()->Position(), natObj));
                    fellJob->tasks.push_back(Task(FELL, natObj.lock()->Position(), natObj));
                    JobManager::Inst()->AddJob(fellJob);
                }
            }
        }
   }
}

void Game::HarvestWildPlant(Coordinate a, Coordinate b) {
    for (int x = a.x(); x <= b.x(); ++x) {
        for (int y = a.y(); y <= b.y(); ++y) {
            int natUid = Map::Inst()->NatureObject(x,y);
            if (natUid >= 0) {
                boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
                if (natObj.lock() && natObj.lock()->Harvestable() && !natObj.lock()->Marked()) {
                    natObj.lock()->Mark();
                    boost::shared_ptr<Job> fellJob(new Job("Harvest wild plant", MED, 0, false));
                    fellJob->ConnectToEntity(natObj);
                    fellJob->tasks.push_back(Task(MOVEADJACENT, natObj.lock()->Position(), natObj));
                    fellJob->tasks.push_back(Task(HARVESTWILDPLANT, natObj.lock()->Position(), natObj));
                    JobManager::Inst()->AddJob(fellJob);
                }
            }
        }
   }
}


void Game::RemoveNatureObject(boost::weak_ptr<NatureObject> natObj) {
    Map::Inst()->NatureObject(natObj.lock()->x(), natObj.lock()->y(), -1);
    natureList.erase(natObj.lock()->Uid());
}

std::string Game::SeasonToString(Seasons season) {
    switch (season) {
        case EarlySpring: return "Early Spring";
        case Spring: return "Spring";
        case LateSpring: return "Late Spring";
        case EarlySummer: return "Early Summer";
        case Summer: return "Summer";
        case LateSummer: return "Late Summer";
        case EarlyFall: return "Early Fall";
        case Fall: return "Fall";
        case LateFall: return "Late Fall";
        case EarlyWinter: return "Early Winter";
        case Winter: return "Winter";
        case LateWinter: return "Late Winter";
        default: return "???";
    }
}

void Game::DecayItems() {
    std::list<int> eraseList;
    std::list<std::pair<ItemType, Coordinate> > creationList;
    for (std::map<int,boost::shared_ptr<Item> >::iterator itemit = itemList.begin(); itemit != itemList.end(); ++itemit) {
        if (itemit->second->decayCounter > 0) {
            if (--itemit->second->decayCounter == 0) {
                for (std::vector<ItemType>::iterator decaylisti = Item::Presets[itemit->second->type].decayList.begin(); decaylisti != Item::Presets[itemit->second->type].decayList.end(); ++decaylisti) {
                    creationList.push_back(std::pair<ItemType, Coordinate>(*decaylisti, itemit->second->Position()));
                }
                eraseList.push_back(itemit->first);
            }
        }
    }

    for (std::list<int>::iterator delit = eraseList.begin(); delit != eraseList.end(); ++delit) {
        RemoveItem(GetItem(*delit));
    }

    for (std::list<std::pair<ItemType, Coordinate> >::iterator crit = creationList.begin(); crit != creationList.end(); ++crit) {
        if (crit->first >= 0) {
            CreateItem(crit->second, crit->first, false);
        } else {
            CreateFilth(crit->second);
        }
    }
}

void Game::CreateFilth(Coordinate pos) {
	CreateFilth(pos, 1);
}

void Game::CreateFilth(Coordinate pos, int amount) {
	boost::weak_ptr<FilthNode> filth(Map::Inst()->GetFilth(pos.x(), pos.y()));
	if (!filth.lock()) {
		boost::shared_ptr<FilthNode> newFilth(new FilthNode(pos.x(), pos.y(), amount));
		filthList.push_back(boost::weak_ptr<FilthNode>(newFilth));
		Map::Inst()->SetFilth(pos.x(), pos.y(), newFilth);
	} else {filth.lock()->Depth(filth.lock()->Depth()+amount);}
}

void Game::FindNearbyNPCs(boost::shared_ptr<NPC> npc) {
    npc->nearNpcs.clear();
    for (int endx = std::max((signed int)npc->_x - LOS_DISTANCE, 0); endx < std::min((signed int)npc->_x + LOS_DISTANCE, Map::Inst()->Width()); endx += 3) {
        for (int endy = std::max((signed int)npc->_y - LOS_DISTANCE, 0); endy < std::min((signed int)npc->_y + LOS_DISTANCE, Map::Inst()->Height()); endy += 3) {
            if (endx == std::max((signed int)npc->_x - LOS_DISTANCE, 0) || endx == std::min((signed int)npc->_x + LOS_DISTANCE, Map::Inst()->Width())
                || endy == std::max((signed int)npc->_y - LOS_DISTANCE, 0) || endy == std::min((signed int)npc->_y + LOS_DISTANCE, Map::Inst()->Height())) {
                int x = npc->_x;
                int y = npc->_y;
                TCODLine::init(x, y, endx, endy);
                do {
                    if (Map::Inst()->BlocksLight(x,y)) break;

                    for (std::set<int>::iterator npci = Map::Inst()->NPCList(x,y)->begin(); npci != Map::Inst()->NPCList(x,y)->end(); ++npci) {
						if (*npci != npc->uid) npc->nearNpcs.push_back(npcList[*npci]);
                    }

                    if (npc->nearNpcs.size() > 10) break;

                } while(!TCODLine::step(&x, &y));
            }
        }
    }
}

void Game::Pause() {
	paused = !paused;
}

bool Game::Paused() { return paused; }

int Game::CharHeight() const { return charHeight; }
int Game::CharWidth() const { return charWidth; }

void Game::RemoveNPC(boost::weak_ptr<NPC> npc) {
	if (npc.lock()) {
		npcList.erase(npc.lock()->uid);
	}
}