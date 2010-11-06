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
#include <boost/serialization/version.hpp>
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
#include <boost/cstdint.hpp>

#include "Logger.hpp"
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
#include "SpawningPool.hpp"

// IMPORTANT
// Implementing class versioning properly is an effort towards backward compatibility for saves,
// or at least eliminating those pesky crashes. This means some discipline in editing following
// functions. General rules:
//   - If you change ::save, you change ::load accordingly, and vice versa.
//   - If you change any of them, therefore changing the file structure,
//     (and this is vital) *increment the appropriate class' version*.
//   - No reordering of old fields, Boost.Serialization is very order-dependent.
//     Append new fields at the end, and enclose them with the correct version check.
//   - If you need to remove any old field, remove it from the saving code, but
//     *preserve loading code for the previous version* (use dummy variable of
//     the same type) -- no point in having zeroed fields in saved game with newer class version,
//     but old saves will always have those fields present.
//   - Do not change the magic constant. It's introduced to ease recognition of the save
//     files, especially after implementing these rules.
//
//  NB: When saving, we always use newest version, so version checks have to be
//      present only in loading code.
//
// File format after this revision:
//
//  These two fields MUST always be present:
//    - magic constant (uint32_t, little endian):
//        This value should never change and must always be equal to saveMagicConst.
//        If it's not, the file is not Goblin Camp save, and MUST be rejected.
//    - file format version (uint8_t, little endian):
//        This value represents overall file format, as defined here.
//        It should be incremented when file format changes so much that maintaining backward 
//        compatibility is not possible or feasible. Parser MUST NOT attempt any further decoding
//        if file format version is different than build's fileFormatConst.
//        
//        File format version of 0xFF is reserved for experimental file formats,
//        and should never be used in production branches.
//
//  These fields are specific to current file format version:
//    - 0x00 (uint64_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - actual serialised payload, defined and processed by Boost.Serialization machinery

// Magic constant: reversed fourcc 'GCMP'
// (so you can see it actually spelled like this when hex-viewing the save).
const boost::uint32_t saveMagicConst = 0x47434d50;

// File format version (8-bit, because it should not change too often).
const boost::uint8_t fileFormatConst = 0x00;

//
// class Coordinate
//
BOOST_CLASS_VERSION(Coordinate, 0)

template<class Archive>
void Coordinate::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
}

template<class Archive>
void Coordinate::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & x;
		ar & y;
	}
}

//
// class Game
//
BOOST_CLASS_VERSION(Game, 0)

template<class Archive>
void Game::save(Archive & ar, const unsigned int version) const  {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<OrganicItem>();
	ar.template register_type<FarmPlot>();
	ar.template register_type<Door>();
	ar.template register_type<SpawningPool>();
	ar & season;
	ar & time;
	ar & orcCount;
	ar & goblinCount;
	ar & peacefulFaunaCount;
	ar & safeMonths;
	ar & devMode;
	ar & marks;
	ar & upleft;
	ar & npcList;
	ar & squadList;
	ar & hostileSquadList;
	ar & staticConstructionList;
	ar & dynamicConstructionList;
	ar & itemList;
	ar & freeItems;
	ar & flyingItems;
	ar & stoppedItems;
	ar & natureList;
	ar & waterList;
	ar & filthList;
	ar & bloodList;
}

template<class Archive>
void Game::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar.template register_type<Container>();
		ar.template register_type<Item>();
		ar.template register_type<Entity>();
		ar.template register_type<OrganicItem>();
		ar.template register_type<FarmPlot>();
		ar.template register_type<Door>();
		ar.template register_type<SpawningPool>();
		ar & season;
		ar & time;
		ar & orcCount;
		ar & goblinCount;
		ar & peacefulFaunaCount;
		ar & safeMonths;
		ar & devMode;
		ar & marks;
		ar & upleft;
		ar & npcList;
		ar & squadList;
		ar & hostileSquadList;
		ar & staticConstructionList;
		ar & dynamicConstructionList;
		ar & itemList;
		ar & freeItems;
		ar & flyingItems;
		ar & stoppedItems;
		ar & natureList;
		ar & waterList;
		ar & filthList;
		ar & bloodList;
	}
}

//
// class NPC
//
BOOST_CLASS_VERSION(NPC, 0)

template<class Archive>
void NPC::save(Archive & ar, const unsigned int version) const {
	ar.template register_type<Container>();
	ar.template register_type<Item>();
	ar.template register_type<Entity>();
	ar.template register_type<SkillSet>();
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
	ar & armor;
	ar & quiver;
	ar & thirst;
	ar & hunger;
	ar & weariness;
	ar & thinkSpeed;
	ar & statusEffects;
	ar & health;
	ar & maxHealth;
	ar & foundItem;
	ar & inventory;
	ar & needsNutrition;
	ar & needsSleep;
	ar & hasHands;
	ar & baseStats;
	ar & effectiveStats;
	ar & baseResistances;
	ar & effectiveResistances;
	ar & aggressive;
	ar & coward;
	ar & aggressor;
	ar & dead;
	ar & squad;
	ar & attacks;
	ar & escaped;
	ar & Skills;
}

