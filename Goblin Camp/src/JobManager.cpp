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
		availableList[WAITING].push_back(newJob);
	}
}

void JobManager::CancelJob(boost::weak_ptr<Job> oldJob, std::string msg, TaskResult result) {
	if (boost::shared_ptr<Job> job = oldJob.lock()) {
		/*        if (job->priority() > LOW)
		Announce::Inst()->AddMsg(oldJob.lock()->name+std::string(" canceled: ")+msg, TCODColor::red);
		*/
		job->Assign(-1);
		job->Paused(true);
		availableList[WAITING].push_back(job);

		for(std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[job->priority()].begin(); 
			jobi != availableList[job->priority()].end(); ++jobi) {
				if ((*jobi) == job) {
					availableList[job->priority()].erase(jobi);
					break;
				}
		}
	}
}

void JobManager::Draw(Coordinate pos, int from, int width, int height, TCODConsole* console) {
	int skip = 0;
	int y = pos.Y();
	std::map<int,boost::shared_ptr<NPC> >::iterator npc;
	TCODColor color_mappings[] = { TCODColor::green, TCODColor::yellow, TCODColor::red, TCODColor::grey };

	for (int i=0; i<PRIORITY_COUNT; i++) {
		console->setForegroundColor(color_mappings[i]);
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin(); jobi != availableList[i].end(); ++jobi) {
			if (skip < from) ++skip;
			else {
				npc = Game::Inst()->npcList.find((*jobi)->Assigned());
				if (npc != Game::Inst()->npcList.end()) { 
					console->print(pos.X(), y, "%c", npc->second->GetNPCSymbol());
				}
				console->print(pos.X() + 2, y, "%s", (*jobi)->name.c_str());

#if DEBUG
				if (npc != Game::Inst()->npcList.end()) {
					if (npc->second->currentTask() != 0) {
						console->print(pos.X() + 45, y, "%s", (*jobi)->ActionToString(npc->second->currentTask()->action).c_str());
					}
				}
				console->print(pos.X() + width - 11, y, "A-> %d", (*jobi)->Assigned());
#endif
				if (++y - pos.Y() >= height) {
					console->setForegroundColor(TCODColor::white);
					return;
				}
			}
		}
	}
	console->setForegroundColor(TCODColor::white);
}

boost::weak_ptr<Job> JobManager::GetJob(int uid) {
	boost::weak_ptr<Job> job;

	for (int i = 0; i < PRIORITY_COUNT - 1; ++i) {
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
	for (std::list<boost::shared_ptr<Job> >::iterator jobIter = availableList[WAITING].begin(); jobIter != availableList[WAITING].end(); ) {

		if ((*jobIter)->Removable()) {
			jobIter = availableList[WAITING].erase(jobIter);
		} else {

			if (!(*jobIter)->PreReqs()->empty() && (*jobIter)->PreReqsCompleted()) {
				(*jobIter)->Paused(false);
			}

			if (!(*jobIter)->Paused()) {
				AddJob(*jobIter);
				jobIter = availableList[WAITING].erase(jobIter);
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
	for (int i = 0; i < PRIORITY_COUNT - 1; ++i) {
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
	return count;
}

boost::weak_ptr<Job> JobManager::GetJobByListIndex(int index) {
	int count = 0;

	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin(); jobi != availableList[i].end(); ++jobi) {
			if (count++ == index) return (*jobi);
		}
	}

	return boost::weak_ptr<Job>();
}

void JobManager::Reset() {
	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		availableList[i].clear();
	}
}
