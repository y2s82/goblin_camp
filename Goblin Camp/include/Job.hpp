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

#include <queue>
#include <string>
#include <list>

#include <boost/serialization/serialization.hpp>

#include "Item.hpp"
#include "Construction.hpp"
#include "Stockpile.hpp"
#include "Coordinate.hpp"

enum JobPriority {
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
	QUIVER
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
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
	Task(Action = NOACTION, Coordinate = Coordinate(0,0), boost::weak_ptr<Entity> = boost::weak_ptr<Entity>(), ItemCategory = 0, int flags = 0);
	Coordinate target;
	boost::weak_ptr<Entity> entity;
	Action action;
	ItemCategory item;
	int flags;
};

class Job {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	JobPriority _priority;
	JobCompletion completion;
	std::list<boost::weak_ptr<Job> > preReqs;
	boost::weak_ptr<Job> parent;
	int npcUid;
	int _zone;
	bool menial;
	bool paused;
	bool waitingForRemoval;
	std::list<boost::weak_ptr<Entity> > reservedEntities;
	std::pair<boost::weak_ptr<Stockpile>, Coordinate> reservedSpot;
	int attempts, attemptMax;
	boost::weak_ptr<Entity> connectedEntity;
	boost::weak_ptr<Container> reservedSpace;
public:
	static boost::shared_ptr<Job> MoveJob(Coordinate);
	static boost::shared_ptr<Job> BuildJob(boost::weak_ptr<Construction>);

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
	std::list<boost::weak_ptr<Job> >* PreReqs();
	boost::weak_ptr<Job> Parent();
	void Parent(boost::weak_ptr<Job>);
	void Assign(int);
	int Assigned();
	void zone(int);
	int zone();
	bool Menial();
	bool Paused();
	void Paused(bool);
	void Remove();
	bool Removable();

	void ReserveEntity(boost::weak_ptr<Entity>);
	void UnreserveEntities();
	void ReserveSpot(boost::weak_ptr<Stockpile>, Coordinate);
	void UnreserveSpot();
	void ConnectToEntity(boost::weak_ptr<Entity>);
	void ReserveSpace(boost::weak_ptr<Container>);

	bool internal;
	int Attempts();
	void Attempts(int);
	bool Attempt();

	static std::string ActionToString(Action);
};
