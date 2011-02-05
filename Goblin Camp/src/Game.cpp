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

#ifdef DEBUG
#include <iostream>
#endif

#include "Random.hpp"
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
#include "data/Config.hpp"
#include "scripting/Engine.hpp"
#include "scripting/Event.hpp"
#include "SpawningPool.hpp"
#include "Camp.hpp"
#include "MapMarker.hpp"
#include "UI/MessageBox.hpp"

#include "TCODMapRenderer.hpp"
#include "tileRenderer/TileSetLoader.hpp"
#include "tileRenderer/TileSetRenderer.hpp"
#include "MathEx.hpp"

// TODO: Temporary
#include "data/Paths.hpp"


int Game::ItemTypeCount = 0;
int Game::ItemCatCount = 0;

Game* Game::instance = 0;

Game::Game() :
screenWidth(0),
	screenHeight(0),
	season(EarlySpring),
	time(0),
	age(0),
	orcCount(0),
	goblinCount(0),
	peacefulFaunaCount(0),
	paused(false),
	toMainMenu(false),
	running(false),
	safeMonths(9),
	devMode(false),
	events(boost::shared_ptr<Events>()),
	camX(0),
	camY(0),
	buffer(0)
{
	for(int i = 0; i < 12; i++) {
		marks[i] = Coordinate(-1, -1);
	}
}

Game::~Game() {
	if (buffer) {
		delete buffer;
	}
}

Game* Game::Inst() {
	if (!instance) instance = new Game();
	return instance;
}

//Checks whether all the tiles under the rectangle (target is the up-left corner) are buildable
bool Game::CheckPlacement(Coordinate target, Coordinate size, std::set<TileType> tileReqs) {
	for (int x = target.X(); x < target.X() + size.X(); ++x) {
		for (int y = target.Y(); y < target.Y() + size.Y(); ++y) {
			if (x < 0 || y < 0 || x >= Map::Inst()->Width() || y >= Map::Inst()->Height() || !Map::Inst()->IsBuildable(x,y) || (!tileReqs.empty() && tileReqs.find(Map::Inst()->Type(x,y)) == tileReqs.end()) ) return false;
		}
	}
	return true;
}

int Game::PlaceConstruction(Coordinate target, ConstructionType construct) {
	//Check if the required materials exist before creating the build job
	std::list<boost::weak_ptr<Item> > componentList;
	for (std::list<ItemCategory>::iterator mati = Construction::Presets[construct].materials.begin();
		mati != Construction::Presets[construct].materials.end(); ++mati) {
			boost::weak_ptr<Item> material = Game::Inst()->FindItemByCategoryFromStockpiles(*mati, target, EMPTY);
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
	} else if (Construction::Presets[construct].tags[SPAWNINGPOOL]) {
		newCons = boost::shared_ptr<Construction>(new SpawningPool(construct, target));
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
			Map::Inst()->SetBuildable(x,y,false);
			Map::Inst()->SetConstruction(x,y,newCons->Uid());
			Map::Inst()->SetTerritory(x,y,true);
		}
	}

	boost::shared_ptr<Job> buildJob(new Job("Build " + Construction::Presets[construct].name, MED, 0, false));

	for (std::list<ItemCategory>::iterator materialIter = newCons->MaterialList()->begin(); materialIter != newCons->MaterialList()->end(); ++materialIter) {
		boost::shared_ptr<Job> pickupJob(new Job("Pickup " + Item::ItemCategoryToString(*materialIter) + " for " + Construction::Presets[construct].name, MED, 0, true));
		pickupJob->Parent(buildJob);
		buildJob->PreReqs()->push_back(pickupJob);

		pickupJob->tasks.push_back(Task(FIND, target, boost::weak_ptr<Entity>(), *materialIter, EMPTY));
		pickupJob->tasks.push_back(Task(MOVE));
		pickupJob->tasks.push_back(Task(TAKE));
		pickupJob->tasks.push_back(Task(MOVE, newCons->Storage().lock()->Position(), newCons));
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

	//Find a tile from a to b that is buildable
	for (int y = a.Y(); y <= b.Y(); ++y) {
		for (int x = a.X(); x <= b.X(); ++x) {
			if (Map::Inst()->IsBuildable(x,y)) {
				a = Coordinate(x,y);
				goto ContinuePlaceStockpile;
			}
		}
	}
	return -1; //No buildable tiles

ContinuePlaceStockpile:
	boost::shared_ptr<Stockpile> newSp( (Construction::Presets[stockpile].tags[FARMPLOT]) ? new FarmPlot(stockpile, symbol, a) : new Stockpile(stockpile, symbol, a) );
	Map::Inst()->SetBuildable(a.X(), a.Y(), false);
	Map::Inst()->SetConstruction(a.X(), a.Y(), newSp->Uid());
	Map::Inst()->SetTerritory(a.X(), a.Y(), true);
	newSp->Expand(a,b);
	if (Construction::Presets[stockpile].dynamic) {
		Game::Inst()->dynamicConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newSp->Uid(),static_cast<boost::shared_ptr<Construction> >(newSp)));
	} else {
		Game::Inst()->staticConstructionList.insert(std::pair<int,boost::shared_ptr<Construction> >(newSp->Uid(),static_cast<boost::shared_ptr<Construction> >(newSp)));
	}

	Game::Inst()->RefreshStockpiles();

	Script::Event::BuildingCreated(newSp, a.X(), a.Y());

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
							if (Map::Inst()->IsWalkable(ix,iy)) {
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
							if (Map::Inst()->IsWalkable(ix,iy)) {
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

	int tries = 0;
	int radius = 1;
	Coordinate originalTarget = target;
	while (!Map::Inst()->IsWalkable(target.X(), target.Y()) && tries < 20) {
		target.X(originalTarget.X() + Random::Generate(-radius, radius));
		target.Y(originalTarget.Y() + Random::Generate(-radius, radius));
		if (++tries % 3 == 0) ++radius;
	}

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
		npc->baseStats[i] = NPC::Presets[type].stats[i] + (int)Random::Sign(NPC::Presets[type].stats[i] * 0.1);
	}
	for (int i = 0; i < RES_COUNT; ++i) {
		npc->baseResistances[i] = NPC::Presets[type].resistances[i] + (int)Random::Sign(NPC::Presets[type].resistances[i] * 0.1);
	}

	npc->attacks = NPC::Presets[type].attacks;
	for (std::list<Attack>::iterator attacki = NPC::Presets[type].attacks.begin(); attacki != NPC::Presets[type].attacks.end(); ++attacki) {
		if (attacki->IsProjectileMagic()) {
			npc->hasMagicRangedAttacks = true;
			break;
		}
	}

	if (boost::iequals(NPC::NPCTypeToString(type), "orc")) {
		++orcCount;
		npc->AddTrait(FRESH);
	}
	else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) {
		++goblinCount;
		if (Random::Generate(2) == 0) npc->AddTrait(CHICKENHEART);
	}
	else if (NPC::Presets[type].tags.find("localwildlife") != NPC::Presets[type].tags.end()) ++peacefulFaunaCount;

	if (NPC::Presets[type].tags.find("flying") != NPC::Presets[type].tags.end()) {
		npc->AddEffect(FLYING);
	}

	if (NPC::Presets[type].tags.find("coward") != NPC::Presets[type].tags.end()) {
		npc->coward = true;
	}

	if (NPC::Presets[type].tags.find("hashands") != NPC::Presets[type].tags.end()) {
		npc->hasHands = true;
	}

	if (NPC::Presets[type].tags.find("tunneler") != NPC::Presets[type].tags.end()) {
		npc->isTunneler = true;
	}

	npcList.insert(std::pair<int,boost::shared_ptr<NPC> >(npc->Uid(),npc));

	return npc->Uid();
}

int Game::OrcCount() const { return orcCount; }
void Game::OrcCount(int add) { orcCount += add; }
int Game::GoblinCount() const { return goblinCount; }
void Game::GoblinCount(int add) { goblinCount += add; }


