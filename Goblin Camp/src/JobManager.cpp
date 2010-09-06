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
#include "stdafx.hpp"

#include <list>

#include <boost/shared_ptr.hpp>
#include <libtcod.hpp>

#include "JobManager.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "KuhnMunkres.hpp"

JobManager::JobManager() {}
JobManager *JobManager::instance = 0;

JobManager *JobManager::Inst() {
	if (!instance) instance = new JobManager();
	return instance;
}

void JobManager::AddJob(boost::shared_ptr<Job> newJob) {
	if (!newJob->Attempt()) {
		newJob->Fail();
		return;
	}

	if (newJob->PreReqsCompleted()) {
		availableList[newJob->priority()].push_back(newJob);
		return;
	} else {
		newJob->Paused(true);
		waitingList.push_back(newJob);
	}
}

void JobManager::CancelJob(boost::weak_ptr<Job> oldJob, std::string msg, TaskResult result) {
	if (boost::shared_ptr<Job> job = oldJob.lock()) {
		/*        if (job->priority() > LOW)
		Announce::Inst()->AddMsg(oldJob.lock()->name+std::string(" canceled: ")+msg, TCODColor::red);
		*/
		job->Assign(-1);
		job->Paused(true);
		waitingList.push_back(job);

		for(std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[job->priority()].begin(); 
			jobi != availableList[job->priority()].end(); ++jobi) {
				if ((*jobi) == job) {
					availableList[job->priority()].erase(jobi);
					break;
				}
		}
	}
}

//Draw() doesn't conform to the design, it'll only draw jobs from the 4 hardcoded lists
//TODO: Make Draw prioritycount agnostic
//Take into account though that right now this is a debug only view
void JobManager::Draw(Coordinate pos, int from, int count, TCODConsole* console) {
	int skip = 0;
	int y = pos.Y();

	console->setForegroundColor(TCODColor::lightCyan);
	for (std::list<boost::shared_ptr<Job> >::iterator highIter = availableList[HIGH].begin(); highIter != availableList[HIGH].end(); ++highIter) {
		if (skip < from) ++skip;
		else {

			console->print(pos.X(), y, "%s", (*highIter)->name.c_str());
			if (++y - pos.Y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::white);
	for (std::list<boost::shared_ptr<Job> >::iterator medIter = availableList[MED].begin(); medIter != availableList[MED].end(); ++medIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.X(), y, "%s", (*medIter)->name.c_str());
			if (++y - pos.Y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::lightGrey);
	for (std::list<boost::shared_ptr<Job> >::iterator lowIter = availableList[LOW].begin(); lowIter != availableList[LOW].end(); ++lowIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.X(), y, "%s", (*lowIter)->name.c_str());
			if (++y - pos.Y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::grey);
	for (std::list<boost::shared_ptr<Job> >::iterator waitIter = waitingList.begin(); waitIter != waitingList.end(); ++waitIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.X(), y, "%s", (*waitIter)->name.c_str());
			if (++y - pos.Y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::white);
}

boost::weak_ptr<Job> JobManager::GetJob(int uid) {
	boost::weak_ptr<Job> job;

	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
			jobi != availableList[i].end(); ++jobi) {
				if ((*jobi)->Menial() != Game::Inst()->npcList[uid]->Expert()) {
					if ((*jobi)->Assigned() == -1 && !(*jobi)->Removable()) {
						job = (*jobi);
						goto FoundJob;
					}
				}
		}
	}
FoundJob:
	if (job.lock()) job.lock()->Assign(uid);

	return job;
}

