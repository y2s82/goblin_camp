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

#ifdef DEBUG
#include<iostream>
#endif

#include <string>
#include <libtcod.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>

#include "Job.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "GCamp.hpp"
#include "MapMarker.hpp"
#include "Stockpile.hpp"
#include "Door.hpp"
#include "Farmplot.hpp"

Task::Task(Action act, Coordinate tar, boost::weak_ptr<Entity> ent, ItemCategory itt, int fla) :
	target(tar),
	entity(ent),
	action(act),
	item(itt),
	flags(fla)
{
}

void Task::save(OutputArchive& ar, const unsigned int version) const {
	ar & target;
	ar & entity;
	ar & action;
	ar & item;
	ar & flags;
}

void Task::load(InputArchive& ar, const unsigned int version) {
	ar & target;
	ar & entity;
	ar & action;
	ar & item;
	ar & flags;
}

Job::Job(std::string value, JobPriority pri, int z, bool m) :
	_priority(pri),
	completion(ONGOING),
	parent(boost::weak_ptr<Job>()),
	npcUid(-1),
	_zone(z),
	menial(m),
	paused(false),
	waitingForRemoval(false),
	reservedEntities(std::list<boost::weak_ptr<Entity> >()),
	reservedSpot(boost::tuple<boost::weak_ptr<Stockpile>, Coordinate, ItemType>(boost::weak_ptr<Stockpile>(), zero, -1)),
	attempts(0),
	attemptMax(5),
	connectedEntity(boost::weak_ptr<Entity>()),
	reservedContainer(boost::weak_ptr<Container>()),
	reservedSpace(0),
	tool(-1),
	markedGround(undefined),
	obeyTerritory(true),
	fireAllowed(false),
	name(value),
	tasks(std::vector<Task>()),
	internal(false)
{
}

Job::~Job() {
	preReqs.clear();
	UnreserveEntities();
	UnreserveSpot();
	if (connectedEntity.lock()) connectedEntity.lock()->CancelJob();
	if (reservedContainer.lock()) {
		reservedContainer.lock()->ReserveSpace(false, reservedSpace);
	}
	if (Map::Inst()->IsInside(markedGround)) {
			Map::Inst()->Unmark(markedGround);
	}
	for (std::list<int>::iterator marki = mapMarkers.begin(); marki != mapMarkers.end(); ++marki) {
		Map::Inst()->RemoveMarker(*marki);
	}
	mapMarkers.clear();
}

void Job::priority(JobPriority value) { _priority = value; }
JobPriority Job::priority() { return _priority; }

bool Job::Completed() {return (completion == SUCCESS || completion == FAILURE);}
void Job::Complete() {completion = SUCCESS;}
std::list<boost::weak_ptr<Job> >* Job::PreReqs() {return &preReqs;}
boost::weak_ptr<Job> Job::Parent() {return parent;}
void Job::Parent(boost::weak_ptr<Job> value) {parent = value;}
void Job::Assign(int uid) {npcUid = uid;}
int Job::Assigned() {return npcUid;}
void Job::zone(int value) {_zone = value;}
int Job::zone() {return _zone;}
bool Job::Menial() {return menial;}
bool Job::Paused() {return paused;}
void Job::Paused(bool value) {paused = value;}
void Job::Remove() {waitingForRemoval = true;}
bool Job::Removable() {return waitingForRemoval && PreReqsCompleted();}
int Job::Attempts() {return attempts;}
void Job::Attempts(int value) {attemptMax = value;}
bool Job::Attempt() {
	if (++attempts > attemptMax) return false;
	return true;
}

bool Job::PreReqsCompleted() {
	for (std::list<boost::weak_ptr<Job> >::iterator preReqIter = preReqs.begin(); preReqIter != preReqs.end(); ++preReqIter) {
		if (preReqIter->lock() && !preReqIter->lock()->Completed()) return false;
	}
	return true;
}

bool Job::ParentCompleted() {
	if (!parent.lock()) return true;
	return parent.lock()->Completed();
}

void Job::ReserveEntity(boost::weak_ptr<Entity> entity) {
	if (entity.lock()) {
		reservedEntities.push_back(entity);
		entity.lock()->Reserve(true);
	}
}