//Moves the entity to a valid walkable tile
void Game::BumpEntity(int uid) {
	boost::shared_ptr<Entity> entity;

	std::map<int,boost::shared_ptr<NPC> >::iterator npc = npcList.find(uid);
	if (npc != npcList.end()) {
		entity = npc->second;
	} else {
		std::map<int,boost::shared_ptr<Item> >::iterator item = itemList.find(uid);
		if (item != itemList.end()) {
			entity = item->second;
		}
	}

	if (entity) {
		if (!Map::Inst()->IsWalkable(entity->Position().X(), entity->Position().Y())) {
			for (int radius = 1; radius < 10; ++radius) {
				for (int xi = entity->Position().X() - radius; xi <= entity->Position().X() + radius; ++xi) {
					for (int yi = entity->Position().Y() - radius; yi <= entity->Position().Y() + radius; ++yi) {
						if (Map::Inst()->IsWalkable(xi, yi)) {
							entity->Position(Coordinate(xi, yi));
							return;
						}
					}
				}
			}
		}
	} 
#ifdef DEBUG
	else { std::cout<<"\nTried to bump nonexistant entity."; }
#endif
}

void Game::DoNothing() {}

void Game::Exit(bool confirm) {
	boost::function<void()> exitFunc = boost::bind(&Game::Running, Game::Inst(), false);

	if (confirm) {
		MessageBox::ShowMessageBox("Really exit?", exitFunc, "Yes", NULL, "No");
	} else {
		exitFunc();
	}
}

int Game::ScreenWidth() const {	return screenWidth; }
int Game::ScreenHeight() const { return screenHeight; }

void Game::LoadingScreen() {
	TCODConsole::root->setDefaultForeground(TCODColor::white);
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->setAlignment(TCOD_CENTER);
	TCODConsole::root->clear();
	TCODConsole::root->print(screenWidth / 2, screenHeight / 2, "Loading...");
	TCODConsole::root->flush();
}

void Game::ErrorScreen() {
	TCODConsole::root->setDefaultForeground(TCODColor::white);
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->setAlignment(TCOD_CENTER);
	TCODConsole::root->clear();
	TCODConsole::root->print(
		screenWidth / 2, screenHeight / 2,
		"There was an error loading the data files, refer to the logfile for more information."
	);
	TCODConsole::root->print(screenWidth / 2, screenHeight / 2 + 1, "Press any key to exit the game.");
	TCODConsole::root->flush();
	TCODConsole::waitForKeypress(true);
	exit(255);
}

void Game::Init() {
	int width  = Config::GetCVar<int>("resolutionX");
	int height = Config::GetCVar<int>("resolutionY");
	bool fullscreen = Config::GetCVar<bool>("fullscreen");
	bool useTileset = Config::GetCVar<bool>("useTileset");

	if (width <= 0 || height <= 0) {
		if (fullscreen) {
			TCODSystem::getCurrentResolution(&width, &height);
		} else {
			width  = 640;
			height = 480;
		}
	}

	TCODSystem::getCharSize(&charWidth, &charHeight);
	screenWidth  = width / charWidth;
	screenHeight = height / charHeight;

	srand((unsigned int)std::time(0));

	//Enabling TCOD_RENDERER_GLSL can cause GCamp to crash on exit, apparently it's because of an ATI driver issue.
	TCOD_renderer_t renderer_type = static_cast<TCOD_renderer_t>(Config::GetCVar<int>("renderer"));
	TCODConsole::initRoot(screenWidth, screenHeight, "Goblin Camp", fullscreen, renderer_type);
	TCODMouse::showCursor(true);
	TCODConsole::setKeyboardRepeat(500, 10);

	buffer = new TCODConsole(screenWidth, screenHeight);
	if (renderer_type == TCOD_RENDERER_SDL && useTileset) {
		TileSetLoader loader;
		std::string tilesetName = Config::GetStringCVar("tileset");
		// Try to load the configured tileset, else fallback on the default tileset, else revert to TCOD rendering
		bool foundTileset = false;
		if (tilesetName.size() > 0) {
			foundTileset = loader.LoadTileSet(Paths::Get(Paths::Tilesets) / tilesetName / "tileset.dat");
		}
		if (!foundTileset) {
			foundTileset = loader.LoadTileSet(Paths::Get(Paths::GlobalData) / "tiles" / "tileset.dat");
		}

		if (foundTileset)
		{
			renderer = boost::shared_ptr<MapRenderer>(new TileSetRenderer(width, height, loader.LoadedTileSet(), buffer));
		}
		else
		{
			renderer = boost::shared_ptr<MapRenderer>(new TCODMapRenderer(buffer)); 
		}
	} else {
		renderer = boost::shared_ptr<MapRenderer>(new TCODMapRenderer(buffer));
	}

	LoadingScreen();

	events = boost::shared_ptr<Events>(new Events(Map::Inst()));
	
	season = LateWinter;
	camX = 180;
	camY = 180;
}

void Game::TilesetChanged() {
	// For now just recreate the whole renderer
	bool useTileset = Config::GetCVar<bool>("useTileset");

	if (TCODSystem::getRenderer() == TCOD_RENDERER_SDL && useTileset) {
		TileSetLoader loader;
		std::string tilesetName = Config::GetStringCVar("tileset");
		// Try to load the configured tileset, else fallback on the default tileset, else revert to TCOD rendering
		bool foundTileset = false;
		if (tilesetName.size() > 0) {
			foundTileset = loader.LoadTileSet(Paths::Get(Paths::Tilesets) / tilesetName / "tileset.dat");
		}
		if (!foundTileset) {
			foundTileset = loader.LoadTileSet(Paths::Get(Paths::GlobalData) / "tiles" / "tileset.dat");
		}

		int screenWidth, screenHeight;
		TCODSystem::getCurrentResolution(&screenWidth, &screenHeight);

		if (foundTileset)
		{
			renderer = boost::shared_ptr<MapRenderer>(new TileSetRenderer(screenWidth, screenHeight, loader.LoadedTileSet(), buffer));
		}
		else
		{
			renderer = boost::shared_ptr<MapRenderer>(new TCODMapRenderer(buffer)); 
		}
	}
	if (running) {
		renderer->PreparePrefabs();
	}
}

void Game::RemoveConstruction(boost::weak_ptr<Construction> cons) {
	if (boost::shared_ptr<Construction> construct = cons.lock()) {
		Coordinate blueprint = Construction::Blueprint(construct->Type());
		for (int x = construct->X(); x < construct->X() + blueprint.X(); ++x) {
			for (int y = construct->Y(); y < construct->Y() + blueprint.Y(); ++y) {
				Map::Inst()->SetBuildable(x,y,true);
				Map::Inst()->SetConstruction(x,y,-1);
			}
		}

		Script::Event::BuildingDestroyed(cons, construct->X(), construct->Y());

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
					instance->GetConstruction(construction).lock()->Dismantle(Coordinate(x,y));
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
			orgItem->SetFaction(ownerFaction);
		} else if (Item::Presets[type].container > 0) {
			newItem.reset(static_cast<Item*>(new Container(pos, type, Item::Presets[type].container, ownerFaction, comps)));
		} else {
			newItem.reset(new Item(pos, type, ownerFaction, comps));
		}

		if (!container) {
			freeItems.insert(newItem);
			Map::Inst()->ItemList(newItem->X(), newItem->Y())->insert(newItem->Uid());
		} else {
			container->AddItem(newItem);
		}
		itemList.insert(std::pair<int,boost::shared_ptr<Item> >(newItem->Uid(), newItem));
		if (store) StockpileItem(newItem, false, true);

		Script::Event::ItemCreated(newItem, pos.X(), pos.Y());

#ifdef DEBUG
		std::cout<<newItem->name<<"("<<newItem->Uid()<<") created\n";
#endif

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
	if (itemList.find(uid) != itemList.end()) return itemList[uid];
	return boost::weak_ptr<Item>();
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

	//If there is filth here mix it with the water
	boost::shared_ptr<FilthNode> filth = Map::Inst()->GetFilth(pos.X(), pos.Y()).lock();

	boost::weak_ptr<WaterNode> water(Map::Inst()->GetWater(pos.X(), pos.Y()));
	if (!water.lock()) {
		boost::shared_ptr<WaterNode> newWater(new WaterNode(pos.X(), pos.Y(), amount, time));
		waterList.push_back(boost::weak_ptr<WaterNode>(newWater));
		Map::Inst()->SetWater(pos.X(), pos.Y(), newWater);
		if (filth) newWater->AddFilth(filth->Depth());
	} else {
		water.lock()->Depth(water.lock()->Depth()+amount);
		if (filth) water.lock()->AddFilth(filth->Depth());
	}

	if (filth) RemoveFilth(pos);
}

