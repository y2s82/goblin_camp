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

#include <string>
#include <vector>

#include "Entity.hpp"
#include "data/Serialization.hpp"

class Coordinate;
class WaterNode;

typedef int ItemType;
typedef int NatureObjectType;

class NatureObjectPreset {
public:
	NatureObjectPreset();
	std::string name;
	int graphic;
	TCODColor color;
	std::list<ItemType> components;
	int rarity;
	int cluster;
	int condition;
	bool tree, harvestable, walkable;
	float minHeight, maxHeight;
	bool evil;
	std::string fallbackGraphicsSet;
	int graphicsHint;
};

class NatureObject : public Entity {
	GC_SERIALIZABLE_CLASS
	
	friend class Game;
	friend class Ice;
protected:
	NatureObject(Coordinate = Coordinate(0,0), NatureObjectType = 0);
	NatureObjectType type;
	int graphic;
	TCODColor color;
	bool marked;
	int condition;
	bool tree, harvestable;
	bool ice;
public:
	~NatureObject();
	static std::vector<NatureObjectPreset> Presets;
	static void LoadPresets(std::string);

	int Type();

	int GetGraphicsHint() const;
	void Draw(Coordinate, TCODConsole*);
	void Update();
	virtual void CancelJob(int=0);
	void Mark();
	void Unmark();
	bool Marked();
	int Fell();
	int Harvest();
	bool Tree();
	bool Harvestable();
	bool IsIce();
};

BOOST_CLASS_VERSION(NatureObject, 1)

class Ice : public NatureObject {
	GC_SERIALIZABLE_CLASS
	
	boost::shared_ptr<WaterNode> frozenWater;
public:
	Ice(Coordinate = Coordinate(0,0), NatureObjectType = 0);
	~Ice();
};

BOOST_CLASS_VERSION(Ice, 0)
