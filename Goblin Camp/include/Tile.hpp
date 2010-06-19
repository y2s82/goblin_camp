#ifndef TILE_HEADER
#define TILE_HEADER

#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Water.hpp"
#include "Filth.hpp"

enum TileType {
	TILENONE,
	TILEGRASS,
	TILEDITCH,
	TILERIVERBED
};

class Tile {
    friend class GameMap;
	private:
		TileType _type;
		bool vis; //Does light pass through this tile? Tile type, but also constructions/objects affect this
		bool walkable;
		bool buildable;
		int _moveCost;
		int construction;
		bool low, blocksWater;
        boost::shared_ptr<WaterNode> water;
        int graphic;
        TCODColor color;
        int natureObject;
		std::set<int> npcList; //Set of NPC uid's
		std::set<int> itemList; //Set of Item uid's
		boost::shared_ptr<FilthNode> filth;

	public:
		Tile(TileType = TILEGRASS, int = 1);
		TileType type();
		void type(TileType);
		bool BlocksLight() const;
		void BlocksLight(bool);
		bool Walkable() const;
		void Walkable(bool);
		bool Buildable() const;
		void Buildable(bool);
		int moveCost() const;
		void moveCost(int);
		void MoveFrom(int);
		bool MoveTo(int);
		void Construction(int);
		int Construction() const;
		boost::weak_ptr<WaterNode> GetWater() const;
		void SetWater(boost::shared_ptr<WaterNode>);
		bool Low() const;
		void Low(bool);
		bool BlocksWater() const;
		void BlocksWater(bool);
        int Graphic() const;
        TCODColor Color() const;
        void NatureObject(int);
        int NatureObject() const;
		boost::weak_ptr<FilthNode> GetFilth() const;
		void SetFilth(boost::shared_ptr<FilthNode>);
};

#endif
