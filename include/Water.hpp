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

#include <boost/enable_shared_from_this.hpp>
#include <libtcod.hpp>
#include "Coordinate.hpp"
#include "data/Serialization.hpp"

#define RIVERDEPTH 5000

class WaterNode : public boost::enable_shared_from_this<WaterNode> {
	GC_SERIALIZABLE_CLASS
	
	Coordinate pos;
	int depth;
	int graphic;
	TCODColor color;
	int inertCounter;
	bool inert;
	int timeFromRiverBed;
	int filth;
	bool coastal;
public:
	WaterNode(const Coordinate& pos = undefined, int depth = 0, int time = 0);
	~WaterNode();

	Coordinate Position();
	void Position(const Coordinate&);
	int X();
	int Y();
	void X(int);
	void Y(int);

	bool Update();
	void MakeInert();
	void DeInert();
	int Depth();
	void Depth(int);
	void UpdateGraphic();
	void AddFilth(int);
	int GetFilth();
	int GetGraphic();
	TCODColor GetColor();
	bool IsCoastal();
};

BOOST_CLASS_VERSION(WaterNode, 0)
