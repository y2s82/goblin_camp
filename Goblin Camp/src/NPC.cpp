#include <cstdlib>
#include <boost/thread/thread.hpp>
#include <boost/multi_array.hpp>
#include <libtcod.hpp>
#include <string>
#ifdef DEBUG
#include <iostream>
#endif

#include "npc.hpp"
#include "coordinate.hpp"
#include "job.hpp"
#include "gcamp.hpp"
#include "Game.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "StatusEffect.hpp"
#include "Camp.hpp"

SkillSet::SkillSet() {
	for (int i = 0; i < SKILLAMOUNT; ++i) { skills[i] = 0; }
}

int SkillSet::operator()(Skill skill) {return skills[skill];}
void SkillSet::operator()(Skill skill, int value) {skills[skill] = value;}

NPC::NPC(Coordinate pos, boost::function<bool(boost::shared_ptr<NPC>)> findJob,
            boost::function<void(boost::shared_ptr<NPC>)> react) : Entity(),
	type(0),
	timeCount(0),
	taskIndex(0),
	nopath(false),
	findPathWorking(false),
	timer(0),
	_speed(10), nextMove(0),
	run(true),
	taskBegun(false),
	expert(false),
	carried(boost::weak_ptr<Item>()),
	thirst(0),
	hunger(0),
	thinkSpeed(UPDATES_PER_SECOND / 5), //Think 5 times a second
	statusEffects(std::list<StatusEffect>()),
	statusEffectIterator(statusEffects.end()),
	statusGraphicCounter(0),
	health(100),
	foundItem(boost::weak_ptr<Item>()),
	bag(boost::shared_ptr<Container>(new Container(pos, 0, 10, -1))),
	needsNutrition(false),
	attackSkill(0), attackPower(0), attackSpeed(0),
	defenceSkill(0),
	aggressive(false),
	aggressor(boost::weak_ptr<NPC>()),
	dead(false),
	squad(boost::weak_ptr<Squad>()),
	FindJob(findJob),
	React(react)
{
	while (!Map::Inst()->Walkable(pos.x(),pos.y())) {
		pos.x(pos.x()+1);
		pos.y(pos.y()+1);
	}
	Position(pos,true);

	thirst += rand() % 10; //Just for some variety
	hunger += rand() % 10;

	path = new TCODPath(Map::Inst()->Width(), Map::Inst()->Height(), Map::Inst(), 0);
}

NPC::~NPC() {
	Map::Inst()->NPCList(_x, _y)->erase(uid);
	if (squad.lock()) squad.lock()->Leave(uid);
}

void NPC::Position(Coordinate pos, bool firstTime) {
	if (!firstTime) {
		if (Map::Inst()->MoveTo(pos.x(), pos.y(), uid)) {
			Map::Inst()->MoveFrom(_x, _y, uid);
			_x = pos.x();
			_y = pos.y();
			Map::Inst()->MoveTo(_x,_y,uid); //TODO: Figure out why the hell this is required
		}
	} else {
		if (Map::Inst()->MoveTo(pos.x(), pos.y(), uid)) {
			_x = pos.x();
			_y = pos.y();
		}
	}
}

void NPC::Position(Coordinate pos) { Position(pos, false); }

bool *NPC::visArray() { return _visArray; }

Task* NPC::currentTask() { return jobs.empty() ? 0 : &(jobs.front()->tasks[taskIndex]); }

boost::weak_ptr<Job> NPC::currentJob() { return jobs.empty() ? boost::weak_ptr<Job>() : boost::weak_ptr<Job>(jobs.front()); }

void NPC::TaskFinished(TaskResult result, std::string msg) {
    if (result == TASKSUCCESS) {
        if (++taskIndex >= (signed int)jobs.front()->tasks.size()) {
            jobs.front()->Complete();
            jobs.pop_front();
            taskIndex = 0;
        }
    } else {
        if (!jobs.front()->internal) JobManager::Inst()->CancelJob(jobs.front(), msg, result);
        else if (msg.size() > 0) Announce::Inst()->AddMsg(std::string("Job cancelled: ")+msg);
        jobs.pop_front();
        taskIndex = 0;
        DropCarriedItem();
    }

	taskBegun = false;
}

