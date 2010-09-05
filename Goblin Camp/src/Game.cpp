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

#include <map>

#include <boost/multi_array.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#ifdef DEBUG
#include <iostream>
#endif

#include "Game.hpp"
#include "Tile.hpp"
#include "Coordinate.hpp"
#include "JobManager.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "Announce.hpp"
#include "GCamp.hpp"
#include "StockManager.hpp"
#include "UI.hpp"
#include "StatusEffect.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Door.hpp"
#include "Data.hpp"
#include "UI/YesNoDialog.hpp"

int Game::ItemTypeCount = 0;
int Game::ItemCatCount = 0;

Game* Game::instance = 0;

Game::Game() :
	screenWidth(0),
	screenHeight(0),
	resolutionWidth(0),
	resolutionHeight(0),
	fullscreen(false),
	renderer(TCOD_RENDERER_SDL),
	season(EarlySpring),
	time(0),
	orcCount(0),
	goblinCount(0),
	paused(false),
	toMainMenu(false),
	running(false),
	safeMonths(9),
	upleft(Coordinate(0,0)),
	events(boost::shared_ptr<Events>())
{
	for(int i = 0; i < 12; i++) {
		marks[i] = Coordinate(-1, -1);
	}
}

Game* Game::Inst() {
	if (!instance) instance = new Game();
	return instance;
}

//Checks whether all the tiles under the rectangle (target is the up-left corner) are buildable
bool Game::CheckPlacement(Coordinate target, Coordinate size) {
	for (int x = target.X(); x < target.X() + size.X(); ++x) {
		for (int y = target.Y(); y < target.Y() + size.Y(); ++y) {
			if (x < 0 || y < 0 || x >= Map::Inst()->Width() || y >= Map::Inst()->Height() || !Map::Inst()->Buildable(x,y)) return false;
		}
	}
	return true;
}

int Game::PlaceConstruction(Coordinate target, ConstructionType construct) {
	//Check if the required materials exist before creating the build job
	std::list<boost::weak_ptr<Item> > componentList;
	for (std::list<ItemCategory>::iterator mati = Construction::Presets[construct].materials.begin();
		mati != Construction::Presets[construct].materials.end(); ++mati) {
			boost::weak_ptr<Item> material = Game::Inst()->FindItemByCategoryFromStockpiles(*mati);
			if (boost::shared_ptr<Item> item = material.lock()) {
				item->Reserve(true);
				componentList.push_back(item);
			} else {
				for (std::list<boost::weak_ptr<Item> >::iterator compi = componentList.begin();
					compi != componentList.end(); ++compi) {
						compi->lock()->Reserve(false);
				}
				componentList.clear();
				Announce::Inst()->AddMsg((boost::format("Cancelled %s: insufficient [%s] in stockpiles") % Construction::Presets[construct].name % Item::ItemCategoryToString(*mati)).str(), TCODColor::red);
				return -1;
			}
	}

	if (Construction::AllowedAmount[construct] >= 0) {
		if (Construction::AllowedAmount[construct] == 0) {
			Announce::Inst()->AddMsg("Cannot build another "+Construction::Presets[construct].name+"!", TCODColor::red);
			return -1;
		}
		--Construction::AllowedAmount[construct];
	}
	
	for (std::list<boost::weak_ptr<Item> >::iterator compi = componentList.begin();
		compi != componentList.end(); ++compi) {
			compi->lock()->Reserve(false);
	}
	componentList.clear();

	boost::shared_ptr<Construction> newCons;
	if (Construction::Presets[construct].tags[DOOR]) {
		newCons = boost::shared_ptr<Construction>(new Door(construct, target));
	} else {
		newCons = boost::shared_ptr<Construction>(new Construction(construct, target));
	}
	if (Construction::Presets[construct].dynamic) {
		Game::Inst()->dynamicConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newCons->Uid(), newCons));
	} else {
		Game::Inst()->staticConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newCons->Uid(), newCons));
	}
	Coordinate blueprint = Construction::Blueprint(construct);
	for (int x = target.X(); x < target.X() + blueprint.X(); ++x) {
		for (int y = target.Y(); y < target.Y() + blueprint.Y(); ++y) {
			Map::Inst()->Buildable(x,y,false);
			Map::Inst()->SetConstruction(x,y,newCons->Uid());
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
	boost::shared_ptr<Stockpile> newSp( (Construction::Presets[stockpile].tags[FARMPLOT]) ? new FarmPlot(stockpile, symbol, a) : new Stockpile(stockpile, symbol, a) );
	Map::Inst()->Buildable(a.X(), a.Y(), false);
	Map::Inst()->SetConstruction(a.X(), a.Y(), newSp->Uid());
	newSp->Expand(a,b);
	if (Construction::Presets[stockpile].dynamic) {
		Game::Inst()->dynamicConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newSp->Uid(),static_cast<boost::shared_ptr<Construction> >(newSp)));
	} else {
		Game::Inst()->staticConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newSp->Uid(),static_cast<boost::shared_ptr<Construction> >(newSp)));
	}
	
	Game::Inst()->RefreshStockpiles();

	//Spawning a BUILD job is not required because stockpiles are created "built"
	return newSp->Uid();
}

