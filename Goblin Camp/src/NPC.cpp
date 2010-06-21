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

SkillSet::SkillSet() {
	for (int i = 0; i < SKILLAMOUNT; ++i) { skills[i] = 0; }
}

int SkillSet::operator()(Skill skill) {return skills[skill];}
void SkillSet::operator()(Skill skill, int value) {skills[skill] = value;}

NPC::NPC(Coordinate pos, boost::function<bool(boost::shared_ptr<NPC>)> findJob,
            boost::function<void(boost::shared_ptr<NPC>)> react) : GameEntity(),
	timeCount(0),
	taskIndex(0),
	nopath(false),
	findPathWorking(false),
	timer(0),
	_speed(10), nextMove(0),
	taskBegun(false),
	expert(false),
	carried(boost::weak_ptr<Item>()),
	thirst(0),
	hunger(0),
	thinkSpeed(UPDATES_PER_SECOND),
	statusGraphicCounter(0),
	health(100),
	foundItem(boost::weak_ptr<Item>()),
	bag(boost::shared_ptr<Container>(new Container(pos, 0, 10, -1))),
	FindJob(findJob),
	React(react)
{
	while (!GameMap::Inst()->Walkable(pos.x(),pos.y())) {
		pos.x(pos.x()+1);
		pos.y(pos.y()+1);
	}
	Position(pos,true);

	thirst += rand() % 10; //Just for some variety
	hunger += rand() % 10;

	path = new TCODPath(GameMap::Inst()->Width(), GameMap::Inst()->Height(), GameMap::Inst(), 0);

	for (int i = 0; i < NPC_STATUSES; ++i) { status[i] = false; }
}

NPC::~NPC() {
	GameMap::Inst()->NPCList(_x, _y)->erase(uid);
}

void NPC::Position(Coordinate pos, bool firstTime) {
	if (!firstTime) {
		if (GameMap::Inst()->MoveTo(pos.x(), pos.y(), uid)) {
			GameMap::Inst()->MoveFrom(_x, _y, uid);
			_x = pos.x();
			_y = pos.y();
			GameMap::Inst()->MoveTo(_x,_y,uid); //TODO: Figure out why the hell this is required
		}
	} else {
		if (GameMap::Inst()->MoveTo(pos.x(), pos.y(), uid)) {
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

/*
bool NPC::FindJob() {
	boost::shared_ptr<Job> newJob(JobManager::Inst()->GetJob(uid).lock());
	if (newJob)  {
		jobs.push_back(newJob);
		return true;
	}
	return false;
}
*/
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
						if (GameMap::Inst()->Walkable(ix,iy)) {
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

	if (GameMap::Inst()->NPCList(_x,_y)->size() > 1) _bgcolor = TCODColor::darkGrey;
	else _bgcolor = TCODColor::black;

	++statusGraphicCounter;
	if (statusGraphicCounter > 99) statusGraphicCounter = 0;

	++thirst; ++hunger;

	if (thirst >= THIRST_THRESHOLD) status[THIRSTY] = true;
	else status[THIRSTY] = false;
	if (hunger >= HUNGER_THRESHOLD) status[HUNGRY] = true;
	else status[HUNGRY] = false;

	if (thirst > THIRST_THRESHOLD && (rand() % (UPDATES_PER_SECOND*10)) == 0) {
		HandleThirst();
	} else if (thirst > THIRST_THRESHOLD * 5) Kill();
	if (hunger > HUNGER_THRESHOLD && (rand() % (UPDATES_PER_SECOND*10)) == 0) {
		HandleHunger();
	} else if (hunger > HUNGER_THRESHOLD * 10) Kill();

	timeCount += thinkSpeed;
	while (timeCount > UPDATES_PER_SECOND) {

        React(boost::static_pointer_cast<NPC>(shared_from_this()));

		timeCount -= UPDATES_PER_SECOND;
		if (!jobs.empty()) {
			switch(currentTask()->action) {
				case MOVE:
#ifdef DEBUG
                    if (name == "Bee") std::cout<<"Bee move";
#endif
					if ((signed int)_x == currentTarget().x() && (signed int)_y == currentTarget().y()) {
						TaskFinished(TASKSUCCESS);
						break;
					}
					if (!taskBegun) { findPath(currentTarget()); taskBegun = true; }
					result = Move();
					if (result == TASKFAILFATAL || result == TASKFAILNONFATAL) {
						TaskFinished(result, std::string("Could not find path to target")); break;
					} else if (result == PATHEMPTY) {
					    if (!((signed int)_x == currentTarget().x() &&  (signed int)_y == currentTarget().y())) {
					        TaskFinished(TASKFAILFATAL, std::string("No path to target")); break;
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
					}
					result = Move();
					if (result == TASKFAILFATAL || result == TASKFAILNONFATAL) { TaskFinished(result, std::string("Could not find path to target")); break; }
					else if (result == PATHEMPTY) {
					    if (!((signed int)_x == currentTarget().x() &&  (signed int)_y == currentTarget().y())) {
					        TaskFinished(TASKFAILFATAL, std::string("No path to target")); break;
					    }
					}
					break;

				case WAIT:
					++timer;
					if (timer > currentTarget().x()) { timer = 0; TaskFinished(TASKSUCCESS); break; }
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
                    if (!boost::static_pointer_cast<Container>(currentEntity().lock())->AddItem(carried)) Announce::Inst()->AddMsg("Container full!");
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
							if (GameMap::Inst()->GetWater(currentTarget().x(), currentTarget().y()).lock()->Depth() > DRINKABLE_WATER_DEPTH) {
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
                        GameMap::Inst()->Walkable(currentEntity().lock()->x(), currentEntity().lock()->y(), true);
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
                        GameMap::Inst()->Walkable(currentEntity().lock()->x(), currentEntity().lock()->y(), true);
                        Game::Inst()->RemoveNatureObject(boost::static_pointer_cast<NatureObject>(currentEntity().lock()));
                        TaskFinished(TASKSUCCESS);
                    }
                    break;

				default: TaskFinished(TASKFAILFATAL, "*BUG*Unknown task*BUG*"); break;
			}
		} else {
			//Idly meander while no jobs available
			if (status[FLEEING]) {
				bool enemyFound = false;
			    if (jobs.empty() && !nearNpcs.empty()) {
			        boost::shared_ptr<Job> fleeJob(new Job("Flee"));
					fleeJob->internal = true;
					for (std::list<boost::weak_ptr<NPC> >::iterator npci = nearNpcs.begin(); npci != nearNpcs.end(); ++npci) {
						if (npci->lock()->faction != faction) {
							int dx = _x - npci->lock()->_x;
							int dy = _y - npci->lock()->_y;
							if (GameMap::Inst()->Walkable(_x + dx, _y + dy)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x+dx,_y+dy)));
								jobs.push_back(fleeJob);
							} else if (GameMap::Inst()->Walkable(_x + dx, _y)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x+dx,_y)));
								jobs.push_back(fleeJob);
							} else if (GameMap::Inst()->Walkable(_x, _y + dy)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(_x,_y+dy)));
								jobs.push_back(fleeJob);
							}
							enemyFound = true;
							break;
						}
					}
			    }
				if (!enemyFound) status[FLEEING] = false;
			} else if (!FindJob(boost::static_pointer_cast<NPC>(shared_from_this()))) {
				if (rand() % 100 == 0) Position(Coordinate(_x + rand() % 3 - 1, _y + rand() % 3 - 1));
			}
		}
	}
	return AINOTHING;
}

