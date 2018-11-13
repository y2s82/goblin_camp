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

#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "JobManager.hpp"
#include "Game.hpp"
#include "KuhnMunkres.hpp"
#include "StockManager.hpp"

JobManager::JobManager() {
	for (std::vector<ItemCat>::iterator i = Item::Categories.begin(); i != Item::Categories.end(); ++i) {
		toolJobs.push_back(std::vector<boost::weak_ptr<Job> >());
	}
}
JobManager *JobManager::instance = 0;

JobManager *JobManager::Inst() {
	if (!instance) instance = new JobManager();
	return instance;
}

void JobManager::AddJob(boost::shared_ptr<Job> newJob) {
	if (!newJob->Attempt() || newJob->OutsideTerritory() || newJob->InvalidFireAllowance()) {
		failList.push_back(newJob);
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
		job->Assign(-1);
		job->Paused(true);

		//Push job onto waiting list
		waitingList.push_back(job);

		//Remove job from availabe list
		for(std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[job->priority()].begin(); 
			jobi != availableList[job->priority()].end(); ++jobi) {
				if ((*jobi) == job) {
					availableList[job->priority()].erase(jobi);
					break;
				}
		}

		//If the job requires a tool, remove it from the toolJobs list
		if (job->RequiresTool()) {
			for (std::vector<boost::weak_ptr<Job> >::iterator jobi = toolJobs[job->GetRequiredTool()].begin(); 
				jobi != toolJobs[job->GetRequiredTool()].end(); ++jobi) {
				if (jobi->lock() == job) {
					toolJobs[job->GetRequiredTool()].erase(jobi);
					break;
				}
			}
		}
	}
}

void JobManager::Draw(Coordinate pos, int from, int width, int height, TCODConsole* console) {
	int skip = 0;
	int y = pos.Y();
	boost::shared_ptr<NPC> npc;
	TCODColor color_mappings[] = { TCODColor::green, TCODColor(0,160,60), TCODColor(175,150,50), TCODColor(165,95,0), TCODColor::grey };

	for (int i=0; i<=PRIORITY_COUNT; i++) {
		console->setDefaultForeground(color_mappings[i]);
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = (i < PRIORITY_COUNT ? availableList[i].begin() : waitingList.begin()); 
			jobi != (i < PRIORITY_COUNT ? availableList[i].end() : waitingList.end()); ++jobi) {
			if (skip < from) ++skip;
			else {
				npc = Game::Inst()->GetNPC((*jobi)->Assigned());
				if (npc) { 
					console->print(pos.X(), y, "%c", npc->GetNPCSymbol());
				}
				console->print(pos.X() + 2, y, "%s", (*jobi)->name.c_str());

#if DEBUG
				if (npc) {
					if (npc->currentTask() != 0) {
						console->print(pos.X() + 45, y, "%s", (*jobi)->ActionToString(npc->currentTask()->action).c_str());
					}
				}
				console->print(pos.X() + width - 11, y, "A-> %d", (*jobi)->Assigned());
#endif
				if (++y - pos.Y() >= height) {
					console->setDefaultForeground(TCODColor::white);
					return;
				}
			}
		}
	}
	console->setDefaultForeground(TCODColor::white);
}