int Game::DistanceNPCToCoordinate(int uid, Coordinate pos) {
	return Distance(npcList[uid]->X(), npcList[uid]->Y(), pos.X(), pos.Y());
}

// TODO this currently checks every stockpile.  We could maintain some data structure that allowed us to check the closest stockpile(s)
// first.
boost::weak_ptr<Item> Game::FindItemByCategoryFromStockpiles(ItemCategory category, Coordinate target, int flags, int value) {
	int nearestDistance = INT_MAX;
	boost::weak_ptr<Item> nearest = boost::weak_ptr<Item>();
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = staticConstructionList.begin(); consIter != staticConstructionList.end(); ++consIter) {
		if (consIter->second->stockpile && !consIter->second->farmplot) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByCategory(category, flags, value));
			if (item.lock() && !item.lock()->Reserved()) {
				int distance = Distance(item.lock()->Position(), target);
				if(distance < nearestDistance) {
					nearestDistance = distance;
					nearest = item;
				}
			}
		}
	}
	return nearest;
}

// TODO this currently checks every stockpile.  We could maintain some data structure that allowed us to check the closest stockpile(s)
// first.
boost::weak_ptr<Item> Game::FindItemByTypeFromStockpiles(ItemType type, Coordinate target, int flags, int value) {
	int nearestDistance = INT_MAX;
	boost::weak_ptr<Item> nearest = boost::weak_ptr<Item>();
	for (std::map<int, boost::shared_ptr<Construction> >::iterator consIter = staticConstructionList.begin(); consIter != staticConstructionList.end(); ++consIter) {
		if (consIter->second->stockpile && !consIter->second->farmplot) {
			boost::weak_ptr<Item> item(boost::static_pointer_cast<Stockpile>(consIter->second)->FindItemByType(type, flags, value));
			if (item.lock() && !item.lock()->Reserved()) {
				int distance = Distance(item.lock()->Position(), target);
				if(distance < nearestDistance) {
					nearestDistance = distance;
					nearest = item;
				}
			}
		}
	}
	return nearest;
}

// Spawns items distributed randomly within the rectangle defined by corner1 & corner2
void Game::CreateItems(int quantity, ItemType type, Coordinate corner1, Coordinate corner2) {
	int areaWidth = std::max(abs(corner1.X()-corner2.X()),1);
	int areaLength = std::max(abs(corner1.Y()-corner2.Y()),1);
	int minX = std::min(corner1.X(), corner2.X());
	int minY = std::min(corner1.Y(), corner2.Y());

	for (int items = 0, count = 0; items < quantity && count < quantity*10; ++count) {
		Coordinate location(
			Random::Generate(minX, areaWidth + minX - 1),
			Random::Generate(minY, areaLength + minY - 1)
		);

		if (Map::Inst()->IsWalkable(location.X(), location.Y())) {
			Game::Inst()->CreateItem(location, type, true);
			++items;
		}
	}
}

Coordinate Game::FindFilth(Coordinate pos) {
	if (filthList.size() == 0) return Coordinate(-1,-1);
	//Choose random filth
	std::priority_queue<std::pair<int, int> > potentialFilth;
	for (size_t i = 0; i < std::min((size_t)30, filthList.size()); ++i) {
		unsigned filth = Random::ChooseIndex(filthList);
		if (boost::next(filthList.begin(), filth)->lock()->Depth() > 0) {
			potentialFilth.push(std::pair<int,int>(Distance(pos, boost::next(filthList.begin(), filth)->lock()->Position()), filth));
			if (potentialFilth.top().first < 10) break; //Near enough
		}
	}
	if (potentialFilth.size() > 0)
		return boost::next(filthList.begin(), potentialFilth.top().second)->lock()->Position();
	else
		return Coordinate(-1,-1);
}

//Findwater returns the coordinates to the closest Water* that has sufficient depth and is coastal
Coordinate Game::FindWater(Coordinate pos) {
	Coordinate closest(-1,-1);
	int closestDistance = INT_MAX;
	for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end(); ++wati) {
		if (boost::shared_ptr<WaterNode> water = wati->lock()) {
			if (water->IsCoastal() && water->Depth() > DRINKABLE_WATER_DEPTH) {
				int waterDistance = Distance(water->Position(), pos);
				//Favor water inside territory
				if (Map::Inst()->IsTerritory(water->Position().X(), water->Position().Y())) waterDistance /= 4;
				if (waterDistance < closestDistance) { 
					closest = water->Position();
					closestDistance = waterDistance;
				}
			}
		}
	}
	return closest;
}