//Returns Coordinate(<0,<0) if not found
Coordinate Game::FindClosestAdjacent(Coordinate pos, boost::weak_ptr<Entity> ent) {
	Coordinate closest(-9999, -9999);
	if (ent.lock()) {
		if (boost::dynamic_pointer_cast<Construction>(ent.lock())) {
			boost::shared_ptr<Construction> construct(boost::static_pointer_cast<Construction>(ent.lock()));
			for (int ix = construct->X()-1; ix <= construct->X() + Construction::Blueprint(construct->Type()).X(); ++ix) {
				for (int iy = construct->Y()-1; iy <= construct->Y() + Construction::Blueprint(construct->Type()).Y(); ++iy) {
					if (ix == construct->X()-1 || ix == construct->X() + Construction::Blueprint(construct->Type()).X() ||
						iy == construct->Y()-1 || iy == construct->Y() + Construction::Blueprint(construct->Type()).Y()) {
							if (Map::Inst()->Walkable(ix,iy)) {
								if (Distance(pos.X(), pos.Y(), ix, iy) < Distance(pos.X(), pos.Y(), closest.X(), closest.Y()))
									closest = Coordinate(ix,iy);
							}
					}
				}
			}
		} else {
			for (int ix = ent.lock()->X()-1; ix <= ent.lock()->X()+1; ++ix) {
				for (int iy = ent.lock()->Y()-1; iy <= ent.lock()->Y()+1; ++iy) {
					if (ix == ent.lock()->X()-1 || ix == ent.lock()->X()+1 ||
						iy == ent.lock()->Y()-1 || iy == ent.lock()->Y()+1) {
							if (Map::Inst()->Walkable(ix,iy)) {
								if (Distance(pos.X(), pos.Y(), ix, iy) < Distance(pos.X(), pos.Y(), closest.X(), closest.Y()))
									closest = Coordinate(ix,iy);
							}
					}
				}
			}
		}
	}
	return closest;
}

//Returns true/false depending on if the given position is adjacent to the entity
//Takes into consideration if the entity is a construction, and thus may be larger than just one tile
bool Game::Adjacent(Coordinate pos, boost::weak_ptr<Entity> ent) {
	if (ent.lock()) {
		if (boost::dynamic_pointer_cast<Construction>(ent.lock())) {
			boost::shared_ptr<Construction> construct(boost::static_pointer_cast<Construction>(ent.lock()));
			for (int ix = construct->X()-1; ix <= construct->X() + Construction::Blueprint(construct->Type()).X(); ++ix) {
				for (int iy = construct->Y()-1; iy <= construct->Y() + Construction::Blueprint(construct->Type()).Y(); ++iy) {
					if (pos.X() == ix && pos.Y() == iy) { return true; }
				}
			}
			return false;
		} else {
			for (int ix = ent.lock()->X()-1; ix <= ent.lock()->X()+1; ++ix) {
				for (int iy = ent.lock()->Y()-1; iy <= ent.lock()->Y()+1; ++iy) {
					if (pos.X() == ix && pos.Y() == iy) { return true; }
				}
			}
			return false;
		}
	}
	return false;
}

int Game::CreateNPC(Coordinate target, NPCType type) {
	boost::shared_ptr<NPC> npc(new NPC(target));
	npc->type = type;
	npc->InitializeAIFunctions();
	npc->expert = NPC::Presets[type].expert;
	npc->color(NPC::Presets[type].color);
	npc->graphic(NPC::Presets[type].graphic);

	if (NPC::Presets[type].generateName) {
		npc->name = TCODNamegen::generate(const_cast<char*>(NPC::Presets[type].name.c_str()));
	} else npc->name = NPC::Presets[type].name;

	npc->needsNutrition = NPC::Presets[type].needsNutrition;
	npc->needsSleep = NPC::Presets[type].needsSleep;
	npc->health = NPC::Presets[type].health;
	npc->maxHealth = NPC::Presets[type].health;
	for (int i = 0; i < STAT_COUNT; ++i) {
		npc->baseStats[i] = NPC::Presets[type].stats[i] + ((NPC::Presets[type].stats[i] * 0.1) * (rand() % 2) ? 1 : -1);
	}
	for (int i = 0; i < RES_COUNT; ++i) {
		npc->baseResistances[i] = NPC::Presets[type].resistances[i] + ((NPC::Presets[type].resistances[i] * 0.1) * (rand() % 2) ? 1 : -1);
	}

	npc->attacks = NPC::Presets[type].attacks;

	if (boost::iequals(NPC::NPCTypeToString(type), "orc")) ++orcCount;
	else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) ++goblinCount;

	if (NPC::Presets[type].tags.find("flying") != NPC::Presets[type].tags.end()) {
		npc->AddEffect(FLYING);
	}

	if (NPC::Presets[type].tags.find("coward") != NPC::Presets[type].tags.end()) {
		npc->coward = true;
	}

	if (NPC::Presets[type].tags.find("hashands") != NPC::Presets[type].tags.end()) {
		npc->hasHands = true;
	}

	npcList.insert(std::pair<int,boost::shared_ptr<NPC> >(npc->Uid(),npc));

	return npc->Uid();
}

int Game::OrcCount() { return orcCount; }
void Game::OrcCount(int add) { orcCount += add; }
int Game::GoblinCount() { return goblinCount; }
void Game::GoblinCount(int add) { goblinCount += add; }


//Moves the entity to a valid walkable tile
//TODO: make it find the closest walkable tile instead of going right, that will lead to weirdness and problems
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
		int newx = entity.lock()->X();
		int newy = entity.lock()->Y();
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

void Game::Exit(bool confirm) {
	boost::function<void()> exitFunc = boost::bind(&Game::Running, Game::Inst(), false);
	
	if (confirm) {
		YesNoDialog::ShowYesNoDialog("Really exit?", exitFunc, NULL);
	} else {
		exitFunc();
	}
}

int Game::ScreenWidth() const {	return screenWidth; }
int Game::ScreenHeight() const { return screenHeight; }

