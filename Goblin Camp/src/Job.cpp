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
	name(value),
	internal(false)
{
}

Job::~Job() {
	preReqs.clear();
	UnreserveItems();
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
void Job::Attempt() {++attempts;}

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
	buildJob->tasks.push_back(Task(MOVEADJACENT, Coordinate(construct.lock()->x(),construct.lock()->y()), construct));
	buildJob->tasks.push_back(Task(BUILD, Coordinate(construct.lock()->x(),construct.lock()->y()), construct));
	return buildJob;
}

void Job::ReserveItem(boost::weak_ptr<Item> item) {
	if (item.lock()) {
        reservedItems.push_back(item);
        item.lock()->Reserve(true);
	}
}

void Job::UnreserveItems() {
	for (std::list<boost::weak_ptr<Item> >::iterator itemI = reservedItems.begin(); itemI != reservedItems.end(); ++itemI) {
		if (itemI->lock()) itemI->lock()->Reserve(false);
	}
	reservedItems.clear();
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

JobManager::JobManager() {}
JobManager *JobManager::instance = 0;

JobManager *JobManager::Inst() {
	if (!instance) instance = new JobManager();
	return instance;
}

void JobManager::AddJob(boost::shared_ptr<Job> newJob) {
	newJob->Attempt();
	if (newJob->Attempts() > MAXIMUM_JOB_ATTEMPTS) {
		newJob->Fail();
		return;
	}

	if (newJob->PreReqsCompleted()) {
		switch (newJob->priority()) {
			case LOW: lowList.push_back(newJob); return;
			case MED: medList.push_back(newJob); return;
			case HIGH: highList.push_back(newJob); return;
		}
	} else {
		newJob->Paused(true);
		waitingList.push_back(newJob);
	}
}

void JobManager::CancelJob(boost::weak_ptr<Job> oldJob, std::string msg, TaskResult result) {
    if (oldJob.lock()) {
        if (oldJob.lock()->priority() > LOW)
            Announce::Inst()->AddMsg(oldJob.lock()->name+std::string(" canceled: ")+msg, TCODColor::red);

        oldJob.lock()->Assign(-1);
        oldJob.lock()->Paused(true);
        waitingList.push_back(oldJob.lock());

        switch(oldJob.lock()->priority()) {
            case LOW:
                for(std::list<boost::shared_ptr<Job> >::iterator lowIter = lowList.begin(); lowIter != lowList.end();
                    ++lowIter) {
                if ((*lowIter) == oldJob.lock()) {
                    lowList.erase(lowIter);
                    break;
                }
            }
                break;
            case MED:
                for(std::list<boost::shared_ptr<Job> >::iterator medIter = medList.begin(); medIter != medList.end();
                    ++medIter) {
                if ((*medIter) == oldJob.lock()) {
                    medList.erase(medIter);
                    break;
                }
            }
                break;
            case HIGH:
                for(std::list<boost::shared_ptr<Job> >::iterator highIter = highList.begin(); highIter != highList.end();
                    ++highIter) {
                    if ((*highIter) == oldJob.lock()) {
                        highList.erase(highIter);
                        break;
                    }
                }
                break;
        }
    }
}

void JobManager::Draw(Coordinate pos, int from, int count, TCODConsole* console) {
	int skip = 0;
	int y = pos.y();

	console->setForegroundColor(TCODColor::lightCyan);
	for (std::list<boost::shared_ptr<Job> >::iterator highIter = highList.begin(); highIter != highList.end(); ++highIter) {
		if (skip < from) ++skip;
		else {

			console->print(pos.x(), y, "%s", (*highIter)->name.c_str());
			if (++y - pos.y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::white);
	for (std::list<boost::shared_ptr<Job> >::iterator medIter = medList.begin(); medIter != medList.end(); ++medIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.x(), y, "%s", (*medIter)->name.c_str());
			if (++y - pos.y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::lightGrey);
	for (std::list<boost::shared_ptr<Job> >::iterator lowIter = lowList.begin(); lowIter != lowList.end(); ++lowIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.x(), y, "%s", (*lowIter)->name.c_str());
			if (++y - pos.y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::grey);
	for (std::list<boost::shared_ptr<Job> >::iterator waitIter = waitingList.begin(); waitIter != waitingList.end(); ++waitIter) {
		if (skip < from) ++skip;
		else {
			console->print(pos.x(), y, "%s", (*waitIter)->name.c_str());
			if (++y - pos.y() >= count) return;
		}
	}

	console->setForegroundColor(TCODColor::white);
}

boost::weak_ptr<Job> JobManager::GetJob(int uid) {
	boost::weak_ptr<Job> job;

	for (std::list<boost::shared_ptr<Job> >::iterator highIter = highList.begin(); highIter != highList.end(); ++highIter) {
		if ((*highIter)->Menial() != Game::Inst()->npcList[uid]->Expert()) {
			if ((*highIter)->Assigned() == -1 && !(*highIter)->Removable()) {
				job = (*highIter);
				break;
			}
		}
	}

	if (!job.lock()) {
		for (std::list<boost::shared_ptr<Job> >::iterator medIter = medList.begin(); medIter != medList.end(); ++medIter) {
			if ((*medIter)->Menial() != Game::Inst()->npcList[uid]->Expert()) {
				if ((*medIter)->Assigned() == -1 && !(*medIter)->Removable()) {
					job = (*medIter);
					break;
				}
			}
		}
	}

	if (!job.lock()) {
		for (std::list<boost::shared_ptr<Job> >::iterator lowIter = lowList.begin(); lowIter != lowList.end(); ++lowIter) {
			if ((*lowIter)->Menial() != Game::Inst()->npcList[uid]->Expert()) {
				if ((*lowIter)->Assigned() == -1 && !(*lowIter)->Removable()) {
					job = (*lowIter);
					break;
				}
			}
		}
	}

	//Now check the distance to the job, the further away the NPC is the larger the chance is that
	//the assignment gets cancelled. This random-chance will skew results towards closerby npc's
	//picking up jobs rather than just anyone.
	if (job.lock()) {
		if (rand() % 100 < std::min(95, Game::Inst()->DistanceNPCToCoordinate(uid, job.lock()->tasks.front().target))) {
			job.reset();
		} else {
			job.lock()->Assign(uid);
		}
	}

	return job;
}

void JobManager::Update() {

	//Check the waiting list for jobs that have completed prerequisites, and move them to the
	//job queue if so, remove removables, and retry retryables, remove unpauseds
	for (std::list<boost::shared_ptr<Job> >::iterator jobIter = waitingList.begin(); jobIter != waitingList.end(); ++jobIter) {

		if ((*jobIter)->Removable()) {
			jobIter = waitingList.erase(jobIter);
		} else {

			if (!(*jobIter)->PreReqs()->empty() && (*jobIter)->PreReqsCompleted()) {
				if (rand() % (UPDATES_PER_SECOND*3) == 0) (*jobIter)->Paused(false);
			}

			if (!(*jobIter)->Paused()) {
				AddJob(*jobIter);
				jobIter = waitingList.erase(jobIter);
			} else if (!(*jobIter)->Parent().lock() && !(*jobIter)->PreReqs()->empty()) {
				//Job has unfinished prereqs, itsn't removable and is NOT a prereq itself
				if (rand() % (UPDATES_PER_SECOND*3) == 0) {
					for (std::list<boost::weak_ptr<Job> >::iterator pri = (*jobIter)->PreReqs()->begin(); pri != (*jobIter)->PreReqs()->end(); ++pri) {
						if (pri->lock()) {
                            pri->lock()->Paused(false);
						}
					}
				}
			} else if (!(*jobIter)->Parent().lock()) {
			    if (rand() % (UPDATES_PER_SECOND*3) == 0) {
			        (*jobIter)->Paused(false);
			    }
			}
		}
	}

	//Check the normal queues for jobs that have all prerequisites and parents completed and
	//remove them
	for (std::list<boost::shared_ptr<Job> >::iterator highIter = highList.begin(); highIter != highList.end(); ++highIter) {
		if ((*highIter)->Completed() && (*highIter)->PreReqsCompleted()) {
			highIter = highList.erase(highIter);
		}
	}
	for (std::list<boost::shared_ptr<Job> >::iterator medIter = medList.begin(); medIter != medList.end(); ++medIter) {
		if ((*medIter)->Completed() && (*medIter)->PreReqsCompleted()) {
			medIter = medList.erase(medIter);
		}
	}
	for (std::list<boost::shared_ptr<Job> >::iterator lowIter = lowList.begin(); lowIter != lowList.end(); ++lowIter) {
		if ((*lowIter)->Completed() && (*lowIter)->PreReqsCompleted()) {
			lowIter = lowList.erase(lowIter);
		}
	}
}

int JobManager::JobAmount() { return highList.size() + medList.size() + lowList.size() + waitingList.size(); }

boost::weak_ptr<Job> JobManager::GetJobByListIndex(int index) {
	int count = 0;
	for (std::list<boost::shared_ptr<Job> >::iterator highIter = highList.begin(); highIter != highList.end(); ++highIter) {
		if (count++ == index) return (*highIter);
	}
	for (std::list<boost::shared_ptr<Job> >::iterator medIter = medList.begin(); medIter != medList.end(); ++medIter) {
		if (count++ == index) return (*medIter);
	}
	for (std::list<boost::shared_ptr<Job> >::iterator lowIter = lowList.begin(); lowIter != lowList.end(); ++lowIter) {
		if (count++ == index) return (*lowIter);
	}
	for (std::list<boost::shared_ptr<Job> >::iterator waitingIter = waitingList.begin(); waitingIter != waitingList.end(); ++waitingIter) {
		if (count++ == index) return (*waitingIter);
	}
	return boost::weak_ptr<Job>();
}
