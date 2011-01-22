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

#pragma once

#include <list>

#include <boost/serialization/split_member.hpp>
#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <libtcod.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"
#include "NPC.hpp"
#include "NatureObject.hpp"
#include "Events.hpp"
#include "Job.hpp"
#include "Fire.hpp"
#include "Spell.hpp"

#include "MapRenderer.hpp"

#define BFS_MAX_DISTANCE 20

#define MONTH_LENGTH (UPDATES_PER_SECOND * 60 * 2)

enum Season {
	EarlySpring,
	Spring,
	LateSpring,
	EarlySummer,
	Summer,
	LateSummer,
	EarlyFall,
	Fall,
	LateFall,
	EarlyWinter,
	Winter,
	LateWinter
};

class Game {
	friend class boost::serialization::access;
	friend class ConfigListener;
	friend void SettingsMenu();
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	Game();
	static Game* instance;
	int screenWidth, screenHeight;
	Season season;
	int time;
	int orcCount, goblinCount;
	unsigned int peacefulFaunaCount;
	bool paused;
	int charWidth, charHeight;
	bool toMainMenu, running;
	int safeMonths;
	bool refreshStockpiles;
	bool devMode;
	Coordinate marks[12];

	boost::shared_ptr<Events> events;

	std::list<std::pair<int, boost::function<void()> > > delays;

	boost::shared_ptr<MapRenderer> renderer;
public:
	static Game* Inst();
	static bool LoadGame(const std::string&);
	static bool SaveGame(const std::string&);
	static void ToMainMenu(bool);
	static bool ToMainMenu();
	void Running(bool);
	bool Running();
	void Reset();
	static void DoNothing();
	static void Exit(bool confirm=true);

	Coordinate TileAt(int pixelX, int pixelY) const;
	boost::shared_ptr<MapRenderer> Renderer() { return renderer; };

	int ScreenWidth() const;
	int ScreenHeight() const;
	void LoadConfig(std::string);
	void Init();
	void LoadingScreen();
	void ErrorScreen();
	void GenerateMap(uint32 seed = 0);

	void Update();
	float camX, camY;
	void CenterOn(Coordinate target);
	void MoveCam(float x, float y);
	void SetMark(int);
	void ReturnToMark(int);

	void Pause();
	bool Paused();

	void GameOver();

	int CharHeight() const;
	int CharWidth() const;

	TCODConsole* buffer;
	void FlipBuffer();
	void Draw(TCODConsole * console = Game::Inst()->buffer, float focusX = Game::Inst()->camX, float focusY = Game::Inst()->camY, bool drawUI = true, int posX = 0, int posY = 0, int xSize = -1, int ySize = -1);

	static int DiceToInt(TCOD_dice_t);

	static void Undesignate(Coordinate, Coordinate);
	bool DevMode();
	void EnableDevMode();

	/*      NPCS        NPCS        NPCS        */
	std::map<int,boost::shared_ptr<NPC> > npcList;
	int CreateNPC(Coordinate, NPCType);
	void BumpEntity(int);
	int DistanceNPCToCoordinate(int, Coordinate);
	int OrcCount() const; int GoblinCount() const;
	void OrcCount(int); void GoblinCount(int);
	void FindNearbyNPCs(boost::shared_ptr<NPC>, bool onlyHostiles = false);
	void RemoveNPC(boost::weak_ptr<NPC>);
	int FindMilitaryRecruit();
	std::map<std::string, boost::shared_ptr<Squad> > squadList;
	std::list<boost::shared_ptr<Squad> > hostileSquadList;
	void CreateSquad(std::string);
	static void SetSquadTargetCoordinate(Order, Coordinate, boost::shared_ptr<Squad>, bool autoClose = true);
	static void SetSquadTargetEntity(Order, Coordinate, boost::shared_ptr<Squad>);
	NPCType GetRandomNPCTypeByTag(std::string tag);
	void CreateNPCs(int,NPCType,Coordinate,Coordinate);
	unsigned int PeacefulFaunaCount() const;
	void PeacefulFaunaCount(int);
	void Hungerize(Coordinate);
	void Tire(Coordinate);