class ConfigListener : public ITCODParserListener {

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<"config structure begun:\n";
#endif
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "fullscreen")) {
			Game::Inst()->fullscreen = true;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "width")) {
			Game::Inst()->resolutionWidth = value.i;
		} else if (boost::iequals(name, "height")) {
			Game::Inst()->resolutionHeight = value.i;
		} else if (boost::iequals(name, "renderer")) {
			if (boost::iequals(value.s, "SDL")) {
				Game::Inst()->renderer = TCOD_RENDERER_SDL;
			} else if (boost::iequals(value.s, "OpenGL")) {
				Game::Inst()->renderer = TCOD_RENDERER_OPENGL;
			} else if (boost::iequals(value.s, "GLSL")) {
				Game::Inst()->renderer = TCOD_RENDERER_GLSL;
			}
		} 
		return true;
	}

	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<"end of config structure\n";
#endif
		return true;
	}
	void error(const char *msg) {
		Logger::Inst()->output<<"ConfigListener: "<<msg<<"\n";
		Game::Inst()->Exit();
	}
};

void Game::LoadConfig(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* configTypeStruct = parser.newStructure("config");
	configTypeStruct->addProperty("width", TCOD_TYPE_INT, true);
	configTypeStruct->addProperty("height", TCOD_TYPE_INT, true);
	configTypeStruct->addFlag("fullscreen");
	const char* renderers[] = { "GLSL", "SDL", "OpenGL", NULL }; 
	configTypeStruct->addValueList("renderer", renderers, true);

	parser.run(filename.c_str(), new ConfigListener());
}

void Game::Init() {
	if (resolutionWidth <= 0 || resolutionHeight <= 0) {
		if (fullscreen) {
			TCODSystem::getCurrentResolution(&resolutionWidth, &resolutionHeight);
		} else {
			resolutionWidth = 640;
			resolutionHeight = 480;
		}
	} 

	TCODSystem::getCharSize(&charWidth, &charHeight);
	screenWidth = resolutionWidth / charWidth;
	screenHeight = resolutionHeight / charHeight;

	srand((unsigned int)std::time(0));

	//Enabling TCOD_RENDERER_GLSL can cause GCamp to crash on exit, apparently it's because of an ATI driver issue.
	TCODConsole::initRoot(screenWidth, screenHeight, "Goblin Camp", fullscreen, renderer);
	TCODConsole::root->setAlignment(TCOD_LEFT);

	TCODMouse::showCursor(true);

	TCODConsole::setKeyboardRepeat(500, 10);
	
	Data::Load();
	
	GenerateMap();
	events = boost::shared_ptr<Events>(new Events(Map::Inst()));

	buffer = new TCODConsole(screenWidth, screenHeight);
	season = LateWinter;
	upleft = Coordinate(180,180);
}

void Game::RemoveConstruction(boost::weak_ptr<Construction> cons) {
	if (boost::shared_ptr<Construction> construct = cons.lock()) {
		Coordinate blueprint = Construction::Blueprint(construct->Type());
		for (int x = construct->X(); x < construct->X() + blueprint.X(); ++x) {
			for (int y = construct->Y(); y < construct->Y() + blueprint.Y(); ++y) {
				Map::Inst()->Buildable(x,y,true);
				Map::Inst()->SetConstruction(x,y,-1);
			}
		}
		if (Construction::Presets[construct->type].dynamic) {
			Game::Inst()->dynamicConstructionList.erase(construct->Uid());
		} else {
			Game::Inst()->staticConstructionList.erase(construct->Uid());
		}
	}
}

void Game::DismantleConstruction(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int construction = Map::Inst()->GetConstruction(x,y);
			if (construction >= 0) {
				if (instance->GetConstruction(construction).lock()) {
					instance->GetConstruction(construction).lock()->Dismantle();
				} else {
					Map::Inst()->SetConstruction(x,y,-1);
				}
			}
		}
	}
}

boost::weak_ptr<Construction> Game::GetConstruction(int uid) {
	if (staticConstructionList.find(uid) != staticConstructionList.end()) 
		return staticConstructionList[uid];
	else if (dynamicConstructionList.find(uid) != dynamicConstructionList.end())
		return dynamicConstructionList[uid];
	return boost::weak_ptr<Construction>();
}

int Game::CreateItem(Coordinate pos, ItemType type, bool store, int ownerFaction, 
	std::vector<boost::weak_ptr<Item> > comps, boost::shared_ptr<Container> container) {

		boost::shared_ptr<Item> newItem;
		if (Item::Presets[type].organic) {
			boost::shared_ptr<OrganicItem> orgItem(new OrganicItem(pos, type));
			newItem = boost::static_pointer_cast<Item>(orgItem);
			orgItem->Nutrition(Item::Presets[type].nutrition);
			orgItem->Growth(Item::Presets[type].growth);
		} else if (Item::Presets[type].container > 0) {
			newItem.reset(static_cast<Item*>(new Container(pos, type, Item::Presets[type].container, 0, comps)));
		} else {
			newItem.reset(new Item(pos, type, 0, comps));
		}

		if (!container) {
			freeItems.insert(newItem);
			Map::Inst()->ItemList(newItem->X(), newItem->Y())->insert(newItem->Uid());
		} else {
			container->AddItem(newItem);
		}
		itemList.insert(std::pair<int,boost::shared_ptr<Item> >(newItem->Uid(), newItem));
		if (store) StockpileItem(newItem);

		return newItem->Uid();
}

void Game::RemoveItem(boost::weak_ptr<Item> witem) {
	if (boost::shared_ptr<Item> item = witem.lock()) {
		Map::Inst()->ItemList(item->x, item->y)->erase(item->uid);
		if (freeItems.find(witem) != freeItems.end()) freeItems.erase(witem);
		if (boost::shared_ptr<Container> container = boost::static_pointer_cast<Container>(item->container.lock())) {
			if (container) {
				container->RemoveItem(witem);
			}
		}
		itemList.erase(item->uid);
	}
}

