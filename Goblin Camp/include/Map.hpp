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
#pragma once

#include <boost/multi_array.hpp>
#include <boost/serialization/split_member.hpp>
#include <libtcod.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"

class Map : public ITCODPathCallback {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	Map();
	static Map* instance;
	boost::multi_array<Tile, 2> tileMap;
	int width, height;
	float waterlevel;

public:
	TCODHeightMap *heightMap;
	static Map* Inst();
	void Reset(int,int);
	float getWalkCost(int, int, int, int, void *) const;
	bool Walkable(int,int) const;
	bool Walkable(int,int,void*) const;
	void SetWalkable(int,int,bool);
	int Width();
	int Height();
	bool Buildable(int,int) const;
	void Buildable(int,int,bool);
	TileType Type(int,int);
	void Type(int,int,TileType);
	void MoveTo(int,int,int);
	void MoveFrom(int,int,int);
	void SetConstruction(int,int,int);
	int GetConstruction(int,int) const;
	void Draw(Coordinate, TCODConsole*);
	boost::weak_ptr<WaterNode> GetWater(int,int);
	void SetWater(int,int,boost::shared_ptr<WaterNode>);
	bool Low(int,int) const;
	void Low(int,int,bool);
	bool BlocksWater(int,int) const;
	void BlocksWater(int,int,bool);
	std::set<int>* NPCList(int,int);
	int Graphic(int,int) const;
	TCODColor ForeColor(int,int) const;
	void ForeColor(int,int,TCODColor);
	TCODColor BackColor(int,int) const;
	void NatureObject(int,int,int);
	int NatureObject(int,int) const;
	std::set<int>* ItemList(int,int);
	boost::weak_ptr<FilthNode> GetFilth(int,int);
	void SetFilth(int,int,boost::shared_ptr<FilthNode>);
	boost::weak_ptr<BloodNode> GetBlood(int,int);
	void SetBlood(int,int,boost::shared_ptr<BloodNode>);
	bool BlocksLight(int, int) const;
	void BlocksLight(int, int, bool);
	bool LineOfSight(Coordinate, Coordinate);
	bool LineOfSight(int, int, int, int);
	void Mark(int,int);
	void Unmark(int,int);
	bool GroundMarked(int,int);
	int GetMoveModifier(int,int);
	float GetWaterlevel();
	void WalkOver(int,int);
	void Naturify(int,int);
	void Corrupt(int x, int y, int magnitude=200);
};