void Game::Update() {
	++time;

	if (time == MONTH_LENGTH) {
		if (safeMonths > 0) --safeMonths;

		Map::Inst()->ShiftWind();

		for (std::map<int, boost::shared_ptr<Construction> >::iterator cons = staticConstructionList.begin();
			cons != staticConstructionList.end(); ++cons) { cons->second->SpawnRepairJob(); }
		for (std::map<int, boost::shared_ptr<Construction> >::iterator cons = dynamicConstructionList.begin();
			cons != dynamicConstructionList.end(); ++cons) { cons->second->SpawnRepairJob(); }

		if (season < LateWinter) season = (Season)((int)season + 1);
		else season = EarlySpring;

		switch (season) {
		case EarlySpring:
			Announce::Inst()->AddMsg("Spring has begun");
			++age;
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
	{
		std::list<boost::weak_ptr<WaterNode> >::iterator nextWati = ++waterList.begin();
		for (std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.begin(); wati != waterList.end();) {
			if (boost::shared_ptr<WaterNode> water = wati->lock()) {
				if (Random::Generate(49) == 0) water->Update();
			} else waterList.erase(wati);
			wati = nextWati;
			++nextWati;
		}
	}
	//Updating the last 10 waternodes each time means that recently created water moves faster.
	//This has the effect of making water rush to new places such as a moat very quickly, which is the
	//expected behaviour of water.
	if (waterList.size() > 0) {
		//We have to use two iterators, because wati may be invalidated if the water evaporates and is removed
		std::list<boost::weak_ptr<WaterNode> >::iterator wati = waterList.end();
		std::list<boost::weak_ptr<WaterNode> >::iterator nextwati = --wati;
		while (std::distance(wati, waterList.end()) < 10) {
			--nextwati;
			if (wati == waterList.end()) break;
			if (wati->lock()) wati->lock()->Update();
			wati = nextwati;
		}
	}

	std::list<boost::weak_ptr<NPC> > npcsWaitingForRemoval;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = npcList.begin(); npci != npcList.end(); ++npci) {
		npci->second->Update();
		if (!npci->second->Dead()) npci->second->Think();
		if (npci->second->Dead() || npci->second->Escaped()) npcsWaitingForRemoval.push_back(npci->second);
	}
	JobManager::Inst()->AssignJobs();

	for (std::list<boost::weak_ptr<NPC> >::iterator remNpci = npcsWaitingForRemoval.begin(); remNpci != npcsWaitingForRemoval.end(); ++remNpci) {
		RemoveNPC(*remNpci);
	}

	for (std::map<int,boost::shared_ptr<Construction> >::iterator consi = dynamicConstructionList.begin(); consi != dynamicConstructionList.end(); ++consi) {
		consi->second->Update();
	}

	for (std::list<boost::weak_ptr<Item> >::iterator itemi = stoppedItems.begin(); itemi != stoppedItems.end();) {
		flyingItems.erase(*itemi);
		if (boost::shared_ptr<Item> item = itemi->lock()) {
			if (item->condition == 0) { //The impact has destroyed the item
				RemoveItem(item);
			}
		}
		itemi = stoppedItems.erase(itemi);
	}

	for (std::set<boost::weak_ptr<Item> >::iterator itemi = flyingItems.begin(); itemi != flyingItems.end(); ++itemi) {
		if (boost::shared_ptr<Item> item = itemi->lock()) item->UpdateVelocity();
	}

	/*Constantly checking our free item list for items that can be stockpiled is overkill, so it's done once every
	5 seconds, on average, or immediately if a new stockpile is built or a stockpile's allowed items are changed.
	To further reduce load when very many free items exist, only a quarter of them will be checked*/
	if (Random::Generate(UPDATES_PER_SECOND * 5 - 1) == 0 || refreshStockpiles) {
		refreshStockpiles = false;
		if (freeItems.size() < 100) {
			for (std::set<boost::weak_ptr<Item> >::iterator itemi = freeItems.begin(); itemi != freeItems.end(); ++itemi) {
				if (boost::shared_ptr<Item> item = itemi->lock()) {
					if (!item->Reserved() && item->GetFaction() == 0 && item->GetVelocity() == 0) 
						StockpileItem(item);
				}
			}
		} else {
			for (unsigned int i = 0; i < std::max((size_t)100, freeItems.size()/4); ++i) {
				std::set<boost::weak_ptr<Item> >::iterator itemi = boost::next(freeItems.begin(), Random::ChooseIndex(freeItems));
				if (boost::shared_ptr<Item> item = itemi->lock()) {
					if (!item->Reserved() && item->GetFaction() == 0 && item->GetVelocity() == 0) 
						StockpileItem(item);
				}
			}
		}
	}

	//Squads needen't update their member rosters ALL THE TIME
	if (time % (UPDATES_PER_SECOND * 1) == 0) {
		for (std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = squadList.begin(); squadi != squadList.end(); ++squadi) {
			squadi->second->UpdateMembers();
		}
	}

	if (time % (UPDATES_PER_SECOND * 1) == 0) StockManager::Inst()->Update();

	if (time % (UPDATES_PER_SECOND * 1) == 0) JobManager::Inst()->Update();

	events->Update(safeMonths > 0);

	if (time % (UPDATES_PER_SECOND * 1) == 0) Map::Inst()->Naturify(Random::Generate(Map::Inst()->Width() - 1), Random::Generate(Map::Inst()->Height() - 1));

	if (time % (UPDATES_PER_SECOND * 1) == 0) Camp::Inst()->Update();

	Map::Inst()->UpdateMarkers();

	for (std::list<std::pair<int, boost::function<void()> > >::iterator delit = delays.begin(); delit != delays.end();) {
		if (--delit->first <= 0) {
			try {
				delit->second();
			} catch (const py::error_already_set&) {
				Script::LogException();
			}
			delit = delays.erase(delit);
		} else ++delit;
	}

	if (orcCount == 0 && goblinCount == 0) GameOver();

	for (std::list<boost::weak_ptr<FireNode> >::iterator fireit = fireList.begin(); fireit != fireList.end();) {
		if (boost::shared_ptr<FireNode> fire = fireit->lock()) {
			if (Random::GenerateBool()) fire->Update();
			if (fire->GetHeat() <= 0) {
				Map::Inst()->SetFire(fire->GetPosition().X(), fire->GetPosition().Y(), boost::shared_ptr<FireNode>());
				fireit = fireList.erase(fireit);
			} else { ++fireit; }
		} else {
			fireit = fireList.erase(fireit);
		}
	}

	for (std::list<boost::shared_ptr<Spell> >::iterator spellit = spellList.begin(); spellit != spellList.end();) {
		if ((*spellit)->IsDead()) {
			spellit = spellList.erase(spellit);
		} else {
			(*spellit)->UpdateVelocity();
			++spellit;
		}
	}
}

boost::shared_ptr<Job> Game::StockpileItem(boost::weak_ptr<Item> witem, bool returnJob, bool disregardTerritory, bool reserveItem) {
	if (boost::shared_ptr<Item> item = witem.lock()) {
		if ((!reserveItem || !item->Reserved()) && item->GetFaction() == 0) {
			boost::shared_ptr<Stockpile> nearest = boost::shared_ptr<Stockpile>();
			int nearestDistance = INT_MAX;
			for (std::map<int,boost::shared_ptr<Construction> >::iterator stocki = staticConstructionList.begin(); stocki != staticConstructionList.end(); ++stocki) {
				if (stocki->second->stockpile) {
					boost::shared_ptr<Stockpile> sp(boost::static_pointer_cast<Stockpile>(stocki->second));
					if (sp->Allowed(Item::Presets[item->Type()].specificCategories) && !sp->Full(item->Type())) {

						//Found a stockpile that both allows the item, and has space
						int distance = Distance(sp->Center(), item->Position());
						if(distance < nearestDistance) {
							nearestDistance = distance;
							nearest = sp;
						}
					}
				}
			}

			if(nearest) {
				JobPriority priority;
				if (item->IsCategory(Item::StringToItemCategory("Food"))) priority = HIGH;
				else {
					float stockDeficit = (float)StockManager::Inst()->TypeQuantity(item->Type()) / (float)StockManager::Inst()->Minimum(item->Type());
					if (stockDeficit >= 1.0) priority = LOW;
					else if (stockDeficit > 0.25) priority = MED;
					else priority = HIGH;
				}

				boost::shared_ptr<Job> stockJob(new Job("Store " + Item::ItemTypeToString(item->Type()) + " in stockpile", 
					priority));
				stockJob->Attempts(1);
				Coordinate target = Coordinate(-1,-1);
				boost::weak_ptr<Item> container;

				//Check if the item can be contained, and if so if any containers are in the stockpile
				if (Item::Presets[item->Type()].fitsin >= 0) {
					container = nearest->FindItemByCategory(Item::Presets[item->Type()].fitsin, NOTFULL, item->GetBulk());
					if (container.lock()) {
						target = container.lock()->Position();
						stockJob->ReserveSpace(boost::static_pointer_cast<Container>(container.lock()), item->GetBulk());
					}
				}

				if (target.X() == -1) target = nearest->FreePosition();

				if (target.X() != -1) {
					stockJob->ReserveSpot(nearest, target, item->Type());
					if (reserveItem) stockJob->ReserveEntity(item);
					stockJob->tasks.push_back(Task(MOVE, item->Position()));
					stockJob->tasks.push_back(Task(TAKE, item->Position(), item));
					stockJob->tasks.push_back(Task(MOVE, target));
					if (!container.lock())
						stockJob->tasks.push_back(Task(PUTIN, target, nearest->Storage(target)));
					else
						stockJob->tasks.push_back(Task(PUTIN, target, container));

					if (disregardTerritory) stockJob->DisregardTerritory();

					if (!returnJob) JobManager::Inst()->AddJob(stockJob);
					else return stockJob;
				}
			}
		}
	}
	return boost::shared_ptr<Job>();
}

Coordinate Game::TileAt(int pixelX, int pixelY) const {
	return renderer->TileAt(pixelX, pixelY, camX, camY);
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

void Game::Draw(TCODConsole * console, float focusX, float focusY, bool drawUI, int posX, int posY, int sizeX, int sizeY) {
	console->setBackgroundFlag(TCOD_BKGND_SET);
	if (sizeX == -1) {
		sizeX = console->getWidth();
	}
	if (sizeY == -1) {
		sizeY = console->getHeight();
	}
	int charX, charY;
	TCODSystem::getCharSize(&charX, &charY);
	renderer->DrawMap(Map::Inst(), focusX, focusY, posX * charX, posY * charY, sizeX * charX, sizeY * charY);

	if (drawUI) {
		UI::Inst()->Draw(console);
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

//First generates a heightmap, then translates that into the corresponding tiles
//Third places plantlife according to heightmap, and some wildlife as well
void Game::GenerateMap(uint32 seed) {
	Random::Generator random(seed);
	
	Map* map = Map::Inst();
	map->heightMap->clear();

	bool riverStartLeft = random.GenerateBool();
	bool riverEndRight  = random.GenerateBool();

	int px[4];
	int py[4];

	do {
		if (riverStartLeft) {
			px[0] = 0; 
			py[0] = random.Generate(map->Height()-1);
		} else {
			px[0] = random.Generate(map->Width()-1);
			py[0] = 0;
		}

		px[1] = 10 + random.Generate(map->Width()-20);
		py[1] = 10 + random.Generate(map->Height()-20);
		px[2] = 10 + random.Generate(map->Width()-20);
		py[2] = 10 + random.Generate(map->Height()-20);

		if (riverEndRight) {
			px[3] = map->Width()-1;
			py[3] = random.Generate(map->Height()-1);
		} else {
			px[3] = random.Generate(map->Width()-1);
			py[3] = map->Height()-1;
		}
		//This conditional ensures that the river's beginning and end are at least 100 units apart
	} while (std::sqrt( std::pow((double)px[0] - px[3], 2) + std::pow((double)py[0] - py[3], 2)) < 100);

	map->heightMap->digBezier(px, py, 30, -5, 30, -5);

	int hills = 0;
	//infinityCheck is just there to make sure our while loop doesn't become an infinite one
	//in case no suitable hill sites are found
	int infinityCheck = 0;
	while (hills < map->Width()/66 && infinityCheck < 1000) {
		int x = random.Generate(map->Width()  - 1);
		int y = random.Generate(map->Height() - 1);
		int riverDistance;
		int distance;
		int lineX, lineY;

		riverDistance = 70;

		for (int i = 0; i < 4; ++i) {

			//We draw four lines from our potential hill site and measure the least distance to a river
			switch (i) {
			case 0:
				lineX = x - 70;
				lineY = y;
				break;

			case 1:
				lineX = x + 70;
				lineY = y;
				break;

			case 2:
				lineX = x;
				lineY = y - 70;
				break;

			case 3:
				lineX = x;
				lineY = y + 70;
				break;
			}

			distance = 70;
			TCODLine::init(lineX, lineY, x, y);
			do {
				if (lineX >= 0 && lineX < map->Width() && lineY >= 0 && lineY < map->Height()) {
					if (map->heightMap->getValue(lineX, lineY) < map->GetWaterlevel()) {
						if (distance < riverDistance) riverDistance = distance;
					}
				}
				--distance;
			} while (!TCODLine::step(&lineX, &lineY));
		}

		if (riverDistance > 35) {
			map->heightMap->addHill((float)x, (float)y, (float)random.Generate(15,35), (float)random.Generate(1,3));
			map->heightMap->addHill((float)x+random.Generate(-7,7), (float)y+random.Generate(-7,7), (float)random.Generate(15,25), (float)random.Generate(1,3));
			map->heightMap->addHill((float)x+random.Generate(-7,7), (float)y+random.Generate(-7,7), (float)random.Generate(15,25), (float)random.Generate(1,3));
			++hills;
		}

		++infinityCheck;
	}
	
	{
		std::auto_ptr<TCODRandom> tcodRandom = std::auto_ptr<TCODRandom>(new TCODRandom(random.GetSeed()));
		map->heightMap->rainErosion(map->Width()*map->Height()*5, 0.005f, 0.30f, tcodRandom.get());
	}

	//This is a simple kernel transformation that does some horizontal smoothing (lifted straight from the libtcod docs)
	int dx [] = {-1,1,0};
	int dy[] = {0,0,0};
	float weight[] = {0.33f,0.33f,0.33f};
	map->heightMap->kernelTransform(3, dx, dy, weight, 0.0f, 1.0f);


	//Now take the heightmap values and translate them into tiles
	for (int x = 0; x < map->Width(); ++x) {
		for (int y = 0; y < map->Height(); ++y) {
			float height = map->heightMap->getValue(x,y);
			if (height < map->GetWaterlevel()) {
				if (random.GenerateBool()) map->Type(x,y,TILERIVERBED);
				else map->Type(x,y,TILEDITCH);
				CreateWater(Coordinate(x,y), RIVERDEPTH);
			} else if (height < 4.5f) {
				map->Type(x,y,TILEGRASS);
				if (random.Generate(9) < 9) {
					if (height < -0.01f) {
						map->ForeColor(x,y, TCODColor(random.Generate(100,192),127,0));
					} else if (height < 0.0f) {
						map->ForeColor(x,y, TCODColor(random.Generate(20,170),127,0));
					} else if (height > 4.0f) {
						map->ForeColor(x,y, TCODColor(90, random.Generate(120,150), 90));
					}
				}
			} else {
				map->Type(x,y,TILEROCK);
			}
		}
	}

	//Create a bog
	infinityCheck = 0;
	while (infinityCheck < 1000) {
		int x = random.Generate(30, map->Width()  - 30);
		int y = random.Generate(30, map->Height() - 30);
		int riverDistance;
		int distance;
		int lineX, lineY;
		riverDistance = 70;
		for (int i = 0; i < 4; ++i) {
			switch (i) {
			case 0:
				lineX = x - 70;
				lineY = y;
				break;
			case 1:
				lineX = x + 70;
				lineY = y;
				break;
			case 2:
				lineX = x;
				lineY = y - 70;
				break;
			case 3:
				lineX = x;
				lineY = y + 70;
				break;
			}
			distance = 70;
			TCODLine::init(lineX, lineY, x, y);
			do {
				if (lineX >= 0 && lineX < map->Width() && lineY >= 0 && lineY < map->Height()) {
					if (map->heightMap->getValue(lineX, lineY) < map->GetWaterlevel()) {
						if (distance < riverDistance) riverDistance = distance;
					}
				}
				--distance;
			} while (!TCODLine::step(&lineX, &lineY));
		}
		if (riverDistance > 30) {
			int lowOffset = random.Generate(-5, 5);
			int highOffset = random.Generate(-5, 5);
			for (int xOffset = -25; xOffset < 25; ++xOffset) {
				int range = int(std::sqrt((double)(25*25 - xOffset*xOffset)));
				lowOffset = std::min(std::max(random.Generate(-1, 1) + lowOffset, -5), 5);
				highOffset = std::min(std::max(random.Generate(-1, 1) + highOffset, -5), 5);
				for (int yOffset = -range-lowOffset; yOffset < range+highOffset; ++yOffset) {
					map->Type(x+xOffset, y+yOffset, TILEBOG);
				}
			}
			break; //Only generate one bog
		}
		++infinityCheck;
	}

	for (int x = 0; x < map->Width(); ++x) {
		for (int y = 0; y < map->Height(); ++y) {
			map->Naturify(x,y);
		}
	}

	map->RandomizeWind();
	
	map->CalculateFlow(px, py);
}

//This is intentional, otherwise designating where to cut down trees would always show red unless you were over a tree
bool Game::CheckTree(Coordinate, Coordinate) {
	return true;
}

void Game::FellTree(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->GetNatureObject(x,y);
			if (natUid >= 0) {
				boost::shared_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj && natObj->Tree() && !natObj->Marked()) {
					natObj->Mark();
					boost::shared_ptr<Job> fellJob(new Job("Fell tree", MED, 0, true));
					fellJob->Attempts(50);
					fellJob->ConnectToEntity(natObj);
					fellJob->DisregardTerritory();
					fellJob->SetRequiredTool(Item::StringToItemCategory("Axe"));
					fellJob->tasks.push_back(Task(MOVEADJACENT, natObj->Position(), natObj));
					fellJob->tasks.push_back(Task(FELL, natObj->Position(), natObj));
					JobManager::Inst()->AddJob(fellJob);
				}
			}
		}
	}
}

void Game::DesignateTree(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->GetNatureObject(x,y);
			if (natUid >= 0) {
				boost::shared_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj && natObj->Tree() && !natObj->Marked()) {
					//TODO: Implement proper map marker system and change this to use that
					natObj->Mark();
					StockManager::Inst()->UpdateTreeDesignations(natObj, true);
				}
			}
		}
	}
}