boost::weak_ptr<Job> JobManager::GetJob(int uid) {
	boost::weak_ptr<Job> job;

	for (int i = 0; i < PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
			jobi != availableList[i].end(); ++jobi) {
				boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(uid);
				if (npc && (*jobi)->Menial() != npc->Expert()) {
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

	//Check tool jobs, remove them if they no longer point to existing jobs
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		for (std::vector<boost::weak_ptr<Job> >::iterator jobi = toolJobs[i].begin();
			jobi != toolJobs[i].end();) {
				if (!jobi->lock()) jobi = toolJobs[i].erase(jobi);
				else ++jobi;
		}
	}

	while (!failList.empty()) {
		failList.front()->Fail();
		failList.pop_front();
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

void JobManager::RemoveJob(boost::weak_ptr<Job> wjob) {
	if (boost::shared_ptr<Job> job = wjob.lock()) {
		for (int i = 0; i < PRIORITY_COUNT; ++i) {
			for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin(); jobi != availableList[i].end(); ++jobi) {
				if (*jobi == job) {
					jobi = availableList[i].erase(jobi);
					return;
				}
			}
		}
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = waitingList.begin(); jobi != waitingList.end(); ++jobi) {
			if (*jobi == job) {
				jobi = waitingList.erase(jobi);
				return;
			}
		}
	}
}

void JobManager::Reset() {
	delete instance;
	instance = 0;
}

void JobManager::NPCWaiting(int uid) {
	boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(uid);
	if (npc) {
		if (npc->Expert()) {
			for(std::vector<int>::iterator it = expertNPCsWaiting.begin(); it != expertNPCsWaiting.end(); it++) {
				if(*it == uid) {
					return;
				}
			}
			expertNPCsWaiting.push_back(uid);
		} else {
			for(std::vector<int>::iterator it = menialNPCsWaiting.begin(); it != menialNPCsWaiting.end(); it++) {
				if(*it == uid) {
					return;
				}
			}
			menialNPCsWaiting.push_back(uid);
		}
	}
}

void JobManager::NPCNotWaiting(int uid) {
	boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(uid);
	if (npc) {
		if (npc->Expert()) {
			for(std::vector<int>::iterator it = expertNPCsWaiting.begin(); it != expertNPCsWaiting.end(); it++) {
				if(*it == uid) {
					expertNPCsWaiting.erase(it);
					return;
				}
			}
		} else {
			for(std::vector<int>::iterator it = menialNPCsWaiting.begin(); it != menialNPCsWaiting.end(); it++) {
				if(*it == uid) {
					menialNPCsWaiting.erase(it);
					return;
				}
			}
		}
	}
}

void JobManager::ClearWaitingNpcs() {
	expertNPCsWaiting.clear();
	menialNPCsWaiting.clear();
}

void JobManager::AssignJobs() {
	//It's useless to attempt to assing more tool-required jobs than there are tools 
	std::vector<int> maxToolJobs(Item::Categories.size());
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		maxToolJobs[i] = StockManager::Inst()->CategoryQuantity(ItemCategory(i)) - toolJobs[i].size();
	}

	for (int i = 0; i < PRIORITY_COUNT && (!expertNPCsWaiting.empty() || !menialNPCsWaiting.empty()); i++) {
		if(!availableList[i].empty()) {
			std::vector<boost::shared_ptr<Job> > menialJobsToAssign;
			std::vector<boost::shared_ptr<Job> > expertJobsToAssign;
			for (std::list<boost::shared_ptr<Job> >::iterator jobi = availableList[i].begin();
				 jobi != availableList[i].end(); ++jobi) {
				if ((*jobi)->Assigned() == -1 && !(*jobi)->Removable()) {
					/*Limit assigning jobs to 20 at a time, large matrix sizes cause considerable slowdowns.
					Also, if the job requires a tool only add it to assignables if there are potentially enough
					tools for each job*/
					if (!(*jobi)->RequiresTool() || 
						((*jobi)->RequiresTool() && maxToolJobs[(*jobi)->GetRequiredTool()] > 0)) {
						if ((*jobi)->RequiresTool()) --maxToolJobs[(*jobi)->GetRequiredTool()];
						if ((*jobi)->Menial() && menialJobsToAssign.size() < 20) menialJobsToAssign.push_back(*jobi);
						else if (!(*jobi)->Menial() && expertJobsToAssign.size() < 20) expertJobsToAssign.push_back(*jobi);
					}
				}
			}
			if(!menialJobsToAssign.empty() || !expertJobsToAssign.empty()) {
				
				unsigned int menialMatrixSize = std::max(menialJobsToAssign.size(), menialNPCsWaiting.size());
				unsigned int expertMatrixSize = std::max(expertJobsToAssign.size(), expertNPCsWaiting.size());
				boost::numeric::ublas::matrix<int> menialMatrix(menialMatrixSize, menialMatrixSize);
				boost::numeric::ublas::matrix<int> expertMatrix(expertMatrixSize, expertMatrixSize);

				for(unsigned int x = 0; x < menialMatrixSize; x++) {
					for(unsigned int y = 0; y < menialMatrixSize; y++) {
						if(x >= menialNPCsWaiting.size() || y >= menialJobsToAssign.size()) {
							menialMatrix(x, y) = 1;
						} else {
							boost::shared_ptr<Job> job = menialJobsToAssign[y];
							boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(menialNPCsWaiting[x]);
							if(!npc || job->tasks.empty() ||
								(job->tasks[0].target.X() == 0 && job->tasks[0].target.Y() == 0)) {
								menialMatrix(x, y) = 1;
							} else if (npc) {
								menialMatrix(x, y) = 10000 - Distance(job->tasks[0].target, npc->Position());
							}
							if (npc && job->RequiresTool()) {
								if (!npc->Wielding().lock() || !npc->Wielding().lock()->IsCategory(job->GetRequiredTool())) {
									menialMatrix(x, y) -= 2000;
								}
							}
						}
					}
				}
				
				for(unsigned int x = 0; x < expertMatrixSize; x++) {
					for(unsigned int y = 0; y < expertMatrixSize; y++) {
						if(x >= expertNPCsWaiting.size() || y >= expertJobsToAssign.size()) {
							expertMatrix(x, y) = 1;
						} else {
							boost::shared_ptr<Job> job = expertJobsToAssign[y];
							boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(expertNPCsWaiting[x]);
							if(!npc || job->tasks.empty() ||
							   (job->tasks[0].target.X() == 0 && job->tasks[0].target.Y() == 0)) {
								expertMatrix(x, y) = 1;
							} else {
								expertMatrix(x, y) = 10000 - Distance(job->tasks[0].target, npc->Position());
							}
							if (npc && job->RequiresTool()) {
								if (!npc->Wielding().lock() || !npc->Wielding().lock()->IsCategory(job->GetRequiredTool())) {
									expertMatrix(x, y) -= 2000;
								}
							}
						}
					}
				}
				std::vector<int> menialAssignments = FindBestMatching(menialMatrix);
				std::vector<int> expertAssignments = FindBestMatching(expertMatrix);
				
				for(unsigned int i = 0, n = 0; n < menialNPCsWaiting.size(); i++, n++) {
					unsigned int jobNum = menialAssignments[i];
					if(jobNum < menialJobsToAssign.size()) {
						int npcNum = menialNPCsWaiting[n];
						boost::shared_ptr<Job> job = menialJobsToAssign[jobNum];
						boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(npcNum);
						if (job && npc) {
							job->Assign(npcNum);
							menialNPCsWaiting.erase(menialNPCsWaiting.begin() + n);
							n--;
							if (job->RequiresTool())
								toolJobs[job->GetRequiredTool()].push_back(job);
							npc->StartJob(job);
						}
					}
				}

				for(unsigned int i = 0, n = 0; n < expertNPCsWaiting.size(); i++, n++) {
					unsigned int jobNum = expertAssignments[i];
					if(jobNum < expertJobsToAssign.size()) {
						int npcNum = expertNPCsWaiting[n];
						boost::shared_ptr<Job> job = expertJobsToAssign[jobNum];
						boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(npcNum);
						if (job && npc) {
							job->Assign(npcNum);
							expertNPCsWaiting.erase(expertNPCsWaiting.begin() + n);
							n--;
							if (job->RequiresTool())
								toolJobs[job->GetRequiredTool()].push_back(job);
							npc->StartJob(job);
						}
					}
				}

			}
		}
	}
}