void NPC::HandleThirst() {
	Coordinate tmpCoord;
	bool found = false;

	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Drink") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(BEVERAGE);
		if (!item.lock()) {tmpCoord = Game::Inst()->FindWater(Position());}
		if (!item.lock() && tmpCoord.x() == -1) { //Nothing to drink!
			//:ohdear:
		} else { //Something to drink!
			boost::shared_ptr<Job> newJob(new Job("Drink", MED, 0, !expert));
			newJob->internal = true;

			if (item.lock()) {
				newJob->ReserveItem(item);
				newJob->tasks.push_back(Task(MOVE,item.lock()->Position()));
				newJob->tasks.push_back(Task(TAKE,item.lock()->Position(), item));
				newJob->tasks.push_back(Task(DRINK));
				jobs.push_back(newJob);
			} else {
				for (int ix = tmpCoord.x()-1; ix <= tmpCoord.x()+1; ++ix) {
					for (int iy = tmpCoord.y()-1; iy <= tmpCoord.y()+1; ++iy) {
						if (Map::Inst()->Walkable(ix,iy)) {
								newJob->tasks.push_back(Task(MOVE, Coordinate(ix,iy)));
								goto CONTINUEDRINKBLOCK;
						}
					}
				}
				Logger::Inst()->output<<"Couldn't find a walkable tile for water\n";
				CONTINUEDRINKBLOCK:
				newJob->tasks.push_back(Task(DRINK, tmpCoord));
				jobs.push_back(newJob);
			}
		}
	}
}

void NPC::HandleHunger() {
	Coordinate tmpCoord;
	bool found = false;

	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Eat") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Food"));
		if (!item.lock()) {item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Raw Food"));}
		if (!item.lock()) { //Nothing to eat!
			//:ohdear:
		} else { //Something to eat!
			boost::shared_ptr<Job> newJob(new Job("Eat", MED, 0, !expert));
			newJob->internal = true;

			newJob->ReserveItem(item);
			newJob->tasks.push_back(Task(MOVE,item.lock()->Position()));
			newJob->tasks.push_back(Task(TAKE,item.lock()->Position(), item));
			newJob->tasks.push_back(Task(EAT));
			jobs.push_back(newJob);
		}
	}
}