TaskResult NPC::Move() {
	int x,y;
	nextMove += _speed;
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

void NPC::Draw(Coordinate center) {
	int screenx = _x - center.x() + (Game::Inst()->ScreenWidth() / 2);
	int screeny = _y - center.y() + (Game::Inst()->ScreenHeight() / 2);
	if (screenx >= 0 && screenx < Game::Inst()->ScreenWidth() && screeny >= 0 && screeny < Game::Inst()->ScreenHeight()) {
		if (statusGraphicCounter < 10 && status[HUNGRY]) {
			TCODConsole::root->putCharEx(screenx, screeny, TCOD_CHAR_ARROW_S, TCODColor::orange, _bgcolor);
		} else if (statusGraphicCounter >= 10 && statusGraphicCounter < 20 && status[THIRSTY]) {
			TCODConsole::root->putCharEx(screenx, screeny, TCOD_CHAR_ARROW_S, TCODColor::blue, _bgcolor);
		} else if (statusGraphicCounter >= 30 && statusGraphicCounter < 40 && status[FLEEING]) {
			TCODConsole::root->putCharEx(screenx, screeny, '!', TCODColor::white, _bgcolor);
		} else {
			TCODConsole::root->putCharEx(screenx, screeny, _graphic, _color, _bgcolor);
		}
	}
}

void NPC::color(TCODColor value, TCODColor bvalue) { _color = value; _bgcolor = bvalue; }
void NPC::graphic(int value) { _graphic = value; }

bool NPC::Expert() {return expert;}
void NPC::Expert(bool value) {expert = value;}

Coordinate NPC::Position() {return Coordinate(_x,_y);}

bool NPC::Dead() { return health <= 0; }
void NPC::Kill() {
	health = 0;
	_bgcolor = TCODColor::darkRed;
	_color = TCODColor::grey;
	for (int i = 0; i < NPC_STATUSES; ++i) { status[i] = false; }
	while (!jobs.empty()) TaskFinished(TASKFAILFATAL, std::string("Dead"));
	Game::Inst()->CreateItem(Position(), Item::StringToItemType("Corpse"), false);
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
        return foundItem.lock()->GameEntity::Position();
    }
    return currentTask()->target;
}

boost::weak_ptr<GameEntity> NPC::currentEntity() {
    if (currentTask()->entity.lock()) return currentTask()->entity;
    else return boost::weak_ptr<GameEntity>(foundItem.lock());
}


void tFindPath(TCODPath *path, int x0, int y0, int x1, int y1, boost::try_mutex *pathMutex, bool *nopath, bool *findPathWorking) {
	boost::mutex::scoped_lock pathLock(*pathMutex);
	*nopath = !path->compute(x0, y0, x1, y1);
	*findPathWorking = false;
}

bool NPC::JobManagerFinder(boost::shared_ptr<NPC> npc) {
   	boost::shared_ptr<Job> newJob(JobManager::Inst()->GetJob(npc->uid).lock());
	if (newJob)  {
		npc->jobs.push_back(newJob);
		return true;
	}
	return false;
}

void NPC::PlayerNPCReact(boost::shared_ptr<NPC> npc) {
}

void NPC::PeacefulAnimalReact(boost::shared_ptr<NPC> animal) {
    Game::Inst()->FindNearbyNPCs(animal);
    for (std::list<boost::weak_ptr<NPC> >::iterator npci = animal->nearNpcs.begin(); npci != animal->nearNpcs.end(); ++npci) {
        if (npci->lock()->faction != animal->faction) {
            animal->Flee();
        }
    }
}

bool NPC::PeacefulAnimalFindJob(boost::shared_ptr<NPC> animal) {
    return false;
}

void NPC::Flee() {status[FLEEING] = true;}
