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

#include <boost/unordered_map.hpp>

#include "data/Serialization.hpp"

class Stats {
	GC_SERIALIZABLE_CLASS
	
	friend class Game;

	Stats();
	static Stats* instance;

	unsigned points;
	unsigned itemsBurned;
	unsigned filthCreated;
	unsigned filthOutsideMap;
	unsigned constructions;
	unsigned production;
public:
	static Stats* Inst();

	void AddPoints(unsigned amount=1);
	unsigned GetPoints();

	void FilthCreated(unsigned amount=1);
	unsigned GetFilthCreated();

	void FilthFlowsOffEdge(unsigned amount=1);
	unsigned GetFilthFlownOff();

	boost::unordered_map<std::string, unsigned> deaths;
	boost::unordered_map<std::string, unsigned> constructionsBuilt;
	void ConstructionBuilt(std::string);
	unsigned GetConstructionsBuilt();

	boost::unordered_map<std::string, unsigned> itemsBuilt;
	void ItemBuilt(std::string);
	unsigned GetItemsBuilt();

	void ItemBurned(unsigned amount=1);
	unsigned GetItemsBurned();

	static void Reset();
};

BOOST_CLASS_VERSION(Stats, 0)