AiThink NPC::Think() {
	Coordinate tmpCoord;
	int tmp;
	TaskResult result;

	if (Map::Inst()->NPCList(_x,_y)->size() > 1) _bgcolor = TCODColor::darkGrey;
	else _bgcolor = TCODColor::black;

	++statusGraphicCounter;
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->cooldown > 0 && --statusEffectI->cooldown == 0) {
			if (statusEffectI == statusEffectIterator) {
				++statusEffectIterator;
				statusGraphicCounter = 0;
			}
			statusEffectI = statusEffects.erase(statusEffectI);
			if (statusEffectIterator == statusEffects.end()) statusEffectIterator = statusEffects.begin();
		}
	}


	if (statusGraphicCounter > 10) {
		statusGraphicCounter = 0;
		if (statusEffectIterator != statusEffects.end()) ++statusEffectIterator;
		else statusEffectIterator = statusEffects.begin();
	}

	if (needsNutrition) {
		++thirst; ++hunger;

		if (thirst >= THIRST_THRESHOLD) AddEffect(THIRST);
		else RemoveEffect(THIRST);
		if (hunger >= HUNGER_THRESHOLD) AddEffect(HUNGER);
		else RemoveEffect(HUNGER);

		if (thirst > THIRST_THRESHOLD && (rand() % (UPDATES_PER_SECOND*5)) == 0) {
			HandleThirst();
		} else if (thirst > THIRST_THRESHOLD * 5) Kill();
		if (hunger > HUNGER_THRESHOLD && (rand() % (UPDATES_PER_SECOND*5)) == 0) {
			HandleHunger();
		} else if (hunger > HUNGER_THRESHOLD * 10) Kill();
	}

	result = Move();

	timeCount += thinkSpeed;
	while (timeCount > UPDATES_PER_SECOND) {

        if (rand() % 2 == 0) React(boost::static_pointer_cast<NPC>(shared_from_this()));

		if (aggressor.lock())
			if (Game::Inst()->Adjacent(Position(), aggressor)) Hit(aggressor);

		timeCount -= UPDATES_PER_SECOND;
		if (!jobs.empty()) {
			switch(currentTask()->action) {
				case MOVE:
					if ((signed int)_x == currentTarget().x() && (signed int)_y == currentTarget().y()) {
						TaskFinished(TASKSUCCESS);
						break;
					}
					if (!taskBegun) { findPath(currentTarget()); taskBegun = true; result = TASKCONTINUE;}
					//result = Move();
					if (result == TASKFAILFATAL || result == TASKFAILNONFATAL) {
						TaskFinished(result, std::string("Could not find path to target Move()")); break;
					} else if (result == PATHEMPTY) {
					    if (!((signed int)_x == currentTarget().x() &&  (signed int)_y == currentTarget().y())) {
					        TaskFinished(TASKFAILFATAL, std::string("No path to target Move()")); break;
					    }
					}
					break;

				case MOVENEAR:
					//MOVENEAR first figures out our "real" target, which is a tile near
					//to our current target. Near means max 5 tiles away, visible and
					//walkable. Once we have our target we can actually switch over
					//to a normal MOVE task
					tmp = 0;
					while (tmp++ < 10) {
						int tarX = ((rand() % 11) - 5) + currentTarget().x();
						int tarY = ((rand() % 11) - 5) + currentTarget().y();
						if (Map::Inst()->Walkable(tarX, tarY)) {
							if (Map::Inst()->LineOfSight(tarX, tarY, currentTarget().x(), currentTarget().y())) {
								currentJob().lock()->tasks[taskIndex] = Task(MOVE, Coordinate(tarX, tarY));
								break;
							}
						}
					}
					break;


				case MOVEADJACENT:
					if (Game::Inst()->Adjacent(Position(), currentEntity())) {
						TaskFinished(TASKSUCCESS);
						break;
					}
					if (!taskBegun) {
						tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity());
						if (tmpCoord.x() >= 0) {
							findPath(tmpCoord);
						} else { TaskFinished(TASKFAILFATAL, std::string("No walkable adjacent tiles")); break; }
						taskBegun = true;
						result = TASKCONTINUE;
					}
					//result = Move();
					if (result == TASKFAILFATAL || result == TASKFAILNONFATAL) { TaskFinished(result, std::string("Could not find path to target")); break; }
					else if (result == PATHEMPTY) {
/*					    if (!((signed int)_x == currentTarget().x() &&  (signed int)_y == currentTarget().y())) {
					        TaskFinished(TASKFAILFATAL, std::string("No path to target")); break;
					    } */
						TaskFinished(TASKFAILFATAL);
					}
					break;

				case WAIT:
					if (++timer > currentTarget().x()) { timer = 0; TaskFinished(TASKSUCCESS); }
					break;

				case BUILD:
					if (Game::Inst()->Adjacent(Position(), currentEntity())) {
						tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Build();
						if (tmp > 0) {
							TaskFinished(TASKSUCCESS);
							Announce::Inst()->AddMsg("Finished building");
							break;
						} else if (tmp == BUILD_NOMATERIAL) {
							TaskFinished(TASKFAILFATAL, "Missing materials");
							break;
						}
					} else {
						TaskFinished(TASKFAILFATAL, "Not adjacent to building");
						break;
					}
					break;

				case TAKE:
					if (!currentEntity().lock()) { TaskFinished(TASKFAILFATAL); break; }
					if (Position() == currentEntity().lock()->Position()) {
						if (boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()) {
						    boost::weak_ptr<Container> cont(boost::static_pointer_cast<Container>(boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()));
							cont.lock()->RemoveItem(
								boost::static_pointer_cast<Item>(currentEntity().lock()));
						}
						carried = boost::static_pointer_cast<Item>(currentEntity().lock());
						if (!bag->AddItem(carried)) Announce::Inst()->AddMsg("No space in bag");
						TaskFinished(TASKSUCCESS);
						break;
					} else { TaskFinished(TASKFAILFATAL, "Item not found"); break; }
					break;

				case DROP:
					DropCarriedItem();
					TaskFinished(TASKSUCCESS);
					break;

				case PUTIN:
                    bag->RemoveItem(carried);
                    carried.lock()->Position(Position());
					if (currentEntity().lock()) {
						if (!boost::static_pointer_cast<Container>(currentEntity().lock())->AddItem(carried)) Announce::Inst()->AddMsg("Container full!");
					}
                    carried.reset();
                    TaskFinished(TASKSUCCESS);
                    break;

				case DRINK: //Either we have an item target to drink, or a water tile
					if (carried.lock()) { //Drink from an item
                        thirst -= boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
                        bag->RemoveItem(carried);
                        Game::Inst()->RemoveItem(carried);
                        carried = boost::weak_ptr<Item>();
                        TaskFinished(TASKSUCCESS);
                        break;
					} else { //Drink from a water tile
						if (std::abs((signed int)_x - currentTarget().x()) <= 1 &&
							std::abs((signed int)_y - currentTarget().y()) <= 1) {
							if (Map::Inst()->GetWater(currentTarget().x(), currentTarget().y()).lock()->Depth() > DRINKABLE_WATER_DEPTH) {
								thirst -= (int)(THIRST_THRESHOLD / 10);
								if (thirst < 0) { TaskFinished(TASKSUCCESS); break; }
							} else { TaskFinished(TASKFAILFATAL, "Not enough water"); break; }
						} else { TaskFinished(TASKFAILFATAL, "Not adjacent to water"); break; }
					}
					break;

				case EAT:
					hunger -= boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
					bag->RemoveItem(carried);

					for (std::list<ItemType>::iterator fruiti = Item::Presets[carried.lock()->Type()].fruits.begin(); fruiti != Item::Presets[carried.lock()->Type()].fruits.end(); ++fruiti) {
					    Game::Inst()->CreateItem(Position(), *fruiti, true);
					}

					Game::Inst()->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
					TaskFinished(TASKSUCCESS);
					break;

                case FIND:
                    foundItem = Game::Inst()->FindItemByCategoryFromStockpiles(currentTask()->item);
                    if (!foundItem.lock()) {
						TaskFinished(TASKFAILFATAL); 
#ifdef DEBUG
						std::cout<<"Can't FIND required item\n";
#endif
						break;
					}
                    else {
                        currentJob().lock()->ReserveItem(foundItem);
                        TaskFinished(TASKSUCCESS);
                        break;
                    }
                    break;

                case USE:
                    if (boost::dynamic_pointer_cast<Construction>(currentEntity().lock())) {
                        tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Use();
                        if (tmp >= 100) {
                            TaskFinished(TASKSUCCESS);
                        } else if (tmp < 0) {
                            TaskFinished(TASKFAILFATAL, "Unable to use construct!"); break;
                        }
                    } else { TaskFinished(TASKFAILFATAL, "Attempted to use non-construct"); break; }
                    break;

                case HARVEST:
                    if (carried.lock()) {
                        for (std::list<ItemType>::iterator fruiti = Item::Presets[carried.lock()->Type()].fruits.begin(); fruiti != Item::Presets[carried.lock()->Type()].fruits.end(); ++fruiti) {
                            Game::Inst()->CreateItem(Position(), *fruiti, true);
                        }
                        bag->RemoveItem(carried);
                        Game::Inst()->RemoveItem(carried);
                        carried = boost::weak_ptr<Item>();
                        TaskFinished(TASKSUCCESS);
                        break;
                    } else {
                        bag->RemoveItem(carried);
                        carried = boost::weak_ptr<Item>();
                        TaskFinished(TASKFAILFATAL, "Carrying nonexistant item");
                        break;
                    }

                case FELL:
                    tmp = boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Fell();
                    if (tmp <= 0) {
                        for (std::list<ItemType>::iterator iti = NatureObject::Presets[boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Type()].components.begin(); iti != NatureObject::Presets[boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Type()].components.end(); ++iti) {
                            Game::Inst()->CreateItem(Coordinate(currentEntity().lock()->x(), currentEntity().lock()->y()), *iti, true);
                        }
                        Map::Inst()->Walkable(currentEntity().lock()->x(), currentEntity().lock()->y(), true);
                        Game::Inst()->RemoveNatureObject(boost::static_pointer_cast<NatureObject>(currentEntity().lock()));
                        TaskFinished(TASKSUCCESS);
                    }
                    break;

                case HARVESTWILDPLANT:
                    tmp = boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Harvest();
                    if (tmp <= 0) {
                        for (std::list<ItemType>::iterator iti = NatureObject::Presets[boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Type()].components.begin(); iti != NatureObject::Presets[boost::static_pointer_cast<NatureObject>(currentEntity().lock())->Type()].components.end(); ++iti) {
                            Game::Inst()->CreateItem(Coordinate(currentEntity().lock()->x(),currentEntity().lock()->y()), *iti, true);
                        }
                        Map::Inst()->Walkable(currentEntity().lock()->x(), currentEntity().lock()->y(), true);
                        Game::Inst()->RemoveNatureObject(boost::static_pointer_cast<NatureObject>(currentEntity().lock()));
                        TaskFinished(TASKSUCCESS);
                    }
                    break;

				case KILL:
					//The reason KILL isn't a combination of MOVEADJACENT and something else is that the other moving actions
					//assume their target isn't a moving one
					if (!currentEntity().lock()) {
						TaskFinished(TASKSUCCESS);
						break;
					}

					if (Game::Inst()->Adjacent(Position(), currentEntity())) {
						Hit(currentEntity());
						break;
					}

					if (!taskBegun || rand() % (UPDATES_PER_SECOND * 2) == 0) { //Repath every ~2 seconds
						findPath(currentEntity().lock()->Position());
						taskBegun = true;
						result = TASKCONTINUE;
					}
					//result = Move();

					if (result == TASKFAILFATAL || result == TASKFAILNONFATAL) { TaskFinished(result, std::string("Could not find path to target")); break; }
					else if (result == PATHEMPTY) {
						findPath(currentEntity().lock()->Position());
					}
					break;

				default: TaskFinished(TASKFAILFATAL, "*BUG*Unknown task*BUG*"); break;
			}
		} else {
			//Idly meander while no jobs available
			if (HasEffect(PANIC)) {
				bool enemyFound = false;
			    if (jobs.empty() && !nearNpcs.empty()) {
			        boost::shared_ptr<Job> fleeJob(new Job("Flee"));
					fleeJob->internal = true;
					for (std::list<boost::weak_ptr<NPC> >::iterator npci = nearNpcs.begin(); npci != nearNpcs.end(); ++npci) {
						if (npci->lock() && npci->lock()->faction != faction) {
							int dx = _x - npci->lock()->_x;
							int dy = _y - npci->lock()->_y;
							if (Map::Inst()->Walkable(_x + dx, _y + dy)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x+dx,_y+dy)));
								jobs.push_back(fleeJob);
							} else if (Map::Inst()->Walkable(_x + dx, _y)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x+dx,_y)));
								jobs.push_back(fleeJob);
							} else if (Map::Inst()->Walkable(_x, _y + dy)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x,_y+dy)));
								jobs.push_back(fleeJob);
							}
							enemyFound = true;
							break;
						}
					}
			    }
			} else if (!FindJob(boost::static_pointer_cast<NPC>(shared_from_this()))) {
				//if (rand() % 100 == 0) Position(Coordinate(_x + rand() % 3 - 1, _y + rand() % 3 - 1));
				boost::shared_ptr<Job> idleJob(new Job("Idle"));
				idleJob->internal = true;
				idleJob->tasks.push_back(Task(MOVENEAR, faction == 0 ? Camp::Inst()->MeetingPoint() : Position()));
				idleJob->tasks.push_back(Task(WAIT, Coordinate(rand() % 10, 0)));
				jobs.push_back(idleJob);
				run = false;
			}
		}
	}
	return AINOTHING;
}

