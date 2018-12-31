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
#include<memory>

#include "Job.hpp"
#include "data/Serialization.hpp"

class JobManager {
	GC_SERIALIZABLE_CLASS
	
	JobManager();
	static JobManager *instance;
	std::list<std::shared_ptr<Job> > availableList[PRIORITY_COUNT];
	std::list<std::shared_ptr<Job> > waitingList;
	std::vector<int> menialNPCsWaiting;
	std::vector<int> expertNPCsWaiting;
	std::vector<std::vector<std::weak_ptr<Job> > > toolJobs;
	std::list<std::shared_ptr<Job> > failList;
public:
	static JobManager* Inst();
	static void Reset();
	void AddJob(std::shared_ptr<Job>);
	void Draw(Coordinate, int from = 0, int width = 40 ,int height = 40, TCODConsole* = TCODConsole::root);
	void CancelJob(std::weak_ptr<Job>, std::string, TaskResult);
	std::weak_ptr<Job> GetJob(int);
	std::weak_ptr<Job> GetJobByListIndex(int);
	void RemoveJob(std::weak_ptr<Job>);
	void RemoveJob(Action, Coordinate); //Can remove more than was intended, use with caution
	void Update();
	int JobAmount();
	void NPCWaiting(int);
	void NPCNotWaiting(int);
	void ClearWaitingNpcs();
	void AssignJobs();
};

BOOST_CLASS_VERSION(JobManager, 1)