void Job::UnreserveEntities() {
	for (std::list<boost::weak_ptr<Entity> >::iterator itemI = reservedEntities.begin(); itemI != reservedEntities.end(); ++itemI) {
		if (itemI->lock()) itemI->lock()->Reserve(false);
	}
	reservedEntities.clear();
}

void Job::ReserveSpot(boost::weak_ptr<Stockpile> sp, Coordinate pos, ItemType type) {
	if (sp.lock()) {
		sp.lock()->ReserveSpot(pos, true, type);
		reservedSpot = boost::tuple<boost::weak_ptr<Stockpile>, Coordinate, ItemType>(sp, pos, type);
	}
}

void Job::UnreserveSpot() {
	if (reservedSpot.get<0>().lock()) {
		reservedSpot.get<0>().lock()->ReserveSpot(reservedSpot.get<1>(), false, reservedSpot.get<2>());
		reservedSpot.get<0>().reset();
	}
}

void Job::Fail() {
	completion = FAILURE;
	if (parent.lock()) parent.lock()->Fail();
	Remove();
}

std::string Job::ActionToString(Action action) {
	switch (action) {
		case NOACTION: return std::string("No Action");
		case USE: return std::string("Use");
		case TAKE: return std::string("Pick up");
		case DROP: return std::string("Drop");
		case PUTIN: return std::string("Put in");
		case BUILD: return std::string("Build");
		case MOVE: return std::string("Move");
		case MOVEADJACENT: return std::string("Move adjacent");
		case MOVENEAR: return std::string("Move Near");
		case WAIT: return std::string("Wait");
		case DRINK: return std::string("Drink");
		case EAT: return std::string("Eat");
		case FIND: return std::string("Find");
		case HARVEST: return std::string("Harvest");
		case FELL: return std::string("Fell");
		case HARVESTWILDPLANT: return std::string("Harvest plant");
		case KILL: return std::string("Kill");	
		case FLEEMAP: return std::string("Flee!!");
		case SLEEP: return std::string("Sleep");
		case DISMANTLE: return std::string("Dismantle");
		case WIELD: return std::string("Wield");
		case BOGIRON: return std::string("Collect bog iron");
		case STOCKPILEITEM: return std::string("Stockpile item");
		case QUIVER: return std::string("Quiver");
		case FILL: return std::string("Fill");
		case POUR: return std::string("Pour");
		case DIG: return std::string("Dig");
		case FORGET: return std::string("Huh?");
		default: return std::string("???");
	}
}

void Job::ConnectToEntity(boost::weak_ptr<Entity> ent) {
	connectedEntity = ent;
}

void Job::ReserveSpace(boost::weak_ptr<Container> cont, int bulk) {
	if (cont.lock()) {
		cont.lock()->ReserveSpace(true, bulk);
		reservedContainer = cont;
		reservedSpace = bulk;
	}
}

boost::weak_ptr<Entity> Job::ConnectedEntity() { return connectedEntity; }

bool Job::RequiresTool() { return tool != -1; }

void Job::SetRequiredTool(ItemCategory item) { tool = item; }
ItemCategory Job::GetRequiredTool() { return tool; }

void Job::MarkGround(Coordinate ground) {
	if (Map::Inst()->IsInside(ground)) {
			markedGround = ground;
			Map::Inst()->Mark(ground);
	}
}

void Job::DisregardTerritory() { obeyTerritory = false; }

bool Job::OutsideTerritory() {
	if (obeyTerritory) {
		for (std::vector<Task>::iterator task = tasks.begin(); task != tasks.end(); ++task) {
			Coordinate coord = task->target;
			if (!Map::Inst()->IsInside(coord)) {
				if (task->entity.lock()) {
					coord = task->entity.lock()->Position();
				}
			}

			if (Map::Inst()->IsInside(coord))
				if (!Map::Inst()->IsTerritory(coord))
					return true;
		}
	}
	return false;
}

void Job::AddMapMarker(MapMarker marker) {
	mapMarkers.push_back(Map::Inst()->AddMarker(marker));
}

