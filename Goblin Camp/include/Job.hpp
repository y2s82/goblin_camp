#pragma once

#include <queue>
#include <string>
#include <list>

#include "Item.hpp"
#include "Construction.hpp"
#include "Stockpile.hpp"
#include "Coordinate.hpp"

enum JobPriority {
	LOW,
	MED,
	HIGH,
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
	KILL
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
	private:
	public:
		Task(Action = NOACTION, Coordinate = Coordinate(0,0), boost::weak_ptr<Entity> = boost::weak_ptr<Entity>(), ItemCategory = 0);
		Coordinate target;
		boost::weak_ptr<Entity> entity;
		Action action;
		ItemCategory item;
};

class Job {
	private:
		JobPriority _priority;
		JobCompletion completion;
		std::list<boost::weak_ptr<Job> > preReqs;
		boost::weak_ptr<Job> parent;
		int npcUid;
		int _zone;
		bool menial;
		bool paused;
		bool waitingForRemoval;
		std::list<boost::weak_ptr<Item> > reservedItems;
		std::pair<boost::weak_ptr<Stockpile>, Coordinate> reservedSpot;
		int attempts;
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

		void ReserveItem(boost::weak_ptr<Item>);
		void UnreserveItems();
		void ReserveSpot(boost::weak_ptr<Stockpile>, Coordinate);
		void UnreserveSpot();
		void ConnectToEntity(boost::weak_ptr<Entity>);
		void ReserveSpace(boost::weak_ptr<Container>);

		bool internal;
		int Attempts();
		void Attempt();

		static std::string ActionToString(Action);
};

class JobManager {
	private:
		JobManager();
		static JobManager *instance;
		std::list<boost::shared_ptr<Job> > availableList[PRIORITY_COUNT];
		std::list<boost::shared_ptr<Job> > waitingList;
	public:
		static JobManager* Inst();
		void AddJob(boost::shared_ptr<Job>);
		void Draw(Coordinate, int from = 0, int count = 20, TCODConsole* = TCODConsole::root);
		void CancelJob(boost::weak_ptr<Job>, std::string, TaskResult);
		boost::weak_ptr<Job> GetJob(int);
		boost::weak_ptr<Job> GetJobByListIndex(int);
		void Update();
		int JobAmount();

};
