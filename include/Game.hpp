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
#include<memory>

#include <list>


#include <libtcod.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"
#include "NPC.hpp"
#include "NatureObject.hpp"
#include "Events.hpp"
#include "Job.hpp"
#include "Fire.hpp"
#include "Spell.hpp"
#include "GCamp.hpp"

#include "MapRenderer.hpp"
#include "data/Serialization.hpp"

#define BFS_MAX_DISTANCE 20

#define MONTH_LENGTH (UPDATES_PER_SECOND * 60 * 4)

class Faction;
class MapRenderer;

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
	GC_SERIALIZABLE_CLASS
	
	friend class ConfigListener;
	friend void SettingsMenu();
	friend class TCODMapRenderer;
	friend class TilesetRenderer;
	friend class NPCDialog;
	friend void StartNewGame();

	Game();
	static Game* instance;
	int screenWidth, screenHeight;
	Season season;
	int time;
	int age;
	int orcCount, goblinCount;
	unsigned int peacefulFaunaCount;
	bool paused;
	int charWidth, charHeight;
	bool toMainMenu, running;
	int safeMonths;
	bool refreshStockpiles;
	static bool devMode;
	Coordinate marks[12];

	std::shared_ptr<Events> events;

	std::list<std::pair<int, std::function<void()> > > delays;

	std::shared_ptr<MapRenderer> renderer;
	bool gameOver;

	std::map<int, std::shared_ptr<Construction> > staticConstructionList;
	std::map<int, std::shared_ptr<Construction> > dynamicConstructionList;
	std::map<int, std::shared_ptr<NPC> > npcList;

	static bool initializedOnce;