void JobManager::RemoveJob(Action action, Coordinate location) {
	for (int i = 0; i <= PRIORITY_COUNT; ++i) {
		for (std::list<boost::shared_ptr<Job> >::iterator jobi = (i < PRIORITY_COUNT ? availableList[i].begin() : waitingList.begin()); 
			jobi != (i < PRIORITY_COUNT ? availableList[i].end() : waitingList.end());) {
				bool remove = false;
				for (std::vector<Task>::iterator taski = (*jobi)->tasks.begin(); taski != (*jobi)->tasks.end(); ++taski) {
					if (taski->action == action && taski->target == location) {
						remove = true;
						break;
					}
				}
				if (remove) {
					(*jobi)->Attempts(0);
					if ((*jobi)->Assigned() >= 0) {
						boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC((*jobi)->Assigned());
						boost::weak_ptr<Job> jobToRemove = *jobi;
						++jobi; //AbortJob will cancel the job and the invalidate the old iterator
						if (npc) npc->AbortJob(jobToRemove);
					} else {
						jobi = (i < PRIORITY_COUNT ? availableList[i].erase(jobi) : waitingList.erase(jobi));
					}
				} else {
					++jobi;
				}
		}
	}
}

void JobManager::save(OutputArchive& ar, const unsigned int version) const {
	int count = PRIORITY_COUNT;
	ar & count;
	for (int i = 0; i < count; ++i) {
		ar & availableList[i];
	}
	ar & waitingList;
	ar & menialNPCsWaiting;
	ar & expertNPCsWaiting;
	ar & toolJobs;
	ar & failList;
}

void JobManager::load(InputArchive& ar, const unsigned int version) {
	if (version == 0) {
		std::list<boost::shared_ptr<Job> > oldList[3];
		ar & oldList;
		for (int i = 0; i < 3 && i < PRIORITY_COUNT; ++i) {
			availableList[i] = oldList[i];
		}
	} else {
		int count;
		ar & count;
		for (int i = 0; i < count && i < PRIORITY_COUNT; ++i) {
			ar & availableList[i];
		}
	}
	ar & waitingList;
	ar & menialNPCsWaiting;
	ar & expertNPCsWaiting;
	ar & toolJobs;
	ar & failList;
}
