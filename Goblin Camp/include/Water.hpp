#ifndef WATER_HEADER
#define WATER_HEADER

#include <libtcod.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Coordinate.hpp"

#define RIVERDEPTH 5000

class WaterNode : public boost::enable_shared_from_this<WaterNode> {
	private:
		int x, y;
		int depth;
		int graphic;
		TCODColor color;
		int inertCounter;
		bool inert;
        int timeFromRiverBed;
	public:
		WaterNode(int x=0,int y=0,int depth=0,int time=0);
		~WaterNode();
		void Update();
		void Draw(Coordinate);
		void MakeInert();
		void DeInert();
		int Depth();
		void Depth(int);
		void UpdateGraphic();
		Coordinate Position();
};

#endif