TaskResult NPC::Move() {
	int x,y;
	nextMove += run ? _speed : _speed/3;
	while (nextMove > 100) {
		nextMove -= 100;
		boost::mutex::scoped_try_lock pathLock(pathMutex);
		if (pathLock.owns_lock()) {
			if (nopath) {nopath = false; return TASKFAILFATAL;}
			if (!path->isEmpty()) {
				if (!path->walk(&x, &y, false)) return TASKFAILNONFATAL;
				Position(Coordinate(x,y));
			} else if (!findPathWorking) return PATHEMPTY;
		}
	}
	return TASKCONTINUE;
}

void NPC::findPath(Coordinate target) {
    findPathWorking = true;
	boost::thread pathThread(boost::bind(tFindPath, path, _x, _y, target.x(), target.y(), &pathMutex, &nopath, &findPathWorking));
}

void NPC::speed(unsigned int value) {_speed=value;}
unsigned int NPC::speed() {return _speed;}

void NPC::Draw(Coordinate upleft, TCODConsole *console) {
	int screenx = _x - upleft.x();
	int screeny = _y - upleft.y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		if (statusGraphicCounter < 5 || statusEffectIterator == statusEffects.end()) {
			console->putCharEx(screenx, screeny, _graphic, _color, _bgcolor);
		} else {
			console->putCharEx(screenx, screeny, statusEffectIterator->graphic, statusEffectIterator->color, _bgcolor);
		}
	}
}