void JobManager::Update() {

	//Check the waiting list for jobs that have completed prerequisites, and move them to the
	//job queue if so, remove removables, and retry retryables, remove unpauseds
	for (std::list<boost::shared_ptr<Job> >::iterator jobIter = waitingList.begin(); jobIter != waitingList.end(); ) {

		if ((*jobIter)->Removable()) {
			jobIter = waitingList.erase(jobIter);
		} else {

			if (!(*jobIter)->PreReqs()->empty() && (*jobIter)->PreReqsCompleted()) {
				(*jobIter)->Paused(false);
			}

			if (!(*jobIter)->Paused()) {
				AddJob(*jobIter);
				jobIter = waitingList.erase(jobIter);
			} else {
				if (!(*jobIter)->Parent().lock() && !(*jobIter)->PreReqs()->empty()) {
					//Job has unfinished prereqs, itsn't removable and is NOT a prereq itself
					for (std::list<boost::weak_ptr<Job> >::iterator pri = (*jobIter)->PreReqs()->begin(); pri != (*jobIter)->PreReqs()->end(); ++pri) {
						if (pri->lock()) {
							pri->lock()->Paused(false);
						}
					}
				} else if (!(*jobIter)->Parent().lock()) {
					(*jobIter)->Paused(false);
				}
				++jobIter;
			}
		}
	}

	//Check the normal queues for jobs that have all prerequisites and parents completed and
	//remove them
	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
			jobi != availableList[i].end(); ) {
				if ((*jobi)->Completed() && (*jobi)->PreReqsCompleted()) {
					jobi = availableList[i].erase(jobi);
				} else {
					++jobi;
				}
		}
	}
}

int JobManager::JobAmount() { 
	int count = 0;
	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		count += availableList[i].size();
	}
	count += waitingList.size(); 
	return count;
}

boost::weak_ptr<Job> JobManager::GetJobByListIndex(int index) {
	int count = 0;

	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
			jobi != availableList[i].end(); ++jobi) {
				if (count++ == index) return (*jobi);
		}
	}

	for (std::list<boost::shared_ptr<Job> >::iterator waitingIter = waitingList.begin(); waitingIter != waitingList.end(); ++waitingIter) {
		if (count++ == index) return (*waitingIter);
	}

	return boost::weak_ptr<Job>();
}

void JobManager::Reset() {
	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		availableList[i].clear();
	}
	waitingList.clear();
	npcsWaiting.clear();
}

void JobManager::NPCWaiting(int uid) {
	for(std::vector<int>::iterator it = npcsWaiting.begin(); it != npcsWaiting.end(); it++) {
		if(*it == uid) {
			return;
		}
	}
	npcsWaiting.push_back(uid);
}

void JobManager::NPCNotWaiting(int uid) {
	for(std::vector<int>::iterator it = npcsWaiting.begin(); it != npcsWaiting.end(); it++) {
		if(*it == uid) {
			npcsWaiting.erase(it);
			return;
		}
	}
}

void JobManager::ClearWaitingNpcs() {
	npcsWaiting.clear();
}

void JobManager::AssignJobs() {
	for (int i = 0; i < PRIORITY_COUNT && !npcsWaiting.empty(); i++) {
		if(!availableList[i].empty()) {
			std::vector<boost::shared_ptr<Job> > jobsToAssign;
			for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
				 jobi != availableList[i].end(); ++jobi) {
				if ((*jobi)->Assigned() == -1 && !(*jobi)->Removable()) {
					jobsToAssign.push_back(*jobi);
				}
			}
			if(!jobsToAssign.empty()) {
				int matrixSize = std::max(jobsToAssign.size(), npcsWaiting.size());
				boost::numeric::ublas::matrix<int> m(matrixSize, matrixSize);
				for(int x = 0; x < matrixSize; x++) {
					for(int y = 0; y < matrixSize; y++) {
						if(x >= npcsWaiting.size() || y >= jobsToAssign.size()) {
							m(x, y) = 1;
						} else {
							boost::shared_ptr<Job> job = jobsToAssign[y];
							boost::shared_ptr<NPC> npc = Game::Inst()->npcList[npcsWaiting[x]];
							if(job->Menial() == npc->Expert() ||
							   job->tasks.empty() ||
							   (job->tasks[0].target.X() == 0 && job->tasks[0].target.Y() == 0)) {
								m(x, y) = 1;
							} else {
								m(x, y) = 10000 - Distance(job->tasks[0].target, npc->Position());
							}
						}
					}
				}
				
				std::vector<int> assignments = FindBestMatching(m);
				
				for(int i = 0, n = 0; n < npcsWaiting.size(); i++, n++) {
					int jobNum = assignments[i];
					if(jobNum < jobsToAssign.size()) {
						int npcNum = npcsWaiting[n];
						boost::shared_ptr<Job> job = jobsToAssign[jobNum];
						boost::shared_ptr<NPC> npc = Game::Inst()->npcList[npcNum];
						job->Assign(npcNum);
						npc->StartJob(job);
						npcsWaiting.erase(npcsWaiting.begin() + n);
						n--;
					}
				}
			}
		}
	}
}