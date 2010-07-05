#pragma once

#include "Job.hpp"

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
