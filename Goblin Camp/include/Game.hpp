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
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	Game();
	static Game* instance;
	int screenWidth, screenHeight;
	int resolutionWidth, resolutionHeight;
	bool fullscreen;
	TCOD_renderer_t renderer;
	Season season;
	int time;
	int orcCount, goblinCount;
	bool paused;
	int charWidth, charHeight;
	bool toMainMenu, running;
	int safeMonths;

	boost::shared_ptr<Events> events;
public:
	static Game* Inst();
	static void LoadGame(std::string);
	static void SaveGame(std::string);
	static void ToMainMenu(bool);
	static bool ToMainMenu();
	void Running(bool);
	bool Running();
	void Reset();
	static void DoNothing();
	static void Exit(bool confirm=true);
	static void ExitConfirmed();

	int ScreenWidth() const;
	int ScreenHeight() const;
	void LoadConfig(std::string);
	void Init();
	void GenerateMap();

	void Update();
	Coordinate upleft;

	void Pause();
	bool Paused();

	int CharHeight() const;
	int CharWidth() const;

	TCODConsole* buffer;
	void FlipBuffer();
	void Draw(Coordinate = Game::Inst()->upleft, TCODConsole* = Game::Inst()->buffer, bool drawUI = true);

	static int DiceToInt(TCOD_dice_t);

	static void Undesignate(Coordinate, Coordinate);

	/*      NPCS        NPCS        NPCS        */
	std::map<int,boost::shared_ptr<NPC> > npcList;
	int CreateNPC(Coordinate, NPCType);
	void BumpEntity(int);
	int DistanceNPCToCoordinate(int, Coordinate);
	int OrcCount(); int GoblinCount();
	void OrcCount(int); void GoblinCount(int);
	void FindNearbyNPCs(boost::shared_ptr<NPC>, bool onlyHostiles = false);
	void RemoveNPC(boost::weak_ptr<NPC>);
	int FindMilitaryRecruit();
	std::map<std::string, boost::shared_ptr<Squad> > squadList;
	std::list<boost::shared_ptr<Squad> > hostileSquadList;
	void CreateSquad(std::string);
	static void SetSquadTargetCoordinate(Coordinate, boost::shared_ptr<Squad>);
	static void SetSquadTargetEntity(Coordinate, boost::shared_ptr<Squad>);
	NPCType GetRandomNPCTypeByTag(std::string tag);
	void CreateNPCs(int,NPCType,Coordinate,Coordinate);

	/*      CONSTRUCTIONS       CONSTRUCTIONS       CONSTRUCTIONS       */
	static bool CheckPlacement(Coordinate, Coordinate);
	static int PlaceConstruction(Coordinate, ConstructionType);
	static void DismantleConstruction(Coordinate, Coordinate);
	void RemoveConstruction(boost::weak_ptr<Construction>);
	static int PlaceStockpile(Coordinate, Coordinate, ConstructionType, int);
	std::map<int, boost::shared_ptr<Construction> > staticConstructionList;
	std::map<int, boost::shared_ptr<Construction> > dynamicConstructionList;
	Coordinate FindClosestAdjacent(Coordinate, boost::weak_ptr<Entity>);
	bool Adjacent(Coordinate, boost::weak_ptr<Entity>);
	boost::weak_ptr<Construction> GetConstruction(int);
	boost::weak_ptr<Construction> FindConstructionByTag(ConstructionTag);

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
	static int ItemTypeCount;
	static int ItemCatCount;
	boost::shared_ptr<Job> StockpileItem(boost::weak_ptr<Item>, bool returnJob = false);
	boost::weak_ptr<Item> FindItemByCategoryFromStockpiles(ItemCategory, int flags = 0, int value = 0);
	boost::weak_ptr<Item> FindItemByTypeFromStockpiles(ItemType, int flags = 0, int value = 0);
	void CreateItems(int,ItemType,Coordinate,Coordinate);

	/*      NATURE      NATURE      NATURE      */
	std::map<int, boost::shared_ptr<NatureObject> > natureList;
	std::list<boost::weak_ptr<WaterNode> > waterList;
	void CreateWater(Coordinate);
	void CreateWater(Coordinate,int,int=0);
	Coordinate FindWater(Coordinate);
	static bool CheckTree(Coordinate, Coordinate);
	static void FellTree(Coordinate, Coordinate);
	static void DesignateTree(Coordinate, Coordinate);
	void RemoveNatureObject(boost::weak_ptr<NatureObject>);
	static void HarvestWildPlant(Coordinate, Coordinate);
	static void DesignateBog(Coordinate, Coordinate);
	static bool CheckTileType(TileType, Coordinate, Coordinate);

	Season CurrentSeason();
	std::string SeasonToString(Season);
	void SpawnTillageJobs();
	void DeTillFarmPlots();
	void DecayItems();

	std::list<boost::weak_ptr<FilthNode> > filthList;
	void CreateFilth(Coordinate);
	void CreateFilth(Coordinate,int);

	std::list<boost::weak_ptr<BloodNode> > bloodList;
	void CreateBlood(Coordinate);
	void CreateBlood(Coordinate,int);
};
