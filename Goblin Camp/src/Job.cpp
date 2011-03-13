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

#include <string>
#include <libtcod.hpp>
#include <boost/algorithm/string.hpp>

#include "Job.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "GCamp.hpp"
#include "MapMarker.hpp"
#include "Stockpile.hpp"

Task::Task(Action act, Coordinate tar, boost::weak_ptr<Entity> ent, ItemCategory itt, int fla) :
	target(tar),
	entity(ent),
	action(act),
	item(itt),
	flags(fla)
{
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
	reservedSpot(boost::tuple<boost::weak_ptr<Stockpile>, Coordinate, ItemType>(boost::weak_ptr<Stockpile>(), Coordinate(0,0), -1)),
	attempts(0),
	attemptMax(5),
	connectedEntity(boost::weak_ptr<Entity>()),
	reservedContainer(boost::weak_ptr<Container>()),
	reservedSpace(0),
	tool(-1),
	markedGround(Coordinate(-1,-1)),
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
	if (markedGround.X() >= 0 && markedGround.X() < Map::Inst()->Width() &&
		markedGround.Y() >= 0 && markedGround.Y() < Map::Inst()->Height()) {
			Map::Inst()->Unmark(markedGround.X(), markedGround.Y());
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


boost::shared_ptr<Job> Job::MoveJob(Coordinate tar) {
	boost::shared_ptr<Job> moveJob(new Job("Move"));
	moveJob->tasks.push_back(Task(MOVE, tar));
	return moveJob;
}

boost::shared_ptr<Job> Job::BuildJob(boost::weak_ptr<Construction> construct) {
	boost::shared_ptr<Job> buildJob(new Job("Build"));
	buildJob->tasks.push_back(Task(MOVEADJACENT, Coordinate(construct.lock()->X(),construct.lock()->Y()), construct));
	buildJob->tasks.push_back(Task(BUILD, Coordinate(construct.lock()->X(),construct.lock()->Y()), construct));
	return buildJob;
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
	if (ground.X() >= 0 && ground.X() < Map::Inst()->Width() &&
		ground.Y() >= 0 && ground.Y() < Map::Inst()->Height()) {
			markedGround = ground;
			Map::Inst()->Mark(ground.X(), ground.Y());
	}
}

void Job::DisregardTerritory() { obeyTerritory = false; }

bool Job::OutsideTerritory() {
	if (obeyTerritory) {
		for (std::vector<Task>::iterator task = tasks.begin(); task != tasks.end(); ++task) {
			Coordinate coord = task->target;
			if (coord.X() < 0 || coord.X() >= Map::Inst()->Width() || coord.Y() < 0 || coord.Y() >= Map::Inst()->Height()) {
				if (task->entity.lock()) {
					coord = task->entity.lock()->Position();
				}
			}

			if (coord.X() >= 0 && coord.X() < Map::Inst()->Width() && coord.Y() >= 0 && coord.Y() < Map::Inst()->Height()) {
				if (!Map::Inst()->IsTerritory(coord.X(), coord.Y())) return true;
			}
		}
	}
	return false;
}

void Job::AddMapMarker(MapMarker marker) {
	mapMarkers.push_back(Map::Inst()->AddMarker(marker));
}

void Job::AllowFire() { fireAllowed = true; }
bool Job::FireAllowed() { return fireAllowed; }