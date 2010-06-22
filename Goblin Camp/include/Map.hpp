#ifndef MAP_HEADER
#define MAP_HEADER

#include <libtcod.hpp>
#include <boost/multi_array.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"

class Map : public ITCODPathCallback {
	private:
		Map();
		static Map* instance;
		boost::multi_array<Tile, 2> tileMap;
		int width, height;

	public:
		static Map* Inst();
		float getWalkCost(int, int, int, int, void *) const;
		bool Walkable(int,int) const;
		void Walkable(int,int,bool);
		int Width();
		int Height();
		bool Buildable(int,int) const;
		void Buildable(int,int,bool);
		TileType Type(int,int);
		void Type(int,int,TileType);
		bool MoveTo(int,int,int);
		void MoveFrom(int,int,int);
		void Construction(int,int,int);
		int Construction(int,int);
		void Draw(Coordinate, TCODConsole*);
		boost::weak_ptr<WaterNode> GetWater(int,int);
		void SetWater(int,int,boost::shared_ptr<WaterNode>);
		bool Low(int,int) const;
		void Low(int,int,bool);
		bool BlocksWater(int,int) const;
		void BlocksWater(int,int,bool);
		std::set<int>* NPCList(int,int);
		int Graphic(int,int) const;
		TCODColor Color(int,int) const;
		void NatureObject(int,int,int);
		int NatureObject(int,int) const;
		std::set<int>* ItemList(int,int);
		boost::weak_ptr<FilthNode> GetFilth(int,int);
		void SetFilth(int,int,boost::shared_ptr<FilthNode>);
		bool BlocksLight(int, int) const;
		void BlocksLight(int, int, bool);
};

#endif