public:
	static Game* Inst();
	~Game();
	static bool LoadGame(const std::string&);
	static bool SaveGame(const std::string&);
	static void ToMainMenu(bool);
	static void GoToMainMenu(bool);
	static bool ToMainMenu();
	void Running(bool);
	bool Running();
	static void Reset();
	static void DoNothing();
	static void Exit(bool confirm=true);

	Coordinate TileAt(int pixelX, int pixelY) const;
	std::shared_ptr<MapRenderer> Renderer() { return renderer; };

	int ScreenWidth() const;
	int ScreenHeight() const;
	void LoadConfig(std::string);
	void Init(bool firstTime);
	void ResetRenderer();
	
	static std::mutex loadingScreenMutex;
	static void ProgressScreen(std::function<void(void)>, bool isLoading);
	static void LoadingScreen(std::function<void(void)> fn) {
		ProgressScreen(fn, true);
	}
	static void SavingScreen(std::function<void(void)> fn) {
		ProgressScreen(fn, false);
	}
	
	static void ErrorScreen();
	void GenerateMap(uint32_t seed = 0);

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
	void Draw(
		TCODConsole * console = Game::Inst()->buffer, float focusX = Game::Inst()->camX, float focusY = Game::Inst()->camY,
		bool drawUI = true, int posX = 0, int posY = 0, int xSize = -1, int ySize = -1
	);

	static int DiceToInt(TCOD_dice_t);

	static void Undesignate(Coordinate, Coordinate);
	bool DevMode();
	void EnableDevMode();

	/*      NPCS        NPCS        NPCS        */
	int CreateNPC(Coordinate, NPCType);
	void BumpEntity(int);
	int DistanceNPCToCoordinate(int, Coordinate);
	int OrcCount() const; int GoblinCount() const;
	void OrcCount(int); void GoblinCount(int);
	void RemoveNPC(std::weak_ptr<NPC>);
	int FindMilitaryRecruit();
	std::map<std::string, std::shared_ptr<Squad> > squadList;
	std::list<std::shared_ptr<Squad> > hostileSquadList;
	void CreateSquad(std::string);
	static void SetSquadTargetCoordinate(Order, Coordinate, std::shared_ptr<Squad>, bool autoClose = true);
	static void SetSquadTargetEntity(Order, Coordinate, std::shared_ptr<Squad>);
	NPCType GetRandomNPCTypeByTag(std::string tag);
	std::vector<int> CreateNPCs(int,NPCType,Coordinate,Coordinate);
	unsigned int PeacefulFaunaCount() const;
	void PeacefulFaunaCount(int);
	void Hungerize(Coordinate);
	void Thirstify(Coordinate);
	void Tire(Coordinate);
	void Badsleepify(Coordinate);
	void Diseasify(Coordinate);
	std::shared_ptr<NPC> GetNPC(int) const;

	/*      CONSTRUCTIONS       CONSTRUCTIONS       CONSTRUCTIONS       */
	static bool CheckPlacement(Coordinate, Coordinate, std::set<TileType> = std::set<TileType>());
	static int PlaceConstruction(Coordinate, ConstructionType);
	static void DismantleConstruction(Coordinate, Coordinate);
	void RemoveConstruction(std::weak_ptr<Construction>);
	static int PlaceStockpile(Coordinate, Coordinate, ConstructionType, int);
	void RefreshStockpiles() { refreshStockpiles = true; }
	void RebalanceStockpiles(ItemCategory requiredCategory, std::shared_ptr<Stockpile> excluded);
	Coordinate FindClosestAdjacent(Coordinate, std::weak_ptr<Entity>, int faction = -1);
	static bool Adjacent(Coordinate, std::weak_ptr<Entity>);
	std::weak_ptr<Construction> GetConstruction(int);
	std::weak_ptr<Construction> FindConstructionByTag(ConstructionTag, Coordinate closeTo=Coordinate(-1,-1));
	std::weak_ptr<Construction> GetRandomConstruction() const;
	void Damage(Coordinate);
	void UpdateFarmPlotSeedAllowances(ItemType);

	/*      ITEMS       ITEMS       ITEMS       */
	int CreateItem(Coordinate, ItemType, bool stockpile = false,
		int ownerFaction = 0,
		std::vector<std::weak_ptr<Item> > = std::vector<std::weak_ptr<Item> >(),
		std::shared_ptr<Container> = std::shared_ptr<Container>());
	void RemoveItem(std::weak_ptr<Item>);
	std::weak_ptr<Item> GetItem(int);
	std::map<int,std::shared_ptr<Item> > itemList;
	void ItemContained(std::weak_ptr<Item>, bool contained);
	std::set<std::weak_ptr<Item> > freeItems; //Free as in not contained
	std::set<std::weak_ptr<Item> > flyingItems; //These need to be updated
	std::list<std::weak_ptr<Item> > stoppedItems; //These need to be removed from flyingItems
	static int ItemTypeCount;
	static int ItemCatCount;
	std::shared_ptr<Job> StockpileItem(std::weak_ptr<Item>, bool returnJob = false, bool disregardTerritory = false, bool reserveItem = true);
	std::weak_ptr<Item> FindItemByCategoryFromStockpiles(ItemCategory, Coordinate, int flags = 0, int value = 0);
	std::weak_ptr<Item> FindItemByTypeFromStockpiles(ItemType, Coordinate, int flags = 0, int value = 0);
	void CreateItems(int,ItemType,Coordinate,Coordinate);
	void TranslateContainerListeners();
	void GatherItems(Coordinate a, Coordinate b);

	/*      NATURE      NATURE      NATURE      */
	std::map<int, std::shared_ptr<NatureObject> > natureList;
	std::list<std::weak_ptr<WaterNode> > waterList;
	void CreateWater(Coordinate);
	void CreateWater1(Coordinate);
	void CreateWater(Coordinate,int,int=0);
	void CreateWaterFromNode(std::shared_ptr<WaterNode>);
	void RemoveWater(Coordinate, bool removeFromList = true);
	Coordinate FindWater(Coordinate);
	Coordinate FindFilth(Coordinate);
	static bool CheckTree(Coordinate, Coordinate);
	static void FellTree(Coordinate, Coordinate);
	static void DesignateTree(Coordinate, Coordinate);
	void RemoveNatureObject(std::weak_ptr<NatureObject>);
	void RemoveNatureObject(Coordinate a, Coordinate b);
	void RemoveNatureObject2(Coordinate a, Coordinate b) { RemoveNatureObject(a, b); }
	static void HarvestWildPlant(Coordinate, Coordinate);
	static void DesignateBog(Coordinate, Coordinate);
	static bool CheckTileType(TileType, Coordinate, Coordinate);
	static void Dig(Coordinate, Coordinate);
	static void FillDitch(Coordinate, Coordinate);
	Coordinate FindClosestAdjacent(Coordinate, Coordinate, int faction = -1);
	static bool Adjacent(Coordinate, Coordinate);
	void CreateNatureObject(Coordinate, int surroundingNatureObjects);
	void CreateNatureObject(Coordinate, std::string);
	void CreateDitch(Coordinate);

	Season CurrentSeason();
	std::string SeasonToString(Season);
	void SetSeason(Season);
	void SpawnTillageJobs();
	void DeTillFarmPlots();
	void DecayItems();

	std::list<std::weak_ptr<FilthNode> > filthList;
	void CreateFilth(Coordinate);
	void CreateFilth(Coordinate,int);
	void CreateFilth2(Coordinate,int);
	void RemoveFilth(Coordinate);

	std::list<std::weak_ptr<BloodNode> > bloodList;
	void CreateBlood(Coordinate);
	void CreateBlood(Coordinate,int);

	void TriggerAttack();
	void TriggerMigration();

	void AddDelay(int delay, std::function<void()>);

	std::list<std::weak_ptr<FireNode> > fireList;
	void CreateFire(Coordinate);
	void CreateFire1(Coordinate c) { CreateFire(c); }
	void CreateFire(Coordinate,int);
	void StartFire(Coordinate);

	std::shared_ptr<Spell> CreateSpell(Coordinate, int type);
	std::list<std::shared_ptr<Spell> > spellList;

	int GetAge();

	void DisplayStats();
	void ProvideMap();
};

BOOST_CLASS_VERSION(Game, 1)
