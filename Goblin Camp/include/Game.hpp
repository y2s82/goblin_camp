#ifndef GAME_HEADER
#define GAME_HEADER

#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <list>
#include <libtcod.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"
#include "NPC.hpp"
#include "NatureObject.hpp"

#define BFS_MAX_DISTANCE 20

#define MONTH_LENGTH (UPDATES_PER_SECOND * 60 * 1)

enum Seasons {
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
	private:
		Game();
		static Game* instance;
		int screenWidth, screenHeight;
		Seasons season;
		int time;
		int orcCount, goblinCount;
		bool paused;
		int charWidth, charHeight;
	public:
		static Game* Inst();

		int Distance(Coordinate,Coordinate);

/*      NPCS        NPCS        NPCS        */
		std::map<int,boost::shared_ptr<NPC> > npcList;
		int CreateNPC(Coordinate, NPCType);
		void BumpEntity(int);
		int DistanceNPCToCoordinate(int, Coordinate);
        int OrcCount(); int GoblinCount();
		void FindNearbyNPCs(boost::shared_ptr<NPC>);
		void RemoveNPC(boost::weak_ptr<NPC>);
		int FindMilitaryRecruit();
		std::map<std::string, boost::shared_ptr<Squad> > squadList;
		void CreateSquad(std::string);


/*      CONSTRUCTIONS       CONSTRUCTIONS       CONSTRUCTIONS       */
		static bool CheckPlacement(Coordinate, Coordinate);
		static int PlaceConstruction(Coordinate, ConstructionType);
		void RemoveConstruction(boost::weak_ptr<Construction>);
		static int PlaceStockpile(Coordinate, Coordinate, ConstructionType, int);
		std::map<int,boost::shared_ptr<Construction> > constructionList;
		Coordinate FindClosestAdjacent(Coordinate, boost::weak_ptr<Entity>);
		bool Adjacent(Coordinate, boost::weak_ptr<Entity>);
		boost::weak_ptr<Construction> GetConstruction(int);

/*      ITEMS       ITEMS       ITEMS       */
		int CreateItem(Coordinate, ItemType, bool stockpile = false,
			int ownerFaction = 0,
			std::vector<boost::weak_ptr<Item> > = std::vector<boost::weak_ptr<Item> >());
		void RemoveItem(boost::weak_ptr<Item>);
		boost::weak_ptr<Item> GetItem(int);
		std::map<int,boost::shared_ptr<Item> > itemList;
		void ItemContained(boost::weak_ptr<Item>, bool contained);
		std::set<boost::weak_ptr<Item> > freeItems; //Free as in not contained

		static int ItemTypeCount;
		static int ItemCatCount;
		void StockpileItem(boost::weak_ptr<Item>);
		boost::weak_ptr<Item> FindItemByCategoryFromStockpiles(ItemCategory);
		boost::weak_ptr<Item> FindItemByTypeFromStockpiles(ItemType);

        std::map<int, boost::shared_ptr<NatureObject> > natureList;

		static void DoNothing();
		static void Exit();

		int ScreenWidth() const;
		int ScreenHeight() const;
		void Init(int,int,bool=false);
		void GenerateMap();


		std::list<boost::weak_ptr<WaterNode> > waterList;
		void CreateWater(Coordinate);
		void CreateWater(Coordinate,int,int=0);
		Coordinate FindWater(Coordinate);

		void Update();
		Coordinate upleft;

		Seasons Season();
		std::string SeasonToString(Seasons);
		void SpawnTillageJobs();
		void DeTillFarmPlots();
		void DecayItems();

		static bool CheckTree(Coordinate, Coordinate);
		static void FellTree(Coordinate, Coordinate);
		void RemoveNatureObject(boost::weak_ptr<NatureObject>);
		static void HarvestWildPlant(Coordinate, Coordinate);

		std::list<boost::weak_ptr<FilthNode> > filthList;
		void CreateFilth(Coordinate);
		void CreateFilth(Coordinate,int);

		void Pause();
		bool Paused();

		int CharHeight() const;
		int CharWidth() const;

		TCODConsole* buffer;
		void FlipBuffer();
		void Draw(Coordinate = Game::Inst()->upleft, TCODConsole* = Game::Inst()->buffer, bool drawUI = true);
};

#endif