void Game::HarvestWildPlant(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int natUid = Map::Inst()->GetNatureObject(x,y);
			if (natUid >= 0) {
				boost::shared_ptr<NatureObject> natObj = Game::Inst()->natureList[natUid];
				if (natObj && natObj->Harvestable() && !natObj->Marked()) {
					natObj->Mark();
					boost::shared_ptr<Job> harvestJob(new Job("Harvest wild plant"));
					harvestJob->ConnectToEntity(natObj);
					harvestJob->DisregardTerritory();
					harvestJob->tasks.push_back(Task(MOVEADJACENT, natObj->Position(), natObj));
					harvestJob->tasks.push_back(Task(HARVESTWILDPLANT, natObj->Position(), natObj));
					if (NatureObject::Presets[natObj->Type()].components.size() > 0)
						harvestJob->tasks.push_back(Task(STOCKPILEITEM));
					JobManager::Inst()->AddJob(harvestJob);
				}
			}
		}
	}
}


void Game::RemoveNatureObject(boost::weak_ptr<NatureObject> natObj) {
	if (natObj.lock()) {
		Map::Inst()->SetNatureObject(natObj.lock()->X(), natObj.lock()->Y(), -1);
		natureList.erase(natObj.lock()->Uid());
	}
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
			int natUid = Map::Inst()->GetNatureObject(x,y);
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
			if (Map::Inst()->GroundMarked(x,y)) { 
				JobManager::Inst()->RemoveJob(DIG, Coordinate(x,y)); //A dig job may exist for this tile
				Camp::Inst()->RemoveWaterZone(Coordinate(x,y), Coordinate(x,y)); //May be marked for water
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

	for (std::list<boost::weak_ptr<BloodNode> >::iterator bli = bloodList.begin(); bli != bloodList.end();) {
		if (boost::shared_ptr<BloodNode> blood = bli->lock()) {
			blood->Depth(blood->Depth()-50);
			if (blood->Depth() <= 0) {
				Map::Inst()->SetBlood(blood->Position().X(), blood->Position().Y(), boost::shared_ptr<BloodNode>());
				bli = bloodList.erase(bli);
			} else ++bli;
		} else {
			bli = bloodList.erase(bli);
		}
	}
}

void Game::CreateFilth(Coordinate pos) {
	CreateFilth(pos, 1);
}

void Game::CreateFilth(Coordinate pos, int amount) {
	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		int loops = -1;
		while (amount > 0 && loops < 1000) {
			++loops;
			boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(pos.X(), pos.Y()).lock();

			if (water) { //If water exists here just add the filth there, no need for filthnodes
				water->AddFilth(amount);
				return;
			}

			boost::weak_ptr<FilthNode> filth(Map::Inst()->GetFilth(pos.X(), pos.Y()));
			if (!filth.lock()) { //No existing filth node so create one
				boost::shared_ptr<FilthNode> newFilth(new FilthNode(pos.X(), pos.Y(), std::min(5, amount)));
				amount -= 5;
				filthList.push_back(boost::weak_ptr<FilthNode>(newFilth));
				Map::Inst()->SetFilth(pos.X(), pos.Y(), newFilth);
			} else {
				int originalDepth = filth.lock()->Depth();
				filth.lock()->Depth(std::min(5, filth.lock()->Depth() + amount));
				amount -= (5 - originalDepth);
			}
			//If theres still remaining filth, it'll spill over according to flow
			if (amount > 0) {
				Coordinate flowTo = pos;
				int diff = std::max(1, loops / 100);
				switch (Map::Inst()->GetFlow(pos.X(), pos.Y())) {
				case NORTH:
					flowTo.Y(flowTo.Y() - diff);
					flowTo.X(flowTo.X() + Random::Generate(-diff, diff));
					break;

				case NORTHEAST:
					if (Random::GenerateBool()) {
						flowTo.Y(flowTo.Y() - diff);
						flowTo.X(flowTo.X() + Random::Generate(0, diff));
					} else {
						flowTo.Y(flowTo.Y() + Random::Generate(-diff, 0));
						flowTo.X(flowTo.X() + diff);
					}
					break;

				case NORTHWEST:
					if (Random::GenerateBool()) {
						flowTo.Y(flowTo.Y() - diff);
						flowTo.X(flowTo.X() - Random::Generate(0, diff));
					} else {
						flowTo.Y(flowTo.Y() + Random::Generate(-diff, 0));
						flowTo.X(flowTo.X() - diff);
					}
					break;

				case SOUTH:
					flowTo.Y(flowTo.Y() + diff);
					flowTo.X(flowTo.X() + Random::Generate(-diff, diff));
					break;

				case SOUTHEAST:
					if (Random::GenerateBool()) {
						flowTo.Y(flowTo.Y() + diff);
						flowTo.X(flowTo.X() + Random::Generate(0, diff));
					} else {
						flowTo.Y(flowTo.Y() + Random::Generate(0, diff));
						flowTo.X(flowTo.X() + diff);
					}
					break;

				case SOUTHWEST:
					if (Random::GenerateBool()) {
						flowTo.Y(flowTo.Y() + diff);
						flowTo.X(flowTo.X() + Random::Generate(0, diff));
					} else {
						flowTo.Y(flowTo.Y() + Random::Generate(0, diff));
						flowTo.X(flowTo.X() + diff);
					}
					break;

				case WEST:
					flowTo.Y(flowTo.Y() + Random::Generate(-diff, diff));
					flowTo.X(flowTo.X() - diff);
					break;

				case EAST:
					flowTo.Y(flowTo.Y() + Random::Generate(-diff, diff));
					flowTo.X(flowTo.X() + diff);
					break;

				default: break;
				}
				while (flowTo == pos || (flowTo.X() < 0 || flowTo.X() >= Map::Inst()->Width() ||
					flowTo.Y() < 0 || flowTo.Y() >= Map::Inst()->Height())) {
						flowTo = Coordinate(pos.X() + Random::Generate(-diff, diff), pos.Y() + Random::Generate(-diff, diff));
				}
				pos = flowTo;
			}
		}
	}
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

void Game::SetSquadTargetCoordinate(Order order, Coordinate target, boost::shared_ptr<Squad> squad, bool autoClose) {
	squad->AddOrder(order);
	squad->AddTargetCoordinate(target);
	if (autoClose) UI::Inst()->CloseMenu();
	Announce::Inst()->AddMsg((boost::format("[%1%] guarding position (%2%,%3%)") % squad->Name() % target.X() % target.Y()).str(), TCODColor::white, target);
	Map::Inst()->AddMarker(MapMarker(FLASHINGMARKER, 'X', target, UPDATES_PER_SECOND*5, TCODColor::azure));
}
void Game::SetSquadTargetEntity(Order order, Coordinate target, boost::shared_ptr<Squad> squad) {
	if (target.X() >= 0 && target.X() < Map::Inst()->Width() && target.Y() >= 0 && target.Y() < Map::Inst()->Height()) {
		std::set<int> *npcList = Map::Inst()->NPCList(target.X(), target.Y());
		if (!npcList->empty()) {
			squad->AddOrder(order);
			squad->AddTargetEntity(Game::Inst()->npcList[*npcList->begin()]);
			UI::Inst()->CloseMenu();
			Announce::Inst()->AddMsg((boost::format("[%1%] following %2%") % squad->Name() % Game::Inst()->npcList[*npcList->begin()]->Name()).str(), TCODColor::white, target);
		}
	}
}

// Spawns NPCs distributed randomly within the rectangle defined by corner1 & corner2
void Game::CreateNPCs(int quantity, NPCType type, Coordinate corner1, Coordinate corner2) {
	int areaWidth = std::max(abs(corner1.X()-corner2.X()), 1);
	int areaLength = std::max(abs(corner1.Y()-corner2.Y()), 1);
	int minX = std::min(corner1.X(), corner2.X());
	int minY = std::min(corner1.Y(), corner2.Y());

	for (int npcs = 0; npcs < quantity; ++npcs) {
		Coordinate location(
			Random::Generate(minX, areaWidth + minX - 1),
			Random::Generate(minY, areaLength + minY - 1)
		);

		Game::Inst()->CreateNPC(location, type);
	}
}

int Game::DiceToInt(TCOD_dice_t dice) {
	return Random::Dice(dice).Roll();
}

void Game::ToMainMenu(bool value) { Game::Inst()->toMainMenu = value; }
bool Game::ToMainMenu() { return Game::Inst()->toMainMenu; }

void Game::Running(bool value) { running = value; }
bool Game::Running() { return running; }

boost::weak_ptr<Construction> Game::FindConstructionByTag(ConstructionTag tag, Coordinate closeTo) {
	
	int distance = -1;
	boost::weak_ptr<Construction> foundConstruct;

	for (std::map<int, boost::shared_ptr<Construction> >::iterator stati = staticConstructionList.begin();
		stati != staticConstructionList.end(); ++stati) {
			if (!stati->second->Reserved() && stati->second->HasTag(tag)) {
				if (closeTo.X() == -1)
					return stati->second;
				else {
					if (distance == -1 || Distance(closeTo, stati->second->Position()) < distance) {
						distance = Distance(closeTo, stati->second->Position());
						foundConstruct = stati->second;
						if (distance < 5) return foundConstruct;
					}
				}
			}
	}

	if (foundConstruct.lock()) return foundConstruct;

	for (std::map<int, boost::shared_ptr<Construction> >::iterator dynai = dynamicConstructionList.begin();
		dynai != dynamicConstructionList.end(); ++dynai) {
			if (!dynai->second->Reserved() && dynai->second->HasTag(tag)) {
				if (closeTo.X() == -1)
					return dynai->second;
				else {
					if (distance == -1 || Distance(closeTo, dynai->second->Position()) < distance) {
						distance = Distance(closeTo, dynai->second->Position());
						foundConstruct = dynai->second;
						if (distance < 5) return foundConstruct;
					}
				}
			}
	}

	return foundConstruct;
}

void Game::Reset() {
	for (int x = 0; x < Map::Inst()->Width(); ++x) {
		for (int y = 0; y < Map::Inst()->Height(); ++y) {
			Map::Inst()->Reset(x,y);
		}
	}
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
	orcCount = 0;
	goblinCount = 0;
	paused = false;
	toMainMenu = false;
	running = false;
	events = boost::shared_ptr<Events>(new Events(Map::Inst()));
	season = LateWinter;
	camX = 180;
	camY = 180;
	safeMonths = 9;
	Announce::Inst()->Reset();
	Camp::Inst()->Reset();
	renderer->PreparePrefabs();
	fireList.clear();
	spellList.clear();
}

NPCType Game::GetRandomNPCTypeByTag(std::string tag) {
	std::vector<NPCType> npcList;
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		if (NPC::Presets[i].tags.find(boost::to_lower_copy(tag)) != NPC::Presets[i].tags.end()) {
			npcList.push_back(i);
		}
	}
	if (npcList.size() > 0)
		return Random::ChooseElement(npcList);
	return -1;
}

