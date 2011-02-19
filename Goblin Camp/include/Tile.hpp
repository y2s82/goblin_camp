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

#include <set>
#include <vector>
#include <string>

#include <boost/serialization/split_member.hpp>
#include <boost/shared_ptr.hpp>

#include "Water.hpp"
#include "Filth.hpp"
#include "Blood.hpp"
#include "Fire.hpp"

enum TileType {
	TILENONE,
	TILEGRASS,
	TILEDITCH,
	TILERIVERBED,
	TILEBOG,
	TILEROCK,
	TILEMUD,
	TILE_TYPE_COUNT
};

class Tile {
	friend class boost::serialization::access;
	friend class Map;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	TileType type;
	bool vis; //Does light pass through this tile? Tile type, but also constructions/objects affect this
	bool walkable;
	bool buildable;
	int moveCost;
	int construction;
	bool low, blocksWater;
	boost::shared_ptr<WaterNode> water;
	int graphic;
	TCODColor foreColor, originalForeColor;
	TCODColor backColor;
	int natureObject;
	std::set<int> npcList; //Set of NPC uid's
	std::set<int> itemList; //Set of Item uid's
	boost::shared_ptr<FilthNode> filth;
	boost::shared_ptr<BloodNode> blood;
	boost::shared_ptr<FireNode> fire;
	bool marked;
	int walkedOver, corruption;
	bool territory;
	int burnt;
	Direction flow;

public:
	Tile(TileType = TILEGRASS, int = 1);
	TileType GetType();
	void SetType(TileType);
	bool BlocksLight() const;
	void SetBlocksLight(bool);
	bool IsWalkable() const;
	void SetWalkable(bool);
	bool IsBuildable() const;
	void SetBuildable(bool);
	int GetMoveCost() const;
	int GetMoveCost(void*) const;
	void SetMoveCost(int);
	void MoveFrom(int);
	void MoveTo(int);
	void SetConstruction(int);
	int GetConstruction() const;
	boost::weak_ptr<WaterNode> GetWater() const;
	void SetWater(boost::shared_ptr<WaterNode>);
	bool IsLow() const;
	void SetLow(bool);
	bool BlocksWater() const;
	void SetBlocksWater(bool);
	int GetGraphic() const;
	TCODColor GetForeColor() const;
	TCODColor GetBackColor() const;
	void SetNatureObject(int);
	int GetNatureObject() const;
	boost::weak_ptr<FilthNode> GetFilth() const;
	void SetFilth(boost::shared_ptr<FilthNode>);
	boost::weak_ptr<BloodNode> GetBlood() const;
	void SetBlood(boost::shared_ptr<BloodNode>);
	boost::weak_ptr<FireNode> GetFire() const;
	void SetFire(boost::shared_ptr<FireNode>);
	void Mark();
	void Unmark();
	void WalkOver();
	void Corrupt(int magnitude);
	static TileType StringToTileType(std::string);
	void Burn(int magnitude);
	int GetTerrainMoveCost() const;
};

BOOST_CLASS_VERSION(Tile, 0)

template<class Archive>
void Tile::save(Archive & ar, const unsigned int version) const {
	ar & type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & originalForeColor.r;
	ar & originalForeColor.g;
	ar & originalForeColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
	ar & marked;
	ar & walkedOver;
	ar & corruption;
	ar & territory;
	ar & burnt;
	ar & fire;
	ar & flow;
}

template<class Archive>
void Tile::load(Archive & ar, const unsigned int version) {
	ar & type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & originalForeColor.r;
	ar & originalForeColor.g;
	ar & originalForeColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
	ar & marked;
	ar & walkedOver;
	ar & corruption;
	ar & territory;
	ar & burnt;
	ar & fire;
	ar & flow;
}