template<class Archive>
void NPC::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar.template register_type<Container>();
		ar.template register_type<Item>();
		ar.template register_type<Entity>();
		ar.template register_type<SkillSet>();
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
		ar & armor;
		ar & quiver;
		ar & thirst;
		ar & hunger;
		ar & weariness;
		ar & thinkSpeed;
		ar & statusEffects;
		ar & health;
		ar & maxHealth;
		ar & foundItem;
		ar & inventory;
		ar & needsNutrition;
		ar & needsSleep;
		ar & hasHands;
		ar & baseStats;
		ar & effectiveStats;
		ar & baseResistances;
		ar & effectiveResistances;
		ar & aggressive;
		ar & coward;
		ar & aggressor;
		ar & dead;
		ar & squad;
		ar & attacks;
		ar & escaped;
		ar & addedTasksToCurrentJob;
		ar & Skills;
	}
	
	InitializeAIFunctions();
}

//
// class Item
//
BOOST_CLASS_VERSION(Item, 0)

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
	ar & attack;
	ar & resistances;
	ar & container;
	ar & internal;
}

template<class Archive>
void Item::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
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
		ar & attack;
		ar & resistances;
		ar & container;
		ar & internal;
	}
}

//
// class OrganicItem
//
BOOST_CLASS_VERSION(OrganicItem, 0)

template<class Archive>
void OrganicItem::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Item>(*this);
	ar & nutrition;
	ar & growth;
}
template<class Archive>
void OrganicItem::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & boost::serialization::base_object<Item>(*this);
		ar & nutrition;
		ar & growth;
	}
}

//
// class Entity
//
BOOST_CLASS_VERSION(Entity, 0)

template<class Archive>
void Entity::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & uid;
	ar & uids;
	ar & zone;
	ar & reserved;
	ar & name;
	ar & faction;
	ar & velocity;
	ar & nextVelocityMove;
	ar & velocityTarget;
}

template<class Archive>
void Entity::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & x;
		ar & y;
		ar & uid;
		ar & uids;
		ar & zone;
		ar & reserved;
		ar & name;
		ar & faction;
		ar & velocity;
		ar & nextVelocityMove;
		ar & velocityTarget;
	}
}

//
// class Job
//
BOOST_CLASS_VERSION(Job, 0)

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
	ar & tool;
	ar & name;
	ar & tasks;
	ar & internal;
}

template<class Archive>
void Job::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
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
		ar & tool;
		ar & name;
		ar & tasks;
		ar & internal;
	}
}

//
// class Container
//
BOOST_CLASS_VERSION(::Container, 0)

template<class Archive>
void Container::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Item>(*this);
	ar & items;
	ar & capacity;
	ar & reservedSpace;
	ar & listenersAsUids;
	ar & water;
	ar & filth;
}

template<class Archive>
void Container::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & boost::serialization::base_object<Item>(*this);
		ar & items;
		ar & capacity;
		ar & reservedSpace;
		ar & listenersAsUids;
		ar & water;
		ar & filth;
	}
}

//
// class StatusEffect
//
BOOST_CLASS_VERSION(StatusEffect, 0)

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
	ar & resistanceChanges;
	ar & damage;
	ar & bleed;
}

template<class Archive>
void StatusEffect::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & graphic;
		ar & color.r;
		ar & color.g;
		ar & color.b;
		ar & name;
		ar & type;
		ar & cooldown;
		ar & cooldownDefault;
		ar & statChanges;
		ar & resistanceChanges;
		ar & damage;
		ar & bleed;
	}
}

//
// class Squad
//
BOOST_CLASS_VERSION(Squad, 0)

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
	ar & armor;
}

template<class Archive>
void Squad::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & name;
		ar & memberReq;
		ar & members;
		ar & order;
		ar & targetCoordinate;
		ar & targetEntity;
		ar & priority;
		ar & weapon;
		ar & armor;
	}
}

//
// class Task
//
BOOST_CLASS_VERSION(Task, 0)

template<class Archive>
void Task::save(Archive & ar, const unsigned int version) const {
	ar & target;
	ar & entity;
	ar & action;
	ar & item;
	ar & flags;
}
template<class Archive>
void Task::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & target;
		ar & entity;
		ar & action;
		ar & item;
		ar & flags;
	}
}