void Game::CenterOn(Coordinate target) {
	camX = target.X() + 0.5f;
	camY = target.Y() + 0.5f;
}

void Game::MoveCam(float x, float y) {
	camX = std::min(std::max(x * renderer->ScrollRate() + camX, 0.0f), Map::Inst()->Width() + 1.0f);
	camY = std::min(std::max(y * renderer->ScrollRate() + camY, 0.0f), Map::Inst()->Height() + 1.0f);
}

void Game::SetMark(int i) {
	marks[i] = Coordinate(FloorToInt::convert(camX), FloorToInt::convert(camY));
}

void Game::ReturnToMark(int i) {
	camX = marks[i].X() + 0.5f;
	camY = marks[i].Y() + 0.5f;
}

void Game::TranslateContainerListeners() {
	for (std::map<int,boost::shared_ptr<Item> >::iterator it = itemList.begin(); it != itemList.end(); ++it) {
		if (boost::dynamic_pointer_cast<Container>(it->second)) {
			boost::static_pointer_cast<Container>(it->second)->TranslateContainerListeners();
		}
	}
	for (std::map<int, boost::shared_ptr<Construction> >::iterator it = staticConstructionList.begin(); 
		it != staticConstructionList.end(); ++it) {
			if (boost::dynamic_pointer_cast<Stockpile>(it->second)) {
				boost::static_pointer_cast<Stockpile>(it->second)->TranslateInternalContainerListeners();
			}
	}
	for (std::map<int, boost::shared_ptr<Construction> >::iterator it = dynamicConstructionList.begin(); 
		it != dynamicConstructionList.end(); ++it) {
			if (boost::dynamic_pointer_cast<Stockpile>(it->second)) {
				boost::static_pointer_cast<Stockpile>(it->second)->TranslateInternalContainerListeners();
			}
	}
}