boost::weak_ptr<Item> Game::GetItem(int uid) {
	return itemList[uid];
}

void Game::ItemContained(boost::weak_ptr<Item> item, bool con) {
	if (!con) {
		freeItems.insert(item);
		Map::Inst()->ItemList(item.lock()->X(), item.lock()->Y())->insert(item.lock()->Uid());
	}
	else {
		freeItems.erase(item);
		Map::Inst()->ItemList(item.lock()->X(), item.lock()->Y())->erase(item.lock()->Uid());
	}
}

void Game::CreateWater(Coordinate pos) {
	CreateWater(pos, 10);
}

void Game::CreateWater(Coordinate pos, int amount, int time) {
	boost::weak_ptr<WaterNode> water(Map::Inst()->GetWater(pos.X(), pos.Y()));
	if (!water.lock()) {
		boost::shared_ptr<WaterNode> newWater(new WaterNode(pos.X(), pos.Y(), amount, time));
		waterList.push_back(boost::weak_ptr<WaterNode>(newWater));
		Map::Inst()->SetWater(pos.X(), pos.Y(), newWater);
	} else {water.lock()->Depth(water.lock()->Depth()+amount);}
}

int Game::DistanceNPCToCoordinate(int uid, Coordinate pos) {
	return Distance(npcList[uid]->X(), npcList[uid]->Y(), pos.X(), pos.Y());
}

boost::weak_ptr<Item> Game::FindItemByCategoryFromStockpiles(ItemCategory category, int flags, int value) {
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = staticConstructionList.begin(); consIter != staticConstructionList.end(); ++consIter) {
		if (consIter->second->stockpile && !consIter->second->farmplot) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByCategory(category, flags, value));
			if (item.lock() && !item.lock()->Reserved()) {
				return item;
			}
		}
	}
	return boost::weak_ptr<Item>();
}

boost::weak_ptr<Item> Game::FindItemByTypeFromStockpiles(ItemType type, int flags, int value) {
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = staticConstructionList.begin(); consIter != staticConstructionList.end(); ++consIter) {
		if (consIter->second->stockpile && !consIter->second->farmplot) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByType(type, flags, value));
			if (item.lock() && !item.lock()->Reserved()) {
				return item;
			}
		}
	}
	return boost::weak_ptr<Item>();
}

// Spawns items distributed randomly within the rectangle defined by corner1 & corner2
void Game::CreateItems(int quantity, ItemType type, Coordinate corner1, Coordinate corner2) {
	int areaWidth = std::max(abs(corner1.X()-corner2.X()),1);
	int areaLength = std::max(abs(corner1.Y()-corner2.Y()),1);
	
	for (int items = 0; items < quantity; ++items) {
		Coordinate location(rand() % areaWidth + std::min(corner1.X(),corner2.X()), rand() % areaLength + std::min(corner1.Y(),corner2.Y()));
		Game::Inst()->CreateItem(location, type, true);
	}
}



//Findwater returns the coordinates to the closest Water* that has sufficient depth
Coordinate Game::FindWater(Coordinate pos) {
	Coordinate closest(-9999,-9999);
	for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end(); ++wati) {
		if (wati->lock()->Depth() > DRINKABLE_WATER_DEPTH) {
			if (Distance(wati->lock()->Position(), pos) < Distance(closest, pos)) closest = wati->lock()->Position();
		}
	}
	if (closest.X() == -9999) return Coordinate(-1,-1);
	return closest;
}

