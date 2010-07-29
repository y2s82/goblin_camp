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

/* I've placed all the serialization here, because I was unable to get
Goblin Camp to compile when each class' serialization function was in
the class' .cpp file. I assume that there is some magical Boost macro
that would fix that problem, but I unfortunately have very limited time
and I couldn't come up with a coherent answer just by googling. */
#include "stdafx.hpp"

#pragma warning(push, 2) //Boost::serialization generates a few very long warnings

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/multi_array.hpp>
#include <map>
#include <fstream>

#include "Game.hpp"
#include "Tile.hpp"
#include "Coordinate.hpp"
#include "JobManager.hpp"
#include "Map.hpp"
#include "StockManager.hpp"
#include "StatusEffect.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Door.hpp"
#include "Camp.hpp"
#include "Container.hpp"
#include "Blood.hpp"
#include "Entity.hpp"
#include "Attack.hpp"

template<class Archive>
void Coordinate::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
}

template<class Archive>
void Coordinate::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
}

template<class Archive>
void Game::save(Archive & ar, const unsigned int version) const  {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<OrganicItem>();
	ar.template register_type<FarmPlot>();
	ar.template register_type<Door>();
	ar & season;
	ar & time;
	ar & orcCount;
	ar & goblinCount;
	ar & npcList;
	ar & squadList;
	ar & hostileSquadList;
	ar & staticConstructionList;
	ar & dynamicConstructionList;
	ar & itemList;
	ar & freeItems;
	ar & natureList;
	ar & waterList;
	ar & upleft;
	ar & filthList;
	ar & bloodList;
}

template<class Archive>
void Game::load(Archive & ar, const unsigned int version) {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<OrganicItem>();
	ar.template register_type<FarmPlot>();
	ar.template register_type<Door>();
	ar & season;
	ar & time;
	ar & orcCount;
	ar & goblinCount;
	ar & npcList;
	ar & squadList;
	ar & hostileSquadList;
	ar & staticConstructionList;
	ar & dynamicConstructionList;
	ar & itemList;
	ar & freeItems;
	ar & natureList;
	ar & waterList;
	ar & upleft;
	ar & filthList;
	ar & bloodList;
}

template<class Archive>
void NPC::save(Archive & ar, const unsigned int version) const {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar & boost::serialization::base_object<Entity>(*this);
	ar & type;
	ar & timeCount;
	ar & jobs;
	ar & taskIndex;
	ar & nopath;
	ar & findPathWorking;
	ar & timer;
	ar & nextMove;
	ar & run;
	ar & _color.r;
	ar & _color.g;
	ar & _color.b;
	ar & _bgcolor.r;
	ar & _bgcolor.g;
	ar & _bgcolor.b;
	ar & _graphic;
	ar & taskBegun;
	ar & expert;
	ar & carried;
	ar & mainHand;
	ar & offHand;
	ar & thirst;
	ar & hunger;
	ar & thinkSpeed;
	ar & statusEffects;
	ar & health;
	ar & foundItem;
	ar & inventory;
	ar & needsNutrition;
	ar & baseStats;
	ar & aggressive;
	ar & coward;
	ar & aggressor;
	ar & dead;
	ar & squad;
	ar & attacks;
	ar & escaped;
}

template<class Archive>
void NPC::load(Archive & ar, const unsigned int version) {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar & boost::serialization::base_object<Entity>(*this);
	ar & type;
	ar & timeCount;
	ar & jobs;
	ar & taskIndex;
	ar & nopath;
	ar & findPathWorking;
	ar & timer;
	ar & nextMove;
	ar & run;
	ar & _color.r;
	ar & _color.g;
	ar & _color.b;
	ar & _bgcolor.r;
	ar & _bgcolor.g;
	ar & _bgcolor.b;
	ar & _graphic;
	ar & taskBegun;
	ar & expert;
	ar & carried;
	ar & mainHand;
	ar & offHand;
	ar & thirst;
	ar & hunger;
	ar & thinkSpeed;
	ar & statusEffects;
	ar & health;
	ar & foundItem;
	ar & inventory;
	ar & needsNutrition;
	ar & baseStats;
	ar & aggressive;
	ar & coward;
	ar & aggressor;
	ar & dead;
	ar & squad;
	ar & attacks;
	ar & escaped;
	InitializeAIFunctions();
}
template<class Archive>
void Item::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & graphic;
	ar & type;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & categories;
	ar & flammable;
	ar & attemptedStore;
	ar & decayCounter;
	ar & ownerFaction;
	ar & container;
}

template<class Archive>
void Item::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & graphic;
	ar & type;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & categories;
	ar & flammable;
	ar & attemptedStore;
	ar & decayCounter;
	ar & ownerFaction;
	ar & container;
}

