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

#include "Job.hpp"

class JobManager {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()
	JobManager();
	static JobManager *instance;
	std::list<boost::shared_ptr<Job> > availableList[PRIORITY_COUNT];
	std::vector<int> npcsWaiting;
public:
	static JobManager* Inst();
	void Reset();
	void AddJob(boost::shared_ptr<Job>);
	void Draw(Coordinate, int from = 0, int width = 40 ,int height = 40, TCODConsole* = TCODConsole::root);
	void CancelJob(boost::weak_ptr<Job>, std::string, TaskResult);
	void CancelJob(boost::weak_ptr<Entity>);
	boost::weak_ptr<Job> GetJob(int);
	boost::weak_ptr<Job> GetJobByListIndex(int);
	void RemoveJobByNPC(int uid);
	void Update();
	int JobAmount();
	void NPCWaiting(int);
	void ClearWaitingNpcs();
	void AssignJobs();
};
