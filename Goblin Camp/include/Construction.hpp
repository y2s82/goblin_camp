#pragma once

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <ticpp.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


#include "Entity.hpp"
#include "Item.hpp"
#include "Container.hpp"

enum BuildResult {
	BUILD_NOMATERIAL = -99999
};

typedef int ConstructionType;
#define	CARPENTERSSHOP 1
#define BONECARVERSSHOP 2
#define	WOODENWALL 0
#define	STOCKPILE 3
#define	FARMPLOT 4

#define NOTFULL (1 << 0)

class ConstructionPreset {
    public:
    ConstructionPreset();
    int maxCondition;
    std::vector<int> graphic;
    bool walkable;
	std::list<ItemCategory> materials;
	bool producer;
	std::vector<ItemType> products;
	std::string name;
	Coordinate blueprint;
	bool wall, stockpile, farmPlot;
	bool door;
	Coordinate productionSpot;
	bool dynamic;
};

class Construction : public Entity {
    friend class Game;

	protected:
		Construction(ConstructionType = 0, Coordinate = Coordinate(0,0));

		int _condition, maxCondition;
		std::vector<int> graphic;
		TCODColor color;
		ConstructionType _type;
		bool walkable;
		std::list<ItemCategory> materials;
		bool producer;
		std::vector<ItemType> products;
		std::deque<ItemType> jobList;
		int progress;
		void SpawnProductionJob();
		boost::shared_ptr<Container> container;
		boost::shared_ptr<Container> materialsUsed;
		bool stockpile, farmplot;

		void UpdateWallGraphic(bool recurse = true);
	public:
		~Construction();
		static Coordinate Blueprint(ConstructionType);
		static Coordinate ProductionSpot(ConstructionType);
		void condition(int);
		int condition();
		virtual void Draw(Coordinate, TCODConsole*);
		int Build();
		ConstructionType type();
		std::list<ItemCategory>* MaterialList();
		bool Producer();
		std::vector<ItemType>* Products();
		ItemType Products(int);
		void AddJob(ItemType);
		virtual void CancelJob(int=0);
		std::deque<ItemType>* JobList();
		ItemType JobList(int);
		virtual int Use();
		static std::vector<ConstructionPreset> Presets;
		static void LoadPresets(ticpp::Document);
        virtual boost::weak_ptr<Container> Storage();
		bool IsStockpile();
		bool IsFarmplot();
		virtual void Update();
};