template<class Archive>
void OrganicItem::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Item>(*this);
	ar & nutrition;
	ar & growth;
}
template<class Archive>
void OrganicItem::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Item>(*this);
	ar & nutrition;
	ar & growth;
}


template<class Archive>
void Entity::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & uid;
	ar & uids,
		ar & zone;
	ar & reserved;
	ar & name;
	ar & faction;
}

template<class Archive>
void Entity::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & uid;
	ar & uids;
	ar & zone;
	ar & reserved;
	ar & name;
	ar & faction;
}


template<class Archive>
void Job::save(Archive & ar, const unsigned int version) const {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<NatureObject>();
	ar.template register_type<Construction>();
	ar.template register_type<Door>();
	ar.template register_type<FarmPlot>();
	ar & _priority;
	ar & completion;
	ar & preReqs;
	ar & parent;
	ar & npcUid;
	ar & _zone;
	ar & menial;
	ar & paused;
	ar & waitingForRemoval;
	ar & reservedEntities;
	ar & reservedSpot;
	ar & attempts;
	ar & attemptMax;
	ar & connectedEntity;
	ar & reservedSpace;
	ar & name;
	ar & tasks;
	ar & internal;
}

template<class Archive>
void Job::load(Archive & ar, const unsigned int version) {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<NatureObject>();
	ar.template register_type<Construction>();
	ar.template register_type<Door>();
	ar.template register_type<FarmPlot>();
	ar & _priority;
	ar & completion;
	ar & preReqs;
	ar & parent;
	ar & npcUid;
	ar & _zone;
	ar & menial;
	ar & paused;
	ar & waitingForRemoval;
	ar & reservedEntities;
	ar & reservedSpot;
	ar & attempts;
	ar & attemptMax;
	ar & connectedEntity;
	ar & reservedSpace;
	ar & name;
	ar & tasks;
	ar & internal;
}

template<class Archive>
void Container::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Item>(*this);
	ar & items;
	ar & capacity;
	ar & reservedSpace;
}

template<class Archive>
void Container::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Item>(*this);
	ar & items;
	ar & capacity;
	ar & reservedSpace;
}


template<class Archive>
void StatusEffect::save(Archive & ar, const unsigned int version) const {
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & name;
	ar & type;
	ar & cooldown;
	ar & cooldownDefault;
	ar & statChanges;
}

template<class Archive>
void StatusEffect::load(Archive & ar, const unsigned int version) {
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & name;
	ar & type;
	ar & cooldown;
	ar & cooldownDefault;
	ar & statChanges;
}

template<class Archive>
void Squad::save(Archive & ar, const unsigned int version) const {
	ar & name;
	ar & memberReq;
	ar & members;
	ar & order;
	ar & targetCoordinate;
	ar & targetEntity;
	ar & priority;
	ar & weapon;
}

template<class Archive>
void Squad::load(Archive & ar, const unsigned int version) {
	ar & name;
	ar & memberReq;
	ar & members;
	ar & order;
	ar & targetCoordinate;
	ar & targetEntity;
	ar & priority;
	ar & weapon;
}

template<class Archive>
void Task::save(Archive & ar, const unsigned int version) const {
	ar & target;
	ar & entity;
	ar & action;
	ar & item;
}
template<class Archive>
void Task::load(Archive & ar, const unsigned int version) {
	ar & target;
	ar & entity;
	ar & action;
	ar & item;
}


template<class Archive>
void Stockpile::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Construction>(*this);
	ar & symbol;
	ar & a;
	ar & b;
	ar & capacity;
	ar & amount;
	ar & allowed;
	ar & used;
	ar & reserved;
	ar & containers;
}

template<class Archive>
void Stockpile::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Construction>(*this);
	ar & symbol;
	ar & a;
	ar & b;
	ar & capacity;
	ar & amount;
	ar & allowed;
	ar & used;
	ar & reserved;
	ar & containers;
}


template<class Archive>
void Construction::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & condition;
	ar & maxCondition;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & type;
	ar & walkable;
	ar & materials;
	ar & producer;
	ar & products;
	ar & jobList;
	ar & progress;
	ar & container;
	ar & materialsUsed;
	ar & stockpile;
	ar & farmplot;
	ar & time;
}

template<class Archive>
void Construction::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & condition;
	ar & maxCondition;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & type;
	ar & walkable;
	ar & materials;
	ar & producer;
	ar & products;
	ar & jobList;
	ar & progress;
	ar & container;
	ar & materialsUsed;
	ar & stockpile;
	ar & farmplot;
	ar & time;
}