void NPC::color(TCODColor value, TCODColor bvalue) { _color = value; _bgcolor = bvalue; }
void NPC::graphic(int value) { _graphic = value; }

bool NPC::Expert() {return expert;}
void NPC::Expert(bool value) {expert = value;}

Coordinate NPC::Position() {return Coordinate(_x,_y);}

bool NPC::Dead() { return dead; }
void NPC::Kill() {
	if (!dead) {//You can't be killed if you're already dead!
		dead = true;
		health = 0;
	#ifdef DEBUG
		std::cout<<"Kill npc:"<<name<<uid<<"\n";
	#endif
		Logger::Inst()->output<<"Kill npc:"<<name<<uid<<"\n";
		int corpse = Game::Inst()->CreateItem(Position(), Item::StringToItemType("Corpse"), false);
		Game::Inst()->GetItem(corpse).lock()->Color(_color);
		Game::Inst()->GetItem(corpse).lock()->Name(Game::Inst()->GetItem(corpse).lock()->Name() + "(" + name + ")");
		while (!jobs.empty()) TaskFinished(TASKFAILFATAL, std::string("Dead"));
	}
}

void NPC::DropCarriedItem() {
    if (carried.lock()) {
        bag->RemoveItem(carried);
		carried.lock()->PutInContainer(boost::weak_ptr<Item>());
        carried.lock()->Position(Position());
        carried.reset();
    }
}