void Job::AllowFire() { fireAllowed = true; }
bool Job::InvalidFireAllowance() {
	if (!fireAllowed) {
		for (std::vector<Task>::iterator task = tasks.begin(); task != tasks.end(); ++task) {
			Coordinate coord = task->target;
			if (!Map::Inst()->IsInside(coord)) {
				if (task->entity.lock()) {
					coord = task->entity.lock()->Position();
				}
			}

			if (Map::Inst()->IsInside(coord)) {
				if (Map::Inst()->GetFire(coord).lock()) return true;
			}
		}
	}
	return false;
}

void Job::CreatePourWaterJob(boost::shared_ptr<Job> job, Coordinate location) {
	job->Attempts(1);

	//First search for a container containing water
	boost::shared_ptr<Item> waterItem = Game::Inst()->FindItemByTypeFromStockpiles(Item::StringToItemType("Water"),
		location).lock();
	Coordinate waterLocation = Game::Inst()->FindWater(location);

	//If a water item exists, is closer and contained then use that
	bool waterContainerFound = false;
	if (waterItem) {
		int distanceToWater = std::numeric_limits<int>::max();
		if (waterLocation != undefined) distanceToWater = Distance(location, waterLocation);
		int distanceToItem = Distance(location, waterItem->Position());

		if (distanceToItem < distanceToWater && waterItem->ContainedIn().lock() && 
			waterItem->ContainedIn().lock()->IsCategory(Item::StringToItemCategory("Container"))) {
				boost::shared_ptr<Container> container = boost::static_pointer_cast<Container>(waterItem->ContainedIn().lock());
				//Reserve everything inside the container
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); 
					itemi != container->end(); ++itemi) {
						job->ReserveEntity(*itemi);
				}
				job->ReserveEntity(container);
				job->tasks.push_back(Task(MOVE, container->Position()));
				job->tasks.push_back(Task(TAKE, container->Position(), container));
				waterContainerFound = true;
		}
	}

	if (!waterContainerFound && waterLocation != undefined) {
		job->SetRequiredTool(Item::StringToItemCategory("Bucket"));
		job->tasks.push_back(Task(MOVEADJACENT, waterLocation));
		job->tasks.push_back(Task(FILL, waterLocation));
	}

	if (waterContainerFound || waterLocation != undefined) {
		job->tasks.push_back(Task(MOVEADJACENT, location));
		job->tasks.push_back(Task(POUR, location));
		if (waterContainerFound) job->tasks.push_back(Task(STOCKPILEITEM));
		job->DisregardTerritory();
		job->AllowFire();
		job->statusEffects.push_back(BRAVE);
	} else {
		job.reset();
	}
}

void Job::save(OutputArchive& ar, const unsigned int version) const {
	ar.register_type<Container>();
	ar.register_type<Item>();
	ar.register_type<Entity>();
	ar.register_type<NatureObject>();
	ar.register_type<Construction>();
	ar.register_type<Door>();
	ar.register_type<FarmPlot>();
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
	ar & reservedSpot.get<0>();
	ar & reservedSpot.get<1>();
	ar & reservedSpot.get<2>();
	ar & attempts;
	ar & attemptMax;
	ar & connectedEntity;
	ar & reservedContainer;
	ar & reservedSpace;
	ar & tool;
	ar & name;
	ar & tasks;
	ar & internal;
	ar & markedGround;
	ar & obeyTerritory;
	ar & statusEffects;
}

void Job::load(InputArchive& ar, const unsigned int version) {
	ar.register_type<Container>();
	ar.register_type<Item>();
	ar.register_type<Entity>();
	ar.register_type<NatureObject>();
	ar.register_type<Construction>();
	ar.register_type<Door>();
	ar.register_type<FarmPlot>();
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
	boost::weak_ptr<Stockpile> sp;
	ar & sp;
	Coordinate location;
	ar & location;
	ItemType type;
	ar & type;
	reservedSpot = boost::tuple<boost::weak_ptr<Stockpile>, Coordinate, ItemType>(sp, location, type);
	ar & attempts;
	ar & attemptMax;
	ar & connectedEntity;
	ar & reservedContainer;
	ar & reservedSpace;
	ar & tool;
	ar & name;
	ar & tasks;
	ar & internal;
	ar & markedGround;
	ar & obeyTerritory;
	if (version >= 1) {
		ar & statusEffects;
	}
}

