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
#include "stdafx.hpp"

#include "Stats.hpp"

Stats::Stats() : points(0), itemsBurned(0), filthCreated(0), filthOutsideMap(0),
constructions(0), production(0) {}

Stats* Stats::instance = 0;

Stats* Stats::Inst() {
	if (!instance) instance = new Stats();
	return instance;
}

void Stats::AddPoints(unsigned amount) { points += amount; }
unsigned Stats::GetPoints() { return points; }

void Stats::FilthCreated(unsigned amount) { filthCreated += amount; }
unsigned Stats::GetFilthCreated() { return filthCreated; }

void Stats::FilthFlowsOffEdge(unsigned amount) { filthOutsideMap += amount; AddPoints(amount); }
unsigned Stats::GetFilthFlownOff() { return filthOutsideMap; }

void Stats::ItemBurned(unsigned amount) { itemsBurned += amount; }
unsigned Stats::GetItemsBurned() { return itemsBurned; }

void Stats::ConstructionBuilt(std::string name) { 
	constructionsBuilt[name] += 1; 
	++constructions; 
	AddPoints(10U);
}
unsigned Stats::GetConstructionsBuilt() { return constructions; }

void Stats::ItemBuilt(std::string name) { itemsBuilt[name] += 1; ++production; AddPoints(); }
unsigned Stats::GetItemsBuilt() { return production; }


void Stats::Reset() {
	delete instance;
	instance = new Stats();
}

namespace {
	template<typename A, typename B>
	void SerializeUnorderedMap(const boost::unordered_map<A,B> &map, OutputArchive& ar) {
		size_t size = map.size();
		ar & size;
		for (typename boost::unordered_map<A, B>::const_iterator iter = map.begin(); iter != map.end(); ++iter) {
			ar & iter->first;
			ar & iter->second;
		}
	}

	template<typename A, typename B>
	void UnserializeUnorderedMap(boost::unordered_map<A,B> &map, InputArchive& ar) {
		size_t size;
		ar & size;
		for (size_t i = 0; i < size; ++i) {
			A a;
			ar & a;
			B b;
			ar & b;
			map[a] = b;
		}
	}
}

void Stats::save(OutputArchive& ar, const unsigned int version) const {
	ar & points;

	SerializeUnorderedMap(deaths, ar);
	SerializeUnorderedMap(constructionsBuilt, ar);
	SerializeUnorderedMap(itemsBuilt, ar);

	ar & itemsBurned;
	ar & filthCreated;
	ar & filthOutsideMap;
	ar & constructions;
	ar & production;
}

void Stats::load(InputArchive& ar, const unsigned int version) {
	ar & points;

	UnserializeUnorderedMap(deaths, ar);
	UnserializeUnorderedMap(constructionsBuilt, ar);
	UnserializeUnorderedMap(itemsBuilt, ar);

	ar & itemsBurned;
	ar & filthCreated;
	ar & filthOutsideMap;
	ar & constructions;
	ar & production;
}