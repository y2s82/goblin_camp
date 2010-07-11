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

#include <string>
#include <libtcod.hpp>

#include "Job.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "GCamp.hpp"

Task::Task(Action act, Coordinate tar, boost::weak_ptr<Entity> ent, ItemCategory itt) :
	target(tar),
	entity(ent),
	action(act),
	item(itt)
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
	reservedSpot(std::pair<boost::weak_ptr<Stockpile>, Coordinate>(boost::weak_ptr<Stockpile>(), Coordinate(0,0))),
	attempts(0),
	attemptMax(5),
	name(value),
	internal(false)
{
}

Job::~Job() {
	preReqs.clear();
	UnreserveEntities();
	UnreserveSpot();
	if (connectedEntity.lock()) connectedEntity.lock()->CancelJob();
	if (reservedSpace.lock()) {
	    reservedSpace.lock()->ReserveSpace(false);
	    reservedSpace = boost::weak_ptr<Container>();
	}
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
bool Job::Removable() {return waitingForRemoval;}
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

void Job::ReserveSpot(boost::weak_ptr<Stockpile> sp, Coordinate pos) {
    if (sp.lock()) {
        sp.lock()->ReserveSpot(pos, true);
        reservedSpot = std::pair<boost::weak_ptr<Stockpile>, Coordinate>(sp, pos);
    }
}

void Job::UnreserveSpot() {
    if (reservedSpot.first.lock()) {
        reservedSpot.first.lock()->ReserveSpot(reservedSpot.second, false);
        reservedSpot.first.reset();
    }
}

void Job::Fail() {
    completion = FAILURE;
	if (parent.lock()) parent.lock()->Fail();
    preReqs.clear();
	Remove();
}

std::string Job::ActionToString(Action action) {
    switch (action) {
        case MOVE: return std::string("Move");
        case MOVEADJACENT: return std::string("Move adjacent");
        case BUILD: return std::string("Build");
        case TAKE: return std::string("Pick up");
        case DROP: return std::string("Drop");
        case PUTIN: return std::string("Put in");
        case USE: return std::string("Use");
        default: return std::string("???");
    }
}

void Job::ConnectToEntity(boost::weak_ptr<Entity> ent) {
    connectedEntity = ent;
}

void Job::ReserveSpace(boost::weak_ptr<Container> cont) {
    cont.lock()->ReserveSpace(true);
    reservedSpace = cont;
}
