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

#include <queue>
#include <string>
#include <list>

#include <boost/tuple/tuple.hpp>

#include "Construction.hpp"
#include "data/Serialization.hpp"

class Stockpile;
class Coordinate;
class MapMarker;
class Entity;
class Container;

typedef int ItemCategory;
typedef int ItemType;

enum JobPriority {
	VERYHIGH,
	HIGH,
	MED,
	LOW,
	PRIORITY_COUNT
};

enum JobCompletion {
	FAILURE,
	SUCCESS,
	ONGOING
};

enum Action {
	NOACTION,
	USE,
	TAKE,
	DROP,
	PUTIN,
	BUILD,
	MOVE,
	MOVEADJACENT,
	MOVENEAR,
	WAIT,
	DRINK,
	EAT,
	FIND,
	HARVEST,
	FELL,
	HARVESTWILDPLANT,
	KILL,
	FLEEMAP,
	SLEEP,
	DISMANTLE,
	WIELD,
	WEAR,
	BOGIRON,
	STOCKPILEITEM,
	QUIVER,
	FILL,
	POUR,
	DIG,
	FORGET,
	UNWIELD,
	GETANGRY,
	CALMDOWN,
	STARTFIRE,
	REPAIR,
	FILLDITCH
};

enum TaskResult {
	TASKSUCCESS,
	TASKFAILNONFATAL,
	TASKFAILFATAL,
	TASKCONTINUE,
	TASKOWNDONE,
	PATHEMPTY
};

class Task {
	GC_SERIALIZABLE_CLASS
public:
	Task(Action = NOACTION, Coordinate = Coordinate(-1,-1), std::weak_ptr<Entity> = std::weak_ptr<Entity>(), ItemCategory = 0, int flags = 0);
	Coordinate target;
	std::weak_ptr<Entity> entity;
	Action action;
	ItemCategory item;
	int flags;
};

BOOST_CLASS_VERSION(Task, 0)

class Job {
	GC_SERIALIZABLE_CLASS
	
	JobPriority _priority;
	JobCompletion completion;
	std::list<std::weak_ptr<Job> > preReqs;
	std::weak_ptr<Job> parent;
	int npcUid;
	int _zone;
	bool menial;
	bool paused;
	bool waitingForRemoval;
	std::list<std::weak_ptr<Entity> > reservedEntities;
	boost::tuple<std::weak_ptr<Stockpile>, Coordinate, ItemType> reservedSpot;
	int attempts, attemptMax;
	std::weak_ptr<Entity> connectedEntity;
	std::weak_ptr<Container> reservedContainer;
	int reservedSpace;
	ItemCategory tool;
	Coordinate markedGround;
	bool obeyTerritory;
	std::list<int> mapMarkers;
	bool fireAllowed;
public:
	Job(std::string = "NONAME JOB", JobPriority = MED, int zone = 0, bool menial = true);
	~Job();
	std::string name;
	std::vector<Task> tasks;
	void priority(JobPriority);
	JobPriority priority();
	bool Completed();
	void Complete();
	void Fail();
	bool PreReqsCompleted();
	bool ParentCompleted();
	std::list<std::weak_ptr<Job> >* PreReqs();
	std::weak_ptr<Job> Parent();
	void Parent(std::weak_ptr<Job>);
	void Assign(int);
	int Assigned();
	void zone(int);
	int zone();
	bool Menial();
	bool Paused();
	void Paused(bool);
	void Remove();
	bool Removable();

	void ReserveEntity(std::weak_ptr<Entity>);
	void UnreserveEntities();
	void ReserveSpot(std::weak_ptr<Stockpile>, Coordinate, ItemType);
	void UnreserveSpot();
	void ConnectToEntity(std::weak_ptr<Entity>);
	std::weak_ptr<Entity> ConnectedEntity();
	void ReserveSpace(std::weak_ptr<Container>, int bulk = 1);

	bool internal;
	int Attempts();
	void Attempts(int);
	bool Attempt();

	bool RequiresTool();
	void SetRequiredTool(ItemCategory);
	ItemCategory GetRequiredTool();

	void MarkGround(Coordinate);

	static std::string ActionToString(Action);
	void DisregardTerritory();
	bool OutsideTerritory();

	void AddMapMarker(MapMarker);

	void AllowFire();
	bool InvalidFireAllowance();

	std::list<StatusEffectType> statusEffects;
	
	static void CreatePourWaterJob(std::shared_ptr<Job>, Coordinate);
};

BOOST_CLASS_VERSION(Job, 1)
