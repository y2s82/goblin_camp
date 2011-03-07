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

#include <boost/serialization/split_member.hpp>

#include "Entity.hpp"

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

class NatureObject : public Entity
{
	friend class boost::serialization::access;
	friend class Game;
	friend class Ice;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

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

#include <boost/serialization/version.hpp>
BOOST_CLASS_VERSION(NatureObject, 1)

template<class Archive>
void NatureObject::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & NatureObject::Presets[type].name;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
	ar & ice;
}

template<class Archive>
void NatureObject::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	std::string typeName;
	ar & typeName;
	bool failedToFindType = true;
	type = 0; //Default to whatever is the first wildplant
	for (int i = 0; i < NatureObject::Presets.size(); ++i) {
		if (boost::iequals(NatureObject::Presets[i].name, typeName)) {
			type = i;
			failedToFindType = false;
			break;
		}
	}
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
	if (failedToFindType) harvestable = true;
	if (version >= 1) {
		ar & ice;
	}
}

class Ice : public NatureObject {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	boost::shared_ptr<WaterNode> frozenWater;
public:
	Ice(Coordinate = Coordinate(0,0), NatureObjectType = 0);
	~Ice();
};

#include <boost/serialization/version.hpp>
BOOST_CLASS_VERSION(Ice, 0)

template<class Archive>
void Ice::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<NatureObject>(*this);
	ar & frozenWater;
}

template<class Archive>
void Ice::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<NatureObject>(*this);
	ar & frozenWater;
}