	/*      CONSTRUCTIONS       CONSTRUCTIONS       CONSTRUCTIONS       */
	static bool CheckPlacement(Coordinate, Coordinate, std::set<TileType> = std::set<TileType>());
	static int PlaceConstruction(Coordinate, ConstructionType);
	static void DismantleConstruction(Coordinate, Coordinate);
	void RemoveConstruction(boost::weak_ptr<Construction>);
	static int PlaceStockpile(Coordinate, Coordinate, ConstructionType, int);
	void RefreshStockpiles() { refreshStockpiles = true; }
	std::map<int, boost::shared_ptr<Construction> > staticConstructionList;
	std::map<int, boost::shared_ptr<Construction> > dynamicConstructionList;
	Coordinate FindClosestAdjacent(Coordinate, boost::weak_ptr<Entity>);
	static bool Adjacent(Coordinate, boost::weak_ptr<Entity>);
	boost::weak_ptr<Construction> GetConstruction(int);
	boost::weak_ptr<Construction> FindConstructionByTag(ConstructionTag);
	void Damage(Coordinate);

	/*      ITEMS       ITEMS       ITEMS       */
	int CreateItem(Coordinate, ItemType, bool stockpile = false,
		int ownerFaction = 0,
		std::vector<boost::weak_ptr<Item> > = std::vector<boost::weak_ptr<Item> >(),
		boost::shared_ptr<Container> = boost::shared_ptr<Container>());
	void RemoveItem(boost::weak_ptr<Item>);
	boost::weak_ptr<Item> GetItem(int);
	std::map<int,boost::shared_ptr<Item> > itemList;
	void ItemContained(boost::weak_ptr<Item>, bool contained);
	std::set<boost::weak_ptr<Item> > freeItems; //Free as in not contained
	std::set<boost::weak_ptr<Item> > flyingItems; //These need to be updated
	std::list<boost::weak_ptr<Item> > stoppedItems; //These need to be removed from flyingItems
	static int ItemTypeCount;
	static int ItemCatCount;
	boost::shared_ptr<Job> StockpileItem(boost::weak_ptr<Item>, bool returnJob = false, bool disregardTerritory = false, bool reserveItem = true);
	boost::weak_ptr<Item> FindItemByCategoryFromStockpiles(ItemCategory, Coordinate, int flags = 0, int value = 0);
	boost::weak_ptr<Item> FindItemByTypeFromStockpiles(ItemType, Coordinate, int flags = 0, int value = 0);
	void CreateItems(int,ItemType,Coordinate,Coordinate);
	void TranslateContainerListeners();
	void GatherItems(Coordinate a, Coordinate b);

	/*      NATURE      NATURE      NATURE      */
	std::map<int, boost::shared_ptr<NatureObject> > natureList;
	std::list<boost::weak_ptr<WaterNode> > waterList;
	void CreateWater(Coordinate);
	void CreateWater(Coordinate,int,int=0);
	void RemoveWater(Coordinate);
	Coordinate FindWater(Coordinate);
	Coordinate FindFilth(Coordinate);
	static bool CheckTree(Coordinate, Coordinate);
	static void FellTree(Coordinate, Coordinate);
	static void DesignateTree(Coordinate, Coordinate);
	void RemoveNatureObject(boost::weak_ptr<NatureObject>);
	void RemoveNatureObject(Coordinate, Coordinate);
	static void HarvestWildPlant(Coordinate, Coordinate);
	static void DesignateBog(Coordinate, Coordinate);
	static bool CheckTileType(TileType, Coordinate, Coordinate);
	static void Dig(Coordinate, Coordinate);
	Coordinate FindClosestAdjacent(Coordinate, Coordinate);
	static bool Adjacent(Coordinate, Coordinate);
	void CreateNatureObject(Coordinate);
	void CreateNatureObject(Coordinate, std::string);

	Season CurrentSeason();
	std::string SeasonToString(Season);
	void SpawnTillageJobs();
	void DeTillFarmPlots();
	void DecayItems();

	std::list<boost::weak_ptr<FilthNode> > filthList;
	void CreateFilth(Coordinate);
	void CreateFilth(Coordinate,int);
	void RemoveFilth(Coordinate);

	std::list<boost::weak_ptr<BloodNode> > bloodList;
	void CreateBlood(Coordinate);
	void CreateBlood(Coordinate,int);

	void TriggerAttack();

	void AddDelay(int delay, boost::function<void()>);

	std::list<boost::weak_ptr<FireNode> > fireList;
	void CreateFire(Coordinate);
	void CreateFire(Coordinate,int);

	void CreateSpell(Coordinate, int type);
	std::list<boost::shared_ptr<Spell> > spellList;
};
