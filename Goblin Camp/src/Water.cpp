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

#include <cmath>

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/weak_ptr.hpp>

#include "Logger.hpp"
#include "Water.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "GCamp.hpp"

WaterNode::WaterNode(int vx, int vy, int vdepth, int time) :
x(vx), y(vy), depth(vdepth),
	graphic('?'), color(TCODColor(0,128,255)),
	inertCounter(0), inert(false),
	timeFromRiverBed(time)
{
	UpdateGraphic();
}

WaterNode::~WaterNode() {}

void WaterNode::Draw(Coordinate upleft, TCODConsole* console) {
	int screenX = x - upleft.X();
	int screenY = y - upleft.Y();

	if (depth > 0) {
		if (screenX >= 0 && screenX < console->getWidth() &&
			screenY >= 0 && screenY < console->getHeight()) {
				console->putCharEx(screenX, screenY, graphic, color, TCODColor::black);
		}
	}
}

void WaterNode::Update() {
	double divided;

	if (inert) {
		++inertCounter;
	}

	if (!inert || inertCounter > (UPDATES_PER_SECOND*1)) {

		if (Map::Inst()->Type(x,y) == TILERIVERBED) {
			timeFromRiverBed = 1000;
			if (depth < RIVERDEPTH) depth = RIVERDEPTH;
		}


		inertCounter = 0;

		if (depth > 1) {

			std::vector<boost::weak_ptr<WaterNode> > waterList;
			std::vector<Coordinate> coordList;
			int depthSum = 0;
			for (int ix = x-1; ix <= x+1; ++ix) {
				for (int iy = y-1; iy <= y+1; ++iy) {
					if (ix >= 0 && ix < Map::Inst()->Width() && iy >= 0 && iy < Map::Inst()->Height()) {
						if ((Map::Inst()->Low(x,y) == Map::Inst()->Low(ix,iy) || depth > RIVERDEPTH*3 || Map::Inst()->Low(ix,iy)) && !Map::Inst()->BlocksWater(ix,iy)) {
							waterList.push_back(Map::Inst()->GetWater(ix,iy));
							coordList.push_back(Coordinate(ix,iy));
							if (waterList.back().lock()) depthSum += waterList.back().lock()->depth;
						}
					}
				}
			}

			if (timeFromRiverBed > 0) --timeFromRiverBed;
			divided = ((double)depthSum/waterList.size());

			for (unsigned int i = 0; i < waterList.size(); ++i) {
				if (waterList[i].lock()) {
					waterList[i].lock()->depth = (int)divided;
					if (Map::Inst()->Low(coordList[i].X(), coordList[i].Y()) && waterList[i].lock()->depth < RIVERDEPTH) waterList[i].lock()->depth += 10;
					waterList[i].lock()->timeFromRiverBed = timeFromRiverBed;
					waterList[i].lock()->UpdateGraphic();
				}
				else {
					Game::Inst()->CreateWater(coordList[i], (int)divided, timeFromRiverBed);
				}
			}

		} else if (rand() % 100 == 0) {
			if (depth > 0) --depth;
		}
	}
}

void WaterNode::MakeInert() {inert = true;}
void WaterNode::DeInert() {inert = false;}
int WaterNode::Depth() {return depth;}

void WaterNode::Depth(int newDepth) {
	depth = newDepth;
	UpdateGraphic();
}

void WaterNode::UpdateGraphic() {
	if (depth == 0) {
		graphic = ' ';
	} else if (depth == 2) {
		graphic = TCOD_CHAR_BLOCK3;
	} else if (depth == 1) {
		graphic = '.';
	} else {
		graphic = 219;
	}

	int col = std::max(255-(int)(depth/25),100);
	if (color.g < col/2) ++color.g;
	if (color.b < col) ++color.b;
	if (color.g > col/2) --color.g;
	if (color.b > col) --color.b;
	if (rand() % 5 == 0 && color.b < 247) color.b += 8;
	if (rand() % 10000 == 0) color.g += rand() % 25;
}

Coordinate WaterNode::Position() {return Coordinate(x,y);}