unsigned int Game::PeacefulFaunaCount() const { return peacefulFaunaCount; }
void Game::PeacefulFaunaCount(int add) { peacefulFaunaCount += add; }

bool Game::DevMode() { return devMode; }
void Game::EnableDevMode() { devMode = true; }

void Game::Dig(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			/*TODO: Relying on GroundMarked() is iffy, it doesn't necessarily mean that that
			spot is reserved for digging. */
			if (CheckPlacement(Coordinate(x,y), Coordinate(1,1)) && !Map::Inst()->GroundMarked(x,y) && !Map::Inst()->IsLow(x,y)) {
				boost::shared_ptr<Job> digJob(new Job("Dig"));
				digJob->SetRequiredTool(Item::StringToItemCategory("Shovel"));
				digJob->MarkGround(Coordinate(x,y));
				digJob->Attempts(1000);
				digJob->DisregardTerritory();
				digJob->tasks.push_back(Task(MOVEADJACENT, Coordinate(x,y)));
				digJob->tasks.push_back(Task(DIG, Coordinate(x,y)));
				JobManager::Inst()->AddJob(digJob);
			}
		}
	}
}

Coordinate Game::FindClosestAdjacent(Coordinate from, Coordinate target) {
	Coordinate closest = Coordinate(-9999, -9999);
	for (int ix = target.X()-1; ix <= target.X()+1; ++ix) {
		for (int iy = target.Y()-1; iy <= target.Y()+1; ++iy) {
			if (ix == target.X()-1 || ix == target.X()+1 ||
				iy == target.Y()-1 || iy == target.Y()+1) {
					if (Map::Inst()->IsWalkable(ix,iy)) {
						if (Distance(from.X(), from.Y(), ix, iy) < Distance(from.X(), from.Y(), closest.X(), closest.Y()))
							closest = Coordinate(ix,iy);
					}
			}
		}
	}
	return closest;
}

bool Game::Adjacent(Coordinate a, Coordinate b) {
	if (std::abs(a.X() - b.X()) < 2 && std::abs(a.Y() - b.Y()) < 2) return true;
	return false;
}

void Game::CreateNatureObject(Coordinate location) {
	if (Map::Inst()->IsWalkable(location.X(),location.Y()) && Map::Inst()->Type(location.X(),location.Y()) == TILEGRASS && Random::Generate(4) < 2) {
		std::priority_queue<std::pair<int, int> > natureObjectQueue;
		float height = Map::Inst()->heightMap->getValue(location.X(),location.Y());

		//Populate the priority queue with all possible plants and give each one a random
		//value based on their rarity
		bool evil = Map::Inst()->GetCorruption(location.X(), location.Y()) >= 100;
		for (unsigned int i = 0; i < NatureObject::Presets.size(); ++i) {
			if (NatureObject::Presets[i].minHeight <= height &&
				NatureObject::Presets[i].maxHeight >= height &&
				NatureObject::Presets[i].evil == evil)
				natureObjectQueue.push(std::make_pair(Random::Generate(NatureObject::Presets[i].rarity - 1) + Random::Generate(2), i));
		}

		if (natureObjectQueue.empty()) return;
		int chosen = natureObjectQueue.top().second;
		int rarity = NatureObject::Presets[chosen].rarity;
		if (std::abs(height - NatureObject::Presets[chosen].minHeight) <= 0.01f ||
			std::abs(height - NatureObject::Presets[chosen].maxHeight) <= 0.5f) rarity = rarity - rarity / 5;
		if (std::abs(height - NatureObject::Presets[chosen].minHeight) <= 0.005f ||
			std::abs(height - NatureObject::Presets[chosen].maxHeight) <= 0.05f) rarity /= 2;

		if (Random::Generate(99) < rarity) {

			for (int clus = 0; clus < NatureObject::Presets[chosen].cluster; ++clus) {
				int ax = location.X() + Random::Generate(NatureObject::Presets[chosen].cluster - 1) - (NatureObject::Presets[chosen].cluster/2);
				int ay = location.Y() + Random::Generate(NatureObject::Presets[chosen].cluster - 1) - (NatureObject::Presets[chosen].cluster/2);
				if (ax < 0) ax = 0; if (ax >= Map::Inst()->Width()) ax = Map::Inst()->Width()-1;
				if (ay < 0) ay = 0; if (ay >= Map::Inst()->Height()) ay = Map::Inst()->Height()-1;
				if (Map::Inst()->IsWalkable(ax,ay) && Map::Inst()->Type(ax,ay) == TILEGRASS &&
					Map::Inst()->GetNatureObject(ax,ay) < 0 &&
					Map::Inst()->GetConstruction(ax, ay) < 0) {
						boost::shared_ptr<NatureObject> natObj(new NatureObject(Coordinate(ax,ay), chosen));
						natureList.insert(std::pair<int, boost::shared_ptr<NatureObject> >(natObj->Uid(), natObj));
						Map::Inst()->SetNatureObject(ax,ay,natObj->Uid());
						Map::Inst()->SetWalkable(ax,ay,NatureObject::Presets[natObj->Type()].walkable);
						Map::Inst()->SetBuildable(ax,ay,false);
						Map::Inst()->SetBlocksLight(ax,ay,!NatureObject::Presets[natObj->Type()].walkable);
				}
			}
		}
	}
}