void Game::Update() {
	++time;

	if (time == MONTH_LENGTH) {
		if (safeMonths > 0) --safeMonths;
		if (season < LateWinter) season = (Season)((int)season + 1);
		else season = EarlySpring;

		switch (season) {
		case EarlySpring:
			Announce::Inst()->AddMsg("Spring has begun");
		case Spring:
		case LateSpring:
			SpawnTillageJobs();
		case Summer:
		case LateSummer:
		case Fall:
		case LateFall:
		case Winter:
			DecayItems();
			break;

		case LateWinter:
			break;

		case EarlySummer:
			Announce::Inst()->AddMsg("Summer has begun");
			DecayItems();
			break;

		case EarlyFall:
			Announce::Inst()->AddMsg("Fall has begun");
			DecayItems();
			break;

		case EarlyWinter:
			Announce::Inst()->AddMsg("Winter has begun");
			DeTillFarmPlots();
			break;

		default: break;
		}
		time = 0;
	}

	//This actually only updates every 50th waternode. This is due to 2 things: updating one water tile actually also
	//updates all its neighbours. Also, by updating only every 50th one, the load on the cpu is less, but you need to
	//remember that Update gets called 25 times a second, and given the nature of rand() this means that each waternode
	//will be updated once every 2 seconds. It turns out that from the player's viewpoint this is just fine
	for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end(); ++wati) {
		if (wati->lock() && rand() % 50 == 0) wati->lock()->Update();
		else if (!wati->lock()) wati = waterList.erase(wati);
	}

	//Updating the last 10 waternodes each time means that recently created water moves faster.
	//This has the effect of making water rush to new places such as a moat very quickly, which is the
	//expected behaviour of water.
	if (waterList.size() > 0) {
		std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.end();
		while (std::distance(wati, waterList.end()) < 10) {
			--wati;
			if (wati->lock()) wati->lock()->Update();
		}
	}

	std::list<boost::weak_ptr<NPC> > npcsWaitingForRemoval;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = npcList.begin(); npci != npcList.end(); ++npci) {
		npci->second->Update();
		if (!npci->second->Dead()) npci->second->Think();
		if (npci->second->Dead() || npci->second->Escaped()) npcsWaitingForRemoval.push_back(npci->second);
	}

	for (std::list<boost::weak_ptr<NPC> >::iterator remNpci = npcsWaitingForRemoval.begin(); remNpci != npcsWaitingForRemoval.end(); ++remNpci) {
		RemoveNPC(*remNpci);
	}

	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = dynamicConstructionList.begin(); consi != dynamicConstructionList.end(); ++consi) {
		consi->second->Update();
	}

	for (std::list<boost::weak_ptr<Item> >::iterator itemi = stoppedItems.begin(); itemi != stoppedItems.end();) {
		flyingItems.erase(*itemi);
		itemi = stoppedItems.erase(itemi);
	}

	for (std::set<boost::weak_ptr<Item> >::iterator itemi = flyingItems.begin(); itemi != flyingItems.end(); ++itemi) {
		if (boost::shared_ptr<Item> item = itemi->lock()) item->UpdateVelocity();
	}

	//Constantly checking our free item list for items that can be stockpiled is overkill, so it's done once every
	//15 seconds, on average, or immediately if a new stockpile is built or a stockpile's allowed items are changed.
	if (rand() % (UPDATES_PER_SECOND * 15) == 0 || refreshStockpiles) {
		refreshStockpiles = false;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = freeItems.begin(); itemi != freeItems.end(); ++itemi) {
			if (itemi->lock() && !itemi->lock()->Reserved() && itemi->lock()->GetFaction() == 0 && itemi->lock()->GetVelocity() == 0) 
				StockpileItem(*itemi);
		}
	}

	//Squads needen't update their member rosters ALL THE TIME
	if (time % (UPDATES_PER_SECOND * 1) == 0) {
		for (std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = squadList.begin(); squadi != squadList.end(); ++squadi) {
			squadi->second->UpdateMembers();
		}
	}

	if (time % (UPDATES_PER_SECOND * 1) == 0) StockManager::Inst()->Update();

	if (time % (UPDATES_PER_SECOND * 1) == UPDATES_PER_SECOND/2) JobManager::Inst()->Update();

	if (safeMonths <= 0) events->Update();
}

boost::shared_ptr<Job> Game::StockpileItem(boost::weak_ptr<Item> item, bool returnJob) {
	for (std::map<int,boost::shared_ptr<Construction> >::iterator stocki = staticConstructionList.begin(); stocki != staticConstructionList.end(); ++stocki) {
		if (stocki->second->stockpile) {
			boost::shared_ptr<Stockpile> sp(boost::static_pointer_cast<Stockpile>(stocki->second));
			if (sp->Allowed(Item::Presets[item.lock()->Type()].specificCategories) && !sp->Full()) {

				//Found a stockpile that both allows the item, and has space
				//Check if the item can be contained, and if so if any containers are in the stockpile

				boost::shared_ptr<Job> stockJob(new Job("Store item", LOW));
				stockJob->Attempts(1);
				Coordinate target = Coordinate(-1,-1);
				boost::weak_ptr<Item> container;

				if (Item::Presets[item.lock()->Type()].fitsin >= 0) {
					container = sp->FindItemByCategory(Item::Presets[item.lock()->Type()].fitsin, NOTFULL);
					if (container.lock()) {
						target = container.lock()->Position();
						stockJob->ReserveSpace(boost::static_pointer_cast<Container>(container.lock()));
					}
				}

				if (target.X() == -1) target = sp->FreePosition();

				if (target.X() != -1) {
					stockJob->ReserveSpot(sp, target);
					stockJob->ReserveEntity(item);
					stockJob->tasks.push_back(Task(MOVE, item.lock()->Position()));
					stockJob->tasks.push_back(Task(TAKE, item.lock()->Position(), item));
					stockJob->tasks.push_back(Task(MOVE, target));
					if (!container.lock())
						stockJob->tasks.push_back(Task(PUTIN, target, sp->Storage(target)));
					else
						stockJob->tasks.push_back(Task(PUTIN, target, container));

					if (!returnJob) JobManager::Inst()->AddJob(stockJob);
					else return stockJob;

					return boost::shared_ptr<Job>();
				}
			}
		}
	}
	return boost::shared_ptr<Job>();
}

namespace {
	template <typename MapT>
	inline void InternalDrawMapItems(const char *name, MapT& map, Coordinate& upleft, TCODConsole *buffer) {
		for (typename MapT::iterator it = map.begin(); it != map.end(); ) {
			typename MapT::mapped_type ptr = it->second;
			
			if (ptr.get() != NULL) {
				ptr->Draw(upleft, buffer);
				++it;
			} else {
		#ifdef DEBUG
				std::cout << "!!! NULL POINTER !!! " << name << " ; id " << it->first << std::endl;
		#endif
				typename MapT::iterator tmp = it;
				++it;
				map.erase(tmp);
			}
		}
	}
}

void Game::Draw(Coordinate upleft, TCODConsole* buffer, bool drawUI) {
	Map::Inst()->Draw(upleft, buffer);
	
	InternalDrawMapItems("static constructions",  staticConstructionList, upleft, buffer);
	InternalDrawMapItems("dynamic constructions", dynamicConstructionList, upleft, buffer);
	InternalDrawMapItems("items",                 itemList, upleft, buffer);
	InternalDrawMapItems("nature objects",        natureList, upleft, buffer);
	InternalDrawMapItems("NPCs",                  npcList, upleft, buffer);
	
	if (drawUI) {
		UI::Inst()->Draw(upleft, buffer);
	}
}