//
// class Stockpile
//
BOOST_CLASS_VERSION(Stockpile, 0)

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
	if (version == 0) {
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
	ar & dismantle;
	ar & time;
	ar & AllowedAmount;
	ar & built;
}

template<class Archive>
void Construction::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
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
		ar & dismantle;
		ar & time;
		ar & AllowedAmount;
		ar & built;
	}
}

//
// class Door
//
BOOST_CLASS_VERSION(Door, 0)

template<class Archive>
void Door::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object <Construction>(*this);
	ar & closedGraphic;
}

template<class Archive>
void Door::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & boost::serialization::base_object<Construction>(*this);
		ar & closedGraphic;
	}
}

//
// class WaterNode
//
BOOST_CLASS_VERSION(WaterNode, 0)

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
	if (version == 0) {
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
}

//
// class FilthNode
//
BOOST_CLASS_VERSION(FilthNode, 0)

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
	if (version == 0) {
		ar & x;
		ar & y;
		ar & depth;
		ar & graphic;
		ar & color.r;
		ar & color.g;
		ar & color.b;
	}
}

//
// class BloodNode
//
BOOST_CLASS_VERSION(BloodNode, 0)

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
	if (version == 0) {
		ar & x;
		ar & y;
		ar & depth;
		ar & graphic;
		ar & color.r;
		ar & color.g;
		ar & color.b;
	}
}

//
// class NatureObject
//
BOOST_CLASS_VERSION(NatureObject, 0)

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
	if (version == 0) {
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
}

//
// class JobManager
//
BOOST_CLASS_VERSION(JobManager, 0)

template<class Archive>
void JobManager::save(Archive & ar, const unsigned int version) const {
	ar & availableList;
	ar & waitingList;
	ar & menialNPCsWaiting;
	ar & expertNPCsWaiting;
}

template<class Archive>
void JobManager::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & availableList;
		ar & waitingList;
		ar & menialNPCsWaiting;
		ar & expertNPCsWaiting;
	}
}

//
// class Camp
//
BOOST_CLASS_VERSION(Camp, 0)

template<class Archive>
void Camp::save(Archive & ar, const unsigned int version) const {
	ar.template register_type<Coordinate>();
	ar & centerX;
	ar & centerY;
	ar & buildingCount;
	ar & locked;
	ar & lockedCenter;
	ar & tier;
	ar & name;
}

template<class Archive>
void Camp::load(Archive & ar, const unsigned int version) {
	ar.template register_type<Container>();
	if (version == 0) {
		ar & centerX;
		ar & centerY;
		ar & buildingCount;
		ar & locked;
		ar & lockedCenter;
		ar & tier;
		ar & name;
	}
}

//
// class StockManager
//
BOOST_CLASS_VERSION(StockManager, 0)

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
	ar & designatedBog;
	ar & bogIronJobs;
}

template<class Archive>
void StockManager::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
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
		ar & designatedBog;
		ar & bogIronJobs;
	}
}

//
// class Map
//
BOOST_CLASS_VERSION(Map, 0)

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
	if (version == 0) {
		for (int x = 0; x < tileMap.size(); ++x) {
			for (int y = 0; y < tileMap[x].size(); ++y) {
				ar & tileMap[x][y];
			}
		}
		ar & width;
		ar & height;
	}
}

//
// class Tile
//
BOOST_CLASS_VERSION(Tile, 0)

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
}

template<class Archive>
void Tile::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
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
	}
}

//
// class FarmPlot
//
BOOST_CLASS_VERSION(FarmPlot, 0)

template<class Archive>
void FarmPlot::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}

template<class Archive>
void FarmPlot::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & boost::serialization::base_object<Stockpile>(*this);
		ar & tilled;
		ar & allowedSeeds;
		ar & growth;
	}
}

//
// class Attack
//
BOOST_CLASS_VERSION(Attack, 0)

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
	ar & projectile;
}

template<class Archive>
void Attack::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & damageType;
		ar & damageAmount.addsub;
		ar & damageAmount.multiplier;
		ar & damageAmount.nb_dices;
		ar & damageAmount.nb_faces;
		ar & cooldown;
		ar & cooldownMax;
		ar & statusEffects;
		ar & projectile;
	}
}

//
// class SkillSet
//
BOOST_CLASS_VERSION(SkillSet, 0)

template<class Archive>
void SkillSet::save(Archive & ar, const unsigned int version) const {
	ar & skills;
}

template<class Archive>
void SkillSet::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & skills;
	}
}

//
// class SpawningPool
//
BOOST_CLASS_VERSION(SpawningPool, 0)

	template<class Archive>