void Game::CreateNatureObject(Coordinate location, std::string name) {
	unsigned int natureObjectIndex = 0;
	for (std::vector<NatureObjectPreset>::iterator preseti = NatureObject::Presets.begin(); preseti != NatureObject::Presets.end();
		++preseti) {
			if (boost::iequals(preseti->name, name)) break;
			++natureObjectIndex;
	}

	if (natureObjectIndex < NatureObject::Presets.size() && 
		boost::iequals(NatureObject::Presets[natureObjectIndex].name, name)) {
		if (location.X() >= 0 && location.X() < Map::Inst()->Width() && 
			location.Y() >= 0 && location.Y() < Map::Inst()->Height() &&
			Map::Inst()->GetNatureObject(location.X(),location.Y()) < 0 &&
			Map::Inst()->GetConstruction(location.X(), location.Y()) < 0) {
				boost::shared_ptr<NatureObject> natObj(new NatureObject(Coordinate(location.X(),location.Y()), natureObjectIndex));
				natureList.insert(std::pair<int, boost::shared_ptr<NatureObject> >(natObj->Uid(), natObj));
				Map::Inst()->SetNatureObject(location.X(),location.Y(),natObj->Uid());
				Map::Inst()->SetWalkable(location.X(),location.Y(),NatureObject::Presets[natObj->Type()].walkable);
				Map::Inst()->SetBuildable(location.X(),location.Y(),false);
				Map::Inst()->SetBlocksLight(location.X(),location.Y(),!NatureObject::Presets[natObj->Type()].walkable);
		}

	}
}

void Game::RemoveNatureObject(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			int uid = Map::Inst()->GetNatureObject(x,y);
			if (uid >= 0) {
				Map::Inst()->SetNatureObject(x,y,-1);
				natureList.erase(uid);
			}
		}
	}
}

void Game::TriggerAttack() { events->SpawnHostileMonsters(); }

void Game::GatherItems(Coordinate a, Coordinate b) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (x >= 0 && x < Map::Inst()->Width() && y >= 0 && y < Map::Inst()->Height()) {
				for (std::set<int>::iterator itemuid = Map::Inst()->ItemList(x,y)->begin(); 
					itemuid != Map::Inst()->ItemList(x,y)->end(); ++itemuid) {
						StockpileItem(GetItem(*itemuid), false, true);
				}
			}
		}
	}
}

void Game::RemoveFilth(Coordinate pos) {
	boost::shared_ptr<FilthNode> filth = Map::Inst()->GetFilth(pos.X(), pos.Y()).lock();
	if (filth) {
		for (std::list<boost::weak_ptr<FilthNode> >::iterator filthi = filthList.begin(); filthi != filthList.end(); ++filthi) {
			if (filthi->lock() == filth) {
				filthList.erase(filthi);
				break;
			}
		}
		Map::Inst()->SetFilth(pos.X(), pos.Y(), boost::shared_ptr<FilthNode>());
	}
}

void Game::RemoveWater(Coordinate pos) {
	boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(pos.X(), pos.Y()).lock();
	if (water) {
		for (std::list<boost::weak_ptr<WaterNode> >::iterator wateri = waterList.begin(); wateri != waterList.end(); ++wateri) {
			if (wateri->lock() == water) {
				waterList.erase(wateri);
				break;
			}
		}
		int filth = water->GetFilth();
		Map::Inst()->SetWater(pos.X(), pos.Y(), boost::shared_ptr<WaterNode>());
		if (filth > 0) CreateFilth(pos, filth);
	}
}

void Game::Damage(Coordinate pos) {
	Attack attack;
	attack.Type(DAMAGE_MAGIC);
	TCOD_dice_t dice;
	dice.nb_dices = 10;
	dice.nb_faces = 10;
	dice.addsub = 1000;
	attack.AddDamage(dice);
	
	boost::shared_ptr<Construction> construction = GetConstruction(Map::Inst()->GetConstruction(pos.X(), pos.Y())).lock();
	if (construction) {
		construction->Damage(&attack);
	}
	for (std::set<int>::iterator npcuid = Map::Inst()->NPCList(pos.X(), pos.Y())->begin(); 
		npcuid != Map::Inst()->NPCList(pos.X(), pos.Y())->end(); ++npcuid) {
			boost::shared_ptr<NPC> npc;
			if (npcList.find(*npcuid) != npcList.end()) npc = npcList[*npcuid];
			if (npc) npc->Damage(&attack);
	}
}

void Game::Hungerize(Coordinate pos) {
	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		for (std::set<int>::iterator npci = Map::Inst()->NPCList(pos.X(), pos.Y())->begin();
			npci != Map::Inst()->NPCList(pos.X(), pos.Y())->end(); ++npci) {
				boost::shared_ptr<NPC> npc;
				if (npcList.find(*npci) != npcList.end()) npc = npcList[*npci];
				if (npc) {
					npc->hunger = 50000;
				}
		}
	}
}

void Game::Tire(Coordinate pos) {
	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		for (std::set<int>::iterator npci = Map::Inst()->NPCList(pos.X(), pos.Y())->begin();
			npci != Map::Inst()->NPCList(pos.X(), pos.Y())->end(); ++npci) {
				boost::shared_ptr<NPC> npc;
				if (npcList.find(*npci) != npcList.end()) npc = npcList[*npci];
				if (npc) {
					npc->weariness = (int)(WEARY_THRESHOLD * 1.5);
				}
		}
	}
}

void Game::AddDelay(int delay, boost::function<void()> callback) {
	delays.push_back(std::pair<int, boost::function<void()> >(delay, callback));
}

void Game::GameOver() {
	MessageBox::ShowMessageBox("All your orcs and goblins have died.");
	running = false;
}

void Game::CreateFire(Coordinate pos) {
	CreateFire(pos, 10);
}

void Game::CreateFire(Coordinate pos, int temperature) {
	boost::weak_ptr<FireNode> fire(Map::Inst()->GetFire(pos.X(), pos.Y()));
	if (!fire.lock()) { //No existing firenode
		boost::shared_ptr<FireNode> newFire(new FireNode(pos.X(), pos.Y(), temperature));
		fireList.push_back(boost::weak_ptr<FireNode>(newFire));
		Map::Inst()->SetFire(pos.X(), pos.Y(), newFire);
	} else {
	}
}

boost::shared_ptr<Spell> Game::CreateSpell(Coordinate pos, int type) {
	boost::shared_ptr<Spell> newSpell(new Spell(pos, type));
	spellList.push_back(newSpell);
	return newSpell;
}

void Game::CreateDitch(Coordinate pos) {
	RemoveNatureObject(pos, pos);
	Map::Inst()->SetLow(pos.X(), pos.Y(), true);
	Map::Inst()->Type(pos.X(), pos.Y(), TILEDITCH);
}

void Game::StartFire(Coordinate pos) {
	boost::shared_ptr<Job> fireJob(new Job("Start a fire", HIGH, 0, false));
	fireJob->Attempts(2);
	fireJob->DisregardTerritory();
	fireJob->tasks.push_back(Task(MOVEADJACENT, pos));
	fireJob->tasks.push_back(Task(STARTFIRE, pos));
	fireJob->AddMapMarker(MapMarker(FLASHINGMARKER, 'F', pos, -1, TCODColor::red));
	JobManager::Inst()->AddJob(fireJob);
}

int Game::GetAge() { return age; }