void Game::FlipBuffer() {
	buffer->flush();
	TCODConsole::blit(buffer, 0, 0, screenWidth, screenHeight, TCODConsole::root, 0, 0);
	TCODConsole::root->flush();
}

Season Game::CurrentSeason() { return season; }

void Game::SpawnTillageJobs() {
	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = dynamicConstructionList.begin(); consi != dynamicConstructionList.end(); ++consi) {
		if (consi->second->farmplot) {
			boost::shared_ptr<Job> tillJob(new Job("Till farmplot"));
			tillJob->tasks.push_back(Task(MOVE, consi->second->Position()));
			tillJob->tasks.push_back(Task(USE, consi->second->Position(), consi->second));
			JobManager::Inst()->AddJob(tillJob);
		}
	}
}

void Game::DeTillFarmPlots() {
	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = dynamicConstructionList.begin(); consi != dynamicConstructionList.end(); ++consi) {
		if (consi->second->farmplot) {
			boost::static_pointer_cast<FarmPlot>(consi->second)->tilled = false;
		}
	}
}

//Placeholder, awaiting a real map generator
void Game::GenerateMap() {
	Map* map = Map::Inst();

	int basey = 260;
	for (int x = 0; x < map->Width(); ++x) {
		basey += ((rand() % 3) - 1);
		for (int y = basey; y <= basey+10; ++y) {
			if (y >= basey+4 && y <= basey+6 && rand() % 5 == 0) {
				map->Type(x,y,TILERIVERBED);
				CreateWater(Coordinate(x,y));
			} else if ((y == basey || y == basey+10) && rand() % 3 == 0) {
			} else {
				map->Type(x,y,TILEDITCH);
				CreateWater(Coordinate(x,y));
			}
		}
	}

	for (int x = 100; x < 150; ++x) {
		for (int y = 100; y < 150; ++y) {
			map->Type(x,y,TILEBOG);
		}
	}


	for (int x = 0; x < map->Width(); ++x) {
		for (int y = 0; y < map->Height(); ++y) {
			if (map->Walkable(x,y) && map->Type(x,y) == TILEGRASS) {
				if (rand() % 100 == 0) {
					int r = rand() % 100;
					for (int i = 0; i < (signed int)NatureObject::Presets.size(); ++i) {
						int type = rand() % NatureObject::Presets.size();
						if (NatureObject::Presets[type].rarity > r) {
							for (int clus = 0; clus < NatureObject::Presets[type].cluster; ++clus) {
								int ax = x + ((rand() % 5) - 2);
								int ay = y + ((rand() % 5) - 2);
								if (ax < 0) ax = 0; if (ax >= map->Width()) ax = map->Width()-1;
								if (ay < 0) ay = 0; if (ay >= map->Height()) ay = map->Height()-1;
								if (map->Walkable(ax,ay) && map->Type(ax,ay) == TILEGRASS
									&& map->NatureObject(ax,ay) < 0) {
										boost::shared_ptr<NatureObject> natObj(new NatureObject(Coordinate(ax,ay), type));
										natureList.insert(std::pair<int, boost::shared_ptr<NatureObject> >(natObj->Uid(), natObj));
										map->NatureObject(ax,ay,natObj->Uid());
										map->SetWalkable(ax,ay,NatureObject::Presets[natObj->Type()].walkable);
										map->Buildable(ax,ay,NatureObject::Presets[natObj->Type()].walkable);
										map->BlocksLight(ax,ay,!NatureObject::Presets[natObj->Type()].walkable);
								}
							}
							break;
						}
					}
				}
			}
		}
	}
 
	std::vector<NPCType> peacefulAnimals;
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find("localwildlife") != NPC::Presets[i].tags.end())
			peacefulAnimals.push_back(i);
	}

	if (peacefulAnimals.size() > 0) {
	//Generate benign fauna
		for (int i = 0; i < 10; ++i) {
			int type = rand() % peacefulAnimals.size();
			Game::Inst()->CreateNPC(Coordinate(300+rand()%20, 100+rand()%20), peacefulAnimals[type]);
		}
	}
}

//This is intentional, otherwise designating where to cut down trees would always show red unless you were over a tree
bool Game::CheckTree(Coordinate, Coordinate) {
	return true;
}

void Game::FellTree(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->NatureObject(x,y);
			if (natUid >= 0) {
				boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj.lock() && natObj.lock()->Tree() && !natObj.lock()->Marked()) {
					natObj.lock()->Mark();
					boost::shared_ptr<Job> fellJob(new Job("Fell tree", MED, 0, true));
					fellJob->ConnectToEntity(natObj);
					fellJob->tasks.push_back(Task(MOVEADJACENT, natObj.lock()->Position(), natObj));
					fellJob->tasks.push_back(Task(FELL, natObj.lock()->Position(), natObj));
					fellJob->tasks.push_back(Task(STOCKPILEITEM));
					JobManager::Inst()->AddJob(fellJob);
				}
			}
		}
	}
}

void Game::DesignateTree(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->NatureObject(x,y);
			if (natUid >= 0) {
				boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj.lock() && natObj.lock()->Tree() && !natObj.lock()->Marked()) {
					//TODO: Implement proper map marker system and change this to use that
					natObj.lock()->Mark();
					StockManager::Inst()->UpdateTreeDesignations(natObj, true);
				}
			}
		}
	}
}