template<class Archive>
void Door::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object <Construction>(*this);
	ar & closedGraphic;
}

template<class Archive>
void Door::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object <Construction>(*this);
	ar & closedGraphic;
}


template<class Archive>
void WaterNode::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & inertCounter;
	ar & inert;
	ar & timeFromRiverBed;
}

template<class Archive>
void WaterNode::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & inertCounter;
	ar & inert;
	ar & timeFromRiverBed;
}


template<class Archive>
void FilthNode::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

template<class Archive>
void FilthNode::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

template<class Archive>
void BloodNode::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

template<class Archive>
void BloodNode::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

template<class Archive>
void NatureObject::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & type;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
}

template<class Archive>
void NatureObject::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & type;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
}

template<class Archive>
void JobManager::save(Archive & ar, const unsigned int version) const {
	ar & availableList;
	ar & waitingList;
}

template<class Archive>
void JobManager::load(Archive & ar, const unsigned int version) {
	ar & availableList;
	ar & waitingList;
}

template<class Archive>
void Camp::save(Archive & ar, const unsigned int version) const {
	ar & center;
	ar & buildingCount;
}

template<class Archive>
void Camp::load(Archive & ar, const unsigned int version) {
	ar & center;
	ar & buildingCount;
	xAcc(center.X(), boost::accumulators::weight = buildingCount);
	yAcc(center.Y(), boost::accumulators::weight = buildingCount);
}

template<class Archive>
void StockManager::save(Archive & ar, const unsigned int version) const {
	ar & categoryQuantities;
	ar & typeQuantities;
	ar & minimums;
	ar & producables;
	ar & producers;
	ar & workshops;
	ar & fromTrees;
	ar & fromEarth;
	ar & designatedTrees;
	ar & treeFellingJobs;
}

template<class Archive>
void StockManager::load(Archive & ar, const unsigned int version) {
	ar & categoryQuantities;
	ar & typeQuantities;
	ar & minimums;
	ar & producables;
	ar & producers;
	ar & workshops;
	ar & fromTrees;
	ar & fromEarth;
	ar & designatedTrees;
	ar & treeFellingJobs;
}

template<class Archive>
void Map::save(Archive & ar, const unsigned int version) const {
	for (int x = 0; x < tileMap.size(); ++x) {
		for (int y = 0; y < tileMap[x].size(); ++y) {
			ar & tileMap[x][y];
		}
	}
	ar & width;
	ar & height;
}

template<class Archive>
void Map::load(Archive & ar, const unsigned int version) {
	for (int x = 0; x < tileMap.size(); ++x) {
		for (int y = 0; y < tileMap[x].size(); ++y) {
			ar & tileMap[x][y];
		}
	}
	ar & width;
	ar & height;
}

template<class Archive>
void Tile::save(Archive & ar, const unsigned int version) const {
	ar & _type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & _moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
}

template<class Archive>
void Tile::load(Archive & ar, const unsigned int version) {
	ar & _type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & _moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
}

void Game::SaveGame(std::string filename) {
	std::ofstream ofs(filename.c_str(), std::ios::binary);
	boost::archive::binary_oarchive oarch(ofs);
	oarch<<*instance;
	oarch<<*JobManager::Inst();
	oarch<<*Camp::Inst();
	oarch<<*StockManager::Inst();
	oarch<<*Map::Inst();
}


void Game::LoadGame(std::string filename) {
	Game::Inst()->Reset();
	std::ifstream ifs(filename.c_str(), std::ios::binary);
	boost::archive::binary_iarchive iarch(ifs);
	iarch>>*instance;
	iarch>>*JobManager::Inst();
	iarch>>*Camp::Inst();
	iarch>>*StockManager::Inst();
	iarch>>*Map::Inst();
}

template<class Archive>
void FarmPlot::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}

template<class Archive>
void FarmPlot::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}

template<class Archive>
void Attack::save(Archive & ar, const unsigned int version) const {
	ar & damageType;
	ar & damageAmount.addsub;
	ar & damageAmount.multiplier;
	ar & damageAmount.nb_dices;
	ar & damageAmount.nb_faces;
	ar & cooldown;
	ar & cooldownMax;
	ar & statusEffects;
	ar & ranged;
	ar & projectile;
}

template<class Archive>
void Attack::load(Archive & ar, const unsigned int version) {
	ar & damageType;
	ar & damageAmount.addsub;
	ar & damageAmount.multiplier;
	ar & damageAmount.nb_dices;
	ar & damageAmount.nb_faces;
	ar & cooldown;
	ar & cooldownMax;
	ar & statusEffects;
	ar & ranged;
	ar & projectile;
}


#pragma warning(pop)