Coordinate NPC::currentTarget() {
    if (currentTask()->target == Coordinate(0,0) && foundItem.lock()) {
        return foundItem.lock()->Entity::Position();
    }
    return currentTask()->target;
}

boost::weak_ptr<Entity> NPC::currentEntity() {
    if (currentTask()->entity.lock()) return currentTask()->entity;
    else return boost::weak_ptr<Entity>(foundItem.lock());
}


void tFindPath(TCODPath *path, int x0, int y0, int x1, int y1, boost::try_mutex *pathMutex, bool *nopath, bool *findPathWorking) {
	boost::mutex::scoped_lock pathLock(*pathMutex);
	*nopath = !path->compute(x0, y0, x1, y1);
	*findPathWorking = false;
}

bool NPC::GetSquadJob(boost::shared_ptr<NPC> npc) {
	if (npc->MemberOf().lock()) {
		boost::shared_ptr<Job> newJob(new Job("Follow orders"));
		newJob->internal = true;
		switch (npc->MemberOf().lock()->Order()) {
		case GUARD:
			if (npc->MemberOf().lock()->TargetCoordinate().x() >= 0) {
				newJob->tasks.push_back(Task(MOVENEAR, npc->MemberOf().lock()->TargetCoordinate()));
				//newJob->tasks.push_back(Task(MOVE, npc->MemberOf().lock()->TargetCoordinate()));
				//WAIT waits Coordinate.x / 5 seconds
				newJob->tasks.push_back(Task(WAIT, Coordinate(5*5, 0)));
				npc->jobs.push_back(newJob);
				npc->run = false;
				return true;
			}
			break;
		
		case ESCORT:
			if (npc->MemberOf().lock()->TargetEntity().lock()) {
				newJob->tasks.push_back(Task(MOVENEAR, npc->MemberOf().lock()->TargetEntity().lock()->Position(), npc->MemberOf().lock()->TargetEntity()));
				npc->jobs.push_back(newJob);
				npc->run = true;
				return true;
			}
			break;
		}
	} 
	return false;
}

bool NPC::JobManagerFinder(boost::shared_ptr<NPC> npc) {
	//Either create an internal job if this npc is part of a squad, or get one from the JobManager
	if (!GetSquadJob(npc)) {
   		boost::shared_ptr<Job> newJob(JobManager::Inst()->GetJob(npc->uid).lock());
		if (newJob)  {
			npc->jobs.push_back(newJob);
			npc->run = true;
			return true;
		}
		return false;
	}
	return true;
}