void Game::HarvestWildPlant(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->NatureObject(x,y);
			if (natUid >= 0) {
				boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj.lock() && natObj.lock()->Harvestable() && !natObj.lock()->Marked()) {
					natObj.lock()->Mark();
					boost::shared_ptr<Job> harvestJob(new Job("Harvest wild plant"));
					harvestJob->ConnectToEntity(natObj);
					harvestJob->tasks.push_back(Task(MOVEADJACENT, natObj.lock()->Position(), natObj));
					harvestJob->tasks.push_back(Task(HARVESTWILDPLANT, natObj.lock()->Position(), natObj));
					harvestJob->tasks.push_back(Task(STOCKPILEITEM));
					JobManager::Inst()->AddJob(harvestJob);
				}
			}
		}
	}
}


void Game::RemoveNatureObject(boost::weak_ptr<NatureObject> natObj) {
	Map::Inst()->NatureObject(natObj.lock()->X(), natObj.lock()->Y(), -1);
	natureList.erase(natObj.lock()->Uid());
}

bool Game::CheckTileType(TileType type, Coordinate target, Coordinate size) {
	for (int x = target.X(); x < target.X()+size.X(); ++x) {
		for (int y = target.Y(); y < target.Y()+size.Y(); ++y) {
			if (Map::Inst()->Type(x,y) == type) return true;
		}
	}
	return false;
}

void Game::DesignateBog(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (Map::Inst()->Type(x,y) == TILEBOG) {
				StockManager::Inst()->UpdateBogDesignations(Coordinate(x,y), true);
				Map::Inst()->Mark(x,y);
			}
		}
	}
}

void Game::Undesignate(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {	
			int natUid = Map::Inst()->NatureObject(x,y);
			if (natUid >= 0) {
				boost::weak_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj.lock() && natObj.lock()->Tree() && natObj.lock()->Marked()) {
					//TODO: Implement proper map marker system and change this to use that
					natObj.lock()->Unmark();
					StockManager::Inst()->UpdateTreeDesignations(natObj, false);
				}
			}
			if (Map::Inst()->Type(x,y) == TILEBOG) {
				StockManager::Inst()->UpdateBogDesignations(Coordinate(x,y), false);
				Map::Inst()->Unmark(x,y);
			}
		}
	}
}

std::string Game::SeasonToString(Season season) {
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

	for (std::list<boost::weak_ptr<BloodNode> >::iterator bli = bloodList.begin(); bli != bloodList.end(); ++bli) {
		if (boost::shared_ptr<BloodNode> blood = bli->lock()) {
			blood->Depth(blood->Depth()-50);
			if (blood->Depth() <= 0) {
				Map::Inst()->SetBlood(blood->Position().X(), blood->Position().Y(), boost::shared_ptr<BloodNode>());
				bli = bloodList.erase(bli);
			}
		} else {
			bli = bloodList.erase(bli);
		}
	}
}

void Game::CreateFilth(Coordinate pos) {
	CreateFilth(pos, 1);
}

void Game::CreateFilth(Coordinate pos, int amount) {
	boost::weak_ptr<FilthNode> filth(Map::Inst()->GetFilth(pos.X(), pos.Y()));
	if (!filth.lock()) {
		boost::shared_ptr<FilthNode> newFilth(new FilthNode(pos.X(), pos.Y(), amount));
		filthList.push_back(boost::weak_ptr<FilthNode>(newFilth));
		Map::Inst()->SetFilth(pos.X(), pos.Y(), newFilth);
	} else {filth.lock()->Depth(filth.lock()->Depth()+amount);}
}

void Game::CreateBlood(Coordinate pos) {
	CreateBlood(pos, 100);
}

void Game::CreateBlood(Coordinate pos, int amount) {
	boost::weak_ptr<BloodNode> blood(Map::Inst()->GetBlood(pos.X(), pos.Y()));
	if (!blood.lock()) {
		boost::shared_ptr<BloodNode> newBlood(new BloodNode(pos.X(), pos.Y(), amount));
		bloodList.push_back(boost::weak_ptr<BloodNode>(newBlood));
		Map::Inst()->SetBlood(pos.X(), pos.Y(), newBlood);
	} else {blood.lock()->Depth(blood.lock()->Depth()+amount);}
}