void SpawningPool::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Construction>(*this);
	ar & dumpFilth;
	ar & dumpCorpses;
	ar & a;
	ar & b;
	ar & expansion;
	ar & filth;
	ar & corpses;
	ar & spawns;
	ar & corpseContainer;
}

template<class Archive>
void SpawningPool::load(Archive & ar, const unsigned int version) {
	if (version == 0) {
		ar & boost::serialization::base_object<Construction>(*this);
		ar & dumpFilth;
		ar & dumpCorpses;
		ar & a;
		ar & b;
		ar & expansion;
		ar & filth;
		ar & corpses;
		ar & spawns;
		ar & corpseContainer;
	}
}

//
// Save/load entry points
//

// XXX: Relying on the little-endianess of the platform. Are we supporting anything other than x86/x64?
// WARNING: Wild templates ahead.
namespace {
	// These exist to determine two smaller types that can compose a bigger type.
	// N is type size in bytes.
	template <size_t N> struct type { };
	
	#define DEFINE_TYPE(T) template <> struct type<sizeof(T)> { typedef T uint; }
	DEFINE_TYPE(boost::uint8_t);
	DEFINE_TYPE(boost::uint16_t);
	DEFINE_TYPE(boost::uint32_t);
	DEFINE_TYPE(boost::uint64_t);
	#undef DEFINE_TYPE
	
	// ReadUInt<boost::uint64_t> calls ReadUInt<boost::uint32_t> and ReadUInt<boost::uint32_t>
	// ReadUInt<boost::uint32_t> calls ReadUInt<boost::uint16_t> and ReadUInt<boost::uint16_t>
	// ReadUInt<boost::uint16_t> calls ReadUInt<boost::uint8_t>  and ReadUInt<boost::uint8_t>
	// ReadUInt<boost::uint8_t> reads single byte from the stream
	//
	// The result is then bitshifted and ORed to reconstruct the value.
	
	template <typename T>
	T ReadUInt(std::ifstream& stream) {
		typedef typename type<sizeof(T) / 2>::uint smaller;
		
		const boost::uint32_t smallerBits = sizeof(smaller) * 8;
		
		smaller a, b;
		a = ReadUInt<smaller>(stream);
		b = ReadUInt<smaller>(stream);
		
		return ((T)a << smallerBits) | (T)b;
	}
	
	template <>
	boost::uint8_t ReadUInt<boost::uint8_t>(std::ifstream& stream) {
		return (boost::uint8_t)stream.get();
	}
	
	// WriteUInt is a recursive call, just like ReadUInt.
	
	template <typename T>
	void WriteUInt(std::ofstream& stream, typename type<sizeof(T)>::uint value) {
		typedef typename type<sizeof(T) / 2>::uint smaller;
		
		const boost::uint32_t smallerBits = sizeof(smaller) * 8;
		// All types here are unsigned.
		const smaller maxValue = (smaller)-1;
		
		WriteUInt<smaller>(stream, (value >> smallerBits) & maxValue);
		WriteUInt<smaller>(stream, value & maxValue);
	}
	
	template <>
	void WriteUInt<boost::uint8_t>(std::ofstream& stream, boost::uint8_t value) {
		stream.put((char)value);
	}
}

bool Game::SaveGame(const std::string& filename) {
	try {
		std::ofstream ofs(filename.c_str(), std::ios::binary);
		
		// Write the file header
		WriteUInt<boost::uint32_t>(ofs, saveMagicConst);
		WriteUInt<boost::uint8_t> (ofs, fileFormatConst);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		
		// Write the payload
		boost::archive::binary_oarchive oarch(ofs);
		oarch << *instance;
		oarch << *JobManager::Inst();
		oarch << *Camp::Inst();
		oarch << *StockManager::Inst();
		oarch << *Map::Inst();
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to save the game: " << e.what());
		return false;
	}
}

bool Game::LoadGame(const std::string& filename) {
	try {
		std::ifstream ifs(filename.c_str(), std::ios::binary);
		
		// Read and verify the file header
		if (ReadUInt<boost::uint32_t>(ifs) != saveMagicConst) {
			throw std::exception("Invalid magic value.");
		}
		
		if (ReadUInt<boost::uint8_t>(ifs) != fileFormatConst) {
			throw std::exception("Invalid file format value.");
		}
		
		// reserved values
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		
		Game::Inst()->Reset();
		Game::Inst()->LoadingScreen();
		
		// Read the payload
		boost::archive::binary_iarchive iarch(ifs);
		iarch >> *instance;
		iarch >> *JobManager::Inst();
		iarch >> *Camp::Inst();
		iarch >> *StockManager::Inst();
		iarch >> *Map::Inst();
		
		Game::Inst()->TranslateContainerListeners();
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to load the game: " << e.what());
		return false;
	}
}

#pragma warning(pop)