void NPC::PlayerNPCReact(boost::shared_ptr<NPC> npc) {
	if (npc->aggressive) {
		if (npc->jobs.empty() || npc->currentTask()->action != KILL) {
			Game::Inst()->FindNearbyNPCs(npc, true);
			for (std::list<boost::weak_ptr<NPC> >::iterator npci = npc->nearNpcs.begin(); npci != npc->nearNpcs.end(); ++npci) {
				if (npci->lock()->faction != npc->faction) {
					boost::shared_ptr<Job> killJob(new Job("Kill "+npci->lock()->name));
					killJob->internal = true;
					killJob->tasks.push_back(Task(KILL, npci->lock()->Position(), *npci));
					while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILNONFATAL);
#ifdef DEBUG
					std::cout<<"Push_back(killJob)\n";
#endif
					npc->jobs.push_back(killJob);
					npc->run = true;
				}
			}
		}
	}
}

void NPC::PeacefulAnimalReact(boost::shared_ptr<NPC> animal) {
    Game::Inst()->FindNearbyNPCs(animal);
    for (std::list<boost::weak_ptr<NPC> >::iterator npci = animal->nearNpcs.begin(); npci != animal->nearNpcs.end(); ++npci) {
        if (npci->lock()->faction != animal->faction) {
            animal->AddEffect(PANIC);
        }
    }
}

bool NPC::PeacefulAnimalFindJob(boost::shared_ptr<NPC> animal) {
    return false;
}

void NPC::HostileAnimalReact(boost::shared_ptr<NPC> animal) {
    Game::Inst()->FindNearbyNPCs(animal);
    for (std::list<boost::weak_ptr<NPC> >::iterator npci = animal->nearNpcs.begin(); npci != animal->nearNpcs.end(); ++npci) {
        if (npci->lock()->faction != animal->faction) {
			boost::shared_ptr<Job> killJob(new Job("Kill "+npci->lock()->name));
			killJob->internal = true;
			killJob->tasks.push_back(Task(KILL, npci->lock()->Position(), *npci));
			while (!animal->jobs.empty()) animal->TaskFinished(TASKFAILNONFATAL);
			animal->jobs.push_back(killJob);
			animal->run = true;
        }
    }
}

bool NPC::HostileAnimalFindJob(boost::shared_ptr<NPC> animal) {
	
	if (!GetSquadJob(animal)) {
		//TODO: Add behaviour for leaderless hostile monsters
		return false;
	}
	return true;
}


void NPC::AddEffect(StatusEffectType effect) {
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->type == effect) {
			statusEffectI->cooldown = statusEffectI->cooldownDefault;
			return;
		}
	}

	statusEffects.push_back(StatusEffect(effect));
}

void NPC::RemoveEffect(StatusEffectType effect) {
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->type == effect) {
			if (statusEffectIterator == statusEffectI) ++statusEffectIterator;
			statusEffects.erase(statusEffectI);
			if (statusEffectIterator == statusEffects.end()) statusEffectIterator = statusEffects.begin();
			return;
		}
	}
}

bool NPC::HasEffect(StatusEffectType effect) {
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->type == effect) {
			return true;
		}
	}
	return false;
}

std::list<StatusEffect>* NPC::StatusEffects() { return &statusEffects; }

void NPC::Hit(boost::weak_ptr<Entity> target) {
	if (target.lock()) {
		if (boost::dynamic_pointer_cast<NPC>(target.lock())) {
			boost::shared_ptr<NPC> npc(boost::static_pointer_cast<NPC>(target.lock()));
			npc->aggressor = boost::static_pointer_cast<NPC>(shared_from_this());
			int dif = ((rand() % 10) + attackSkill) - ((rand() % 10) + npc->defenceSkill);
			if (dif > 0) {
				npc->health -= dif;
#ifdef DEBUG
				std::cout<<"Hit connected, caused "<<dif<<" damage\n";
#endif
				if (npc->health <= 0) npc->Kill();
			} else {
#ifdef DEBUG
				std::cout<<"Hit missed\n";
#endif
			}
		}
	}
}

void NPC::MemberOf(boost::weak_ptr<Squad> newSquad) {squad = newSquad;}
boost::weak_ptr<Squad> NPC::MemberOf() {return squad;}