//This function uses straightforward raycasting, and it is somewhat imprecise right now as it only casts a ray to every second
//tile at the edge of the line of sight distance. This is to conserve cpu cycles, as there may be several hundred creatures
//active at a time, and given the fact that they'll usually be constantly moving, this function needen't be 100% accurate.
void Game::FindNearbyNPCs(boost::shared_ptr<NPC> npc, bool onlyHostiles) {
	npc->nearNpcs.clear();
	for (int endx = std::max((signed int)npc->x - LOS_DISTANCE, 0); endx <= std::min((signed int)npc->x + LOS_DISTANCE, Map::Inst()->Width()-1); endx += 2) {
		for (int endy = std::max((signed int)npc->y - LOS_DISTANCE, 0); endy <= std::min((signed int)npc->y + LOS_DISTANCE, Map::Inst()->Height()-1); endy += 2) {
			if (endx == std::max((signed int)npc->x - LOS_DISTANCE, 0) || endx == std::min((signed int)npc->x + LOS_DISTANCE, Map::Inst()->Width()-1)
				|| endy == std::max((signed int)npc->y - LOS_DISTANCE, 0) || endy == std::min((signed int)npc->y + LOS_DISTANCE, Map::Inst()->Height()-1)) {
					int x = npc->x;
					int y = npc->y;
					TCODLine::init(x, y, endx, endy);
					do {
						if (Map::Inst()->BlocksLight(x,y)) break;
						for (std::set<int>::iterator npci = Map::Inst()->NPCList(x,y)->begin(); npci != Map::Inst()->NPCList(x,y)->end(); ++npci) {
							if (*npci != npc->uid) {
								if (!onlyHostiles || (onlyHostiles && npcList[*npci]->faction != npc->faction)) npc->nearNpcs.push_back(npcList[*npci]);
							}
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

void Game::RemoveNPC(boost::weak_ptr<NPC> wnpc) {
	if (boost::shared_ptr<NPC> npc = wnpc.lock()) {
		if (boost::iequals(npc->name, "orc")) --orcCount;
		else if (boost::iequals(npc->name, "goblin")) --goblinCount;
		npcList.erase(npc->uid);
	}
}

int Game::FindMilitaryRecruit() {
	for (std::map<int, boost::shared_ptr<NPC> >::iterator npci = npcList.begin(); npci != npcList.end(); ++npci) {
		if (npci->second->type == NPC::StringToNPCType("orc") && npci->second->faction == 0 && !npci->second->squad.lock()) {
			return npci->second->uid;
		}
	}
	return -1;
}

void Game::CreateSquad(std::string name) {
	squadList.insert(std::pair<std::string, boost::shared_ptr<Squad> >(name, boost::shared_ptr<Squad>(new Squad(name))));
}

void Game::SetSquadTargetCoordinate(Coordinate target, boost::shared_ptr<Squad> squad) {
	squad->TargetCoordinate(target);
	UI::Inst()->CloseMenu();
	Announce::Inst()->AddMsg((boost::format("[%1%] guarding position (%2%,%3%)") % squad->Name() % target.X() % target.Y()).str(), TCODColor::white, target);
}
void Game::SetSquadTargetEntity(Coordinate target, boost::shared_ptr<Squad> squad) {
	if (target.X() >= 0 && target.X() < Map::Inst()->Width() && target.Y() >= 0 && target.Y() < Map::Inst()->Height()) {
		std::set<int> *npcList = Map::Inst()->NPCList(target.X(), target.Y());
		if (!npcList->empty()) {
			squad->TargetEntity(Game::Inst()->npcList[*npcList->begin()]);
			UI::Inst()->CloseMenu();
			Announce::Inst()->AddMsg((boost::format("[%1%] escorting %2%") % squad->Name() % squad->TargetEntity().lock()->Name()).str(), TCODColor::white, target);
		}
	}
}

// Spawns NPCs distributed randomly within the rectangle defined by corner1 & corner2
void Game::CreateNPCs(int quantity, NPCType type, Coordinate corner1, Coordinate corner2) {
	int areaWidth = std::max(abs(corner1.X()-corner2.X()), 1);
	int areaLength = std::max(abs(corner1.Y()-corner2.Y()), 1);
	
	for (int npcs = 0; npcs < quantity; ++npcs) {
		Coordinate location(rand() % areaWidth + std::min(corner1.X(),corner2.X()), rand() % areaLength + std::min(corner1.Y(),corner2.Y()));

		Game::Inst()->CreateNPC(location, type);
	}
}

int Game::DiceToInt(TCOD_dice_t dice) {
	if (dice.nb_faces == 0) 
		dice.nb_faces = 1;
	return (int)(((dice.nb_dices * ((rand() % dice.nb_faces) + 1)) *
		dice.multiplier) + dice.addsub);
}

void Game::ToMainMenu(bool value) { Game::Inst()->toMainMenu = value; }
bool Game::ToMainMenu() { return Game::Inst()->toMainMenu; }

void Game::Running(bool value) { running = value; }
bool Game::Running() { return running; }

boost::weak_ptr<Construction> Game::FindConstructionByTag(ConstructionTag tag) {
	for (std::map<int, boost::shared_ptr<Construction> >::iterator stati = staticConstructionList.begin();
		stati != staticConstructionList.end(); ++stati) {
			if (!stati->second->Reserved() && stati->second->HasTag(tag)) return stati->second;
	}

	for (std::map<int, boost::shared_ptr<Construction> >::iterator dynai = dynamicConstructionList.begin();
		dynai != dynamicConstructionList.end(); ++dynai) {
			if (!dynai->second->Reserved() && dynai->second->HasTag(tag)) return dynai->second;
	}

	return boost::weak_ptr<Construction>();
}

void Game::Reset() {
	npcList.clear();
	squadList.clear();
	hostileSquadList.clear();
	itemList.clear();
	staticConstructionList.clear();
	dynamicConstructionList.clear();
	natureList.clear();
	waterList.clear();
	filthList.clear();
	bloodList.clear();
	JobManager::Inst()->Reset();
	StockManager::Inst()->Reset();
	time = 0;
	paused = false;
	toMainMenu = false;
	running = false;
	events = boost::shared_ptr<Events>(new Events(Map::Inst()));
	season = LateWinter;
	upleft = Coordinate(180,180);
	safeMonths = 9;
	Announce::Inst()->Reset();
	for (int x = 0; x < Map::Inst()->Width(); ++x) {
		for (int y = 0; y < Map::Inst()->Height(); ++y) {
			Map::Inst()->Reset(x,y);
		}
	}
}

NPCType Game::GetRandomNPCTypeByTag(std::string tag) {
	std::vector<NPCType> npcList;
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find(boost::to_lower_copy(tag)) != NPC::Presets[i].tags.end()) {
			npcList.push_back(i);
		}
	}
	if (npcList.size() > 0)
		return npcList[rand() % npcList.size()];
	return -1;
}

void Game::CenterOn(Coordinate target) {
	int x = std::max(0, target.X() - ScreenWidth() / 2);
	int y = std::max(0, target.Y() - ScreenHeight() / 2);
	upleft = Coordinate(x, y);
}

void Game::SetMark(int i) {
	marks[i] = Coordinate(upleft.X(), upleft.Y());
}

void Game::ReturnToMark(int i) {
	upleft = Coordinate(marks[i].X(), marks[i].Y());
}
