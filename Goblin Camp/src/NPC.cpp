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

#include <cstdlib>
#include <string>
#include <boost/serialization/split_member.hpp>
#include <boost/thread/thread.hpp>
#include <boost/multi_array.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <libtcod.hpp>
#ifdef DEBUG
#include <iostream>
#endif

#include "NPC.hpp"
#include "Coordinate.hpp"
#include "JobManager.hpp"
#include "GCamp.hpp"
#include "Game.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "StatusEffect.hpp"
#include "Camp.hpp"
#include "Stockpile.hpp"

SkillSet::SkillSet() {
	for (int i = 0; i < SKILLAMOUNT; ++i) { skills[i] = 0; }
}

int SkillSet::operator()(Skill skill) {return skills[skill];}
void SkillSet::operator()(Skill skill, int value) {skills[skill] = value;}

std::map<std::string, NPCType> NPC::NPCTypeNames = std::map<std::string, NPCType>();
std::vector<NPCPreset> NPC::Presets = std::vector<NPCPreset>();

NPC::NPC(Coordinate pos, boost::function<bool(boost::shared_ptr<NPC>)> findJob,
	boost::function<void(boost::shared_ptr<NPC>)> react) : Entity(),
	type(0),
	timeCount(0),
	taskIndex(0),
	pathIndex(0),
	nopath(false),
	findPathWorking(false),
	timer(0),
	nextMove(0),
	lastMoveResult(TASKCONTINUE),
	run(true),
	taskBegun(false),
	expert(false),
	carried(boost::weak_ptr<Item>()),
	mainHand(boost::weak_ptr<Item>()),
	offHand(boost::weak_ptr<Item>()),
	thirst(0),
	hunger(0),
	weariness(0),
	thinkSpeed(UPDATES_PER_SECOND / 5), //Think 5 times a second
	statusEffects(std::list<StatusEffect>()),
	statusEffectIterator(statusEffects.end()),
	statusGraphicCounter(0),
	health(100),
	maxHealth(100),
	foundItem(boost::weak_ptr<Item>()),
	inventory(boost::shared_ptr<Container>(new Container(pos, 0, 10, -1))),
	needsNutrition(false),
	needsSleep(false),
	hasHands(false),
	aggressive(false),
	coward(false),
	aggressor(boost::weak_ptr<NPC>()),
	dead(false),
	squad(boost::weak_ptr<Squad>()),
	attacks(std::list<Attack>()),
	FindJob(findJob),
	React(react),
	escaped(false)
{
	while (!Map::Inst()->Walkable(pos.X(),pos.Y())) {
		pos.X(pos.X()+1);
		pos.Y(pos.Y()+1);
	}
	Position(pos,true);

	thirst += rand() % 10; //Just for some variety
	hunger += rand() % 10;

	path = new TCODPath(Map::Inst()->Width(), Map::Inst()->Height(), Map::Inst(), (void*)this);

	for (int i = 0; i < STAT_COUNT; ++i) {baseStats[i] = 0; effectiveStats[i] = 0;}
	for (int i = 0; i < RES_COUNT; ++i) {baseResistances[i] = 0; effectiveResistances[i] = 0;}
}

NPC::~NPC() {
	Map::Inst()->NPCList(x, y)->erase(uid);
	if (squad.lock()) squad.lock()->Leave(uid);

	if (boost::iequals(NPC::NPCTypeToString(type), "orc")) Game::Inst()->OrcCount(-1);
	else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) Game::Inst()->GoblinCount(-1);
}

void NPC::Position(Coordinate pos, bool firstTime) {
	if (!firstTime) {
		Map::Inst()->MoveTo(pos.X(), pos.Y(), uid);
		Map::Inst()->MoveFrom(x, y, uid);
		x = pos.X();
		y = pos.Y();
	} else {
		Map::Inst()->MoveTo(pos.X(), pos.Y(), uid);
		x = pos.X();
		y = pos.Y();
	}
}

void NPC::Position(Coordinate pos) { Position(pos, false); }

bool *NPC::visArray() { return _visArray; }

Task* NPC::currentTask() { return jobs.empty() ? 0 : &(jobs.front()->tasks[taskIndex]); }

boost::weak_ptr<Job> NPC::currentJob() { return jobs.empty() ? boost::weak_ptr<Job>() : boost::weak_ptr<Job>(jobs.front()); }

void NPC::TaskFinished(TaskResult result, std::string msg) {
#ifdef DEBUG
	if (msg.size() > 0) {
		std::cout<<msg<<"\n";
	}
#endif
	if (result == TASKSUCCESS) {
		if (++taskIndex >= (signed int)jobs.front()->tasks.size()) {
			jobs.front()->Complete();
			jobs.pop_front();
			taskIndex = 0;
			foundItem = boost::weak_ptr<Item>();
		}
	} else {
		if (!jobs.front()->internal) JobManager::Inst()->CancelJob(jobs.front(), msg, result);
		jobs.pop_front();
		taskIndex = 0;
		DropCarriedItem();
		foundItem = boost::weak_ptr<Item>();
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
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Drink"));
		if (!item.lock()) {tmpCoord = Game::Inst()->FindWater(Position());}
		if (!item.lock() && tmpCoord.X() == -1) { //Nothing to drink!
			//:ohdear:
		} else { //Something to drink!
			boost::shared_ptr<Job> newJob(new Job("Drink", MED, 0, !expert));
			newJob->internal = true;

			if (item.lock()) {
				newJob->ReserveEntity(item);
				newJob->tasks.push_back(Task(MOVE,item.lock()->Position()));
				newJob->tasks.push_back(Task(TAKE,item.lock()->Position(), item));
				newJob->tasks.push_back(Task(DRINK));
				jobs.push_back(newJob);
				run = true;
			} else {
				for (int ix = tmpCoord.X()-1; ix <= tmpCoord.X()+1; ++ix) {
					for (int iy = tmpCoord.Y()-1; iy <= tmpCoord.Y()+1; ++iy) {
						if (Map::Inst()->Walkable(ix,iy,(void*)this)) {
							newJob->tasks.push_back(Task(MOVE, Coordinate(ix,iy)));
							goto CONTINUEDRINKBLOCK;
						}
					}
				}
CONTINUEDRINKBLOCK:
				newJob->tasks.push_back(Task(DRINK, tmpCoord));
				jobs.push_back(newJob);
				run = true;
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

			newJob->ReserveEntity(item);
			newJob->tasks.push_back(Task(MOVE,item.lock()->Position()));
			newJob->tasks.push_back(Task(TAKE,item.lock()->Position(), item));
			newJob->tasks.push_back(Task(EAT));
			jobs.push_back(newJob);
			run = true;
		}
	}
}

void NPC::HandleWeariness() {
	bool found = false;
	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Sleep") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Construction> wbed = Game::Inst()->FindConstructionByTag(BED);
		if (boost::shared_ptr<Construction> bed = wbed.lock()) {
			run = true;
			boost::shared_ptr<Job> sleepJob(new Job("Sleep"));
			sleepJob->internal = true;
			sleepJob->ReserveEntity(bed);
			sleepJob->tasks.push_back(Task(MOVE, bed->Position()));
			sleepJob->tasks.push_back(Task(SLEEP, bed->Position(), bed));
			jobs.push_back(sleepJob);
			return;
		}
		boost::shared_ptr<Job> sleepJob(new Job("Sleep"));
		sleepJob->internal = true;
		sleepJob->tasks.push_back(Task(SLEEP, Position()));
		jobs.push_back(sleepJob);
	}
}

void NPC::Update() {
	if (Map::Inst()->NPCList(x,y)->size() > 1) _bgcolor = TCODColor::darkGrey;
	else _bgcolor = TCODColor::black;

	UpdateStatusEffects();

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

	if (needsSleep) {
		++weariness;

		if (weariness >= WEARY_THRESHOLD) AddEffect(DROWSY);
		else RemoveEffect(DROWSY);

		if (weariness > WEARY_THRESHOLD) HandleWeariness();
	}

	for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
		attacki->Update();
	}

	if (rand() % UPDATES_PER_SECOND == 0 && health < maxHealth) ++health;
}

void NPC::UpdateStatusEffects() {

	for (int i = 0; i < STAT_COUNT; ++i) {
		effectiveStats[i] = baseStats[i];
	}
	for (int i = 0; i < STAT_COUNT; ++i) {
		effectiveResistances[i] = baseResistances[i];
	}
	++statusGraphicCounter;
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		//Apply effects to stats
		for (int i = 0; i < STAT_COUNT; ++i) {
			effectiveStats[i] = (int)(effectiveStats[i] * statusEffectI->statChanges[i]);
		}
		for (int i = 0; i < STAT_COUNT; ++i) {
			effectiveResistances[i] = (int)(effectiveResistances[i] * statusEffectI->resistanceChanges[i]);
		}

		if (statusEffectI->damage.second > 0 && --statusEffectI->damage.first <= 0) {
			statusEffectI->damage.first = UPDATES_PER_SECOND;
			health -= statusEffectI->damage.second;
			if (statusEffectI->bleed) Game::Inst()->CreateBlood(Position());
		}

		//Remove the statuseffect if it's cooldown has run out
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

}

AiThink NPC::Think() {
	Coordinate tmpCoord;
	int tmp;

	lastMoveResult = Move(lastMoveResult);

	timeCount += thinkSpeed;
	while (timeCount > UPDATES_PER_SECOND) {

		if (rand() % 2 == 0) React(boost::static_pointer_cast<NPC>(shared_from_this()));

		if (aggressor.lock())
			if (Game::Inst()->Adjacent(Position(), aggressor)) Hit(aggressor);

		timeCount -= UPDATES_PER_SECOND;
		if (!jobs.empty()) {
			switch(currentTask()->action) {
			case MOVE:
				if (!Map::Inst()->Walkable(currentTarget().X(), currentTarget().Y(), (void*)this)) {
					TaskFinished(TASKFAILFATAL, "MOVE target unwalkable");
					break;
				}
				if ((signed int)x == currentTarget().X() && (signed int)y == currentTarget().Y()) {
					TaskFinished(TASKSUCCESS);
					break;
				}
				if (!taskBegun) { findPath(currentTarget()); taskBegun = true; lastMoveResult = TASKCONTINUE;}

				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) {
					TaskFinished(lastMoveResult, std::string("Could not find path to target")); break;
				} else if (lastMoveResult == PATHEMPTY) {
					if (!((signed int)x == currentTarget().X() && (signed int)y == currentTarget().Y())) {
						TaskFinished(TASKFAILFATAL, std::string("No path to target")); break;
					}
				}
				break;

			case MOVENEAR:
				/*MOVENEAR first figures out our "real" target, which is a tile near
				to our current target. Near means max 5 tiles away, visible and
				walkable. Once we have our target we can actually switch over
				to a normal MOVE task. In case we can't find a visible tile,
				we'll allow a non LOS one*/
				tmp = 0;
				{bool checkLOS = true;
				for (int i = 0; i < 2; ++i) {
					while (tmp++ < 10) {
						int tarX = ((rand() % 11) - 5) + currentTarget().X();
						int tarY = ((rand() % 11) - 5) + currentTarget().Y();
						if (tarX < 0) tarX = 0;
						if (tarX >= Map::Inst()->Width()) tarX = Map::Inst()->Width()-1;
						if (tarY < 0) tarY = 0;
						if (tarY >= Map::Inst()->Height()) tarY = Map::Inst()->Height()-1;
						if (Map::Inst()->Walkable(tarX, tarY, (void *)this)) {
							if (!checkLOS || (checkLOS && 
								Map::Inst()->LineOfSight(tarX, tarY, currentTarget().X(), currentTarget().Y()))) {
								currentJob().lock()->tasks[taskIndex] = Task(MOVE, Coordinate(tarX, tarY));
								goto MOVENEARend;
							}
						}
					}
					checkLOS = !checkLOS;
				}}
				//If we got here we couldn't find a near coordinate
				TaskFinished(TASKFAILFATAL);
MOVENEARend:
				break;


			case MOVEADJACENT:
				if (Game::Inst()->Adjacent(Position(), currentEntity())) {
					TaskFinished(TASKSUCCESS);
					break;
				}
				if (!taskBegun) {
					tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity());
					if (tmpCoord.X() >= 0) {
						findPath(tmpCoord);
					} else { TaskFinished(TASKFAILFATAL, std::string("No walkable adjacent tiles")); break; }
					taskBegun = true;
					lastMoveResult = TASKCONTINUE;
				}
				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) { TaskFinished(lastMoveResult, std::string("Could not find path to target")); break; }
				else if (lastMoveResult == PATHEMPTY) {
					TaskFinished(TASKFAILFATAL);
				}
				break;

			case WAIT:
				if (++timer > currentTarget().X()) { timer = 0; TaskFinished(TASKSUCCESS); }
				break;

			case BUILD:
				if (Game::Inst()->Adjacent(Position(), currentEntity())) {
					tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Build();
					if (tmp > 0) {
						Announce::Inst()->AddMsg((boost::format("%s completed") % currentEntity().lock()->Name()).str());
						Camp::Inst()->UpdateCenter(currentEntity().lock()->Position());
						TaskFinished(TASKSUCCESS);
						break;
					} else if (tmp == BUILD_NOMATERIAL) {
						TaskFinished(TASKFAILFATAL, "Missing materials");
						break;
					}
				} else {
					TaskFinished(TASKFAILFATAL);
					break;
				}
				break;

			case TAKE:
				if (!currentEntity().lock()) { TaskFinished(TASKFAILFATAL, "No target entity for TAKE"); break; }
				if (Position() == currentEntity().lock()->Position()) {
					if (boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()) {
						boost::weak_ptr<Container> cont(boost::static_pointer_cast<Container>(boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()));
						cont.lock()->RemoveItem(
							boost::static_pointer_cast<Item>(currentEntity().lock()));
					}
					carried = boost::static_pointer_cast<Item>(currentEntity().lock());
					if (!inventory->AddItem(carried)) Announce::Inst()->AddMsg("No space in inventory");
					TaskFinished(TASKSUCCESS);
					break;
				} else { TaskFinished(TASKFAILFATAL, "Item not found"); break; }
				break;

			case DROP:
				DropCarriedItem();
				TaskFinished(TASKSUCCESS);
				break;

			case PUTIN:
				if (carried.lock()) {
					inventory->RemoveItem(carried);
					carried.lock()->Position(Position());
					if (boost::dynamic_pointer_cast<Container>(currentEntity().lock())) {
						boost::shared_ptr<Container> cont = boost::static_pointer_cast<Container>(currentEntity().lock());
						if (!cont->AddItem(carried)) Announce::Inst()->AddMsg("Container full!");
					} else {
						DropCarriedItem();
						TaskFinished(TASKFAILFATAL);
						break;
					}
				}
				carried.reset();
				TaskFinished(TASKSUCCESS);
				break;

			case DRINK: //Either we have an item target to drink, or a water tile
				if (carried.lock()) { //Drink from an item
					thirst -= boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
					inventory->RemoveItem(carried);
					Game::Inst()->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
					TaskFinished(TASKSUCCESS);
					break;
				} else { //Drink from a water tile
					if (std::abs((signed int)x - currentTarget().X()) <= 1 &&
						std::abs((signed int)y - currentTarget().Y()) <= 1) {
							if (boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(currentTarget().X(), currentTarget().Y()).lock()) {
								if (water->Depth() > DRINKABLE_WATER_DEPTH) {
									thirst -= (int)(THIRST_THRESHOLD / 10);
									if (thirst < 0) { TaskFinished(TASKSUCCESS); }
									break;
								}
							}
					}
					TaskFinished(TASKFAILFATAL);
				}
				break;

			case EAT:
				if (carried.lock()) {
					hunger -= boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
					inventory->RemoveItem(carried);

					for (std::list<ItemType>::iterator fruiti = Item::Presets[carried.lock()->Type()].fruits.begin(); fruiti != Item::Presets[carried.lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}

					Game::Inst()->RemoveItem(carried);
				}
				carried = boost::weak_ptr<Item>();
				TaskFinished(TASKSUCCESS);
				break;

			case FIND:
				foundItem = Game::Inst()->FindItemByCategoryFromStockpiles(currentTask()->item, currentTask()->flags);
				if (!foundItem.lock()) {
					TaskFinished(TASKFAILFATAL); 
#ifdef DEBUG
					std::cout<<"Can't FIND required item\n";
#endif
					break;
				}
				else {
					if (faction == 0) currentJob().lock()->ReserveEntity(foundItem);
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
						TaskFinished(TASKFAILFATAL); break;
					}
				} else { TaskFinished(TASKFAILFATAL, "Attempted to use non-construct"); break; }
				break;

			case HARVEST:
				if (carried.lock()) {
					for (std::list<ItemType>::iterator fruiti = Item::Presets[carried.lock()->Type()].fruits.begin(); fruiti != Item::Presets[carried.lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}
					inventory->RemoveItem(carried);
					Game::Inst()->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
					TaskFinished(TASKSUCCESS);
					break;
				} else {
					inventory->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
					TaskFinished(TASKFAILFATAL, "Carrying nonexistant item");
					break;
				}

			case FELL:
				if (boost::shared_ptr<NatureObject> tree = boost::static_pointer_cast<NatureObject>(currentEntity().lock())) {
					tmp = tree->Fell();
					if (tmp <= 0) {
						for (std::list<ItemType>::iterator iti = NatureObject::Presets[tree->Type()].components.begin(); iti != NatureObject::Presets[tree->Type()].components.end(); ++iti) {
							Game::Inst()->CreateItem(tree->Position(), *iti, true);
						}
						Map::Inst()->SetWalkable(tree->X(), tree->Y(), true);
						Map::Inst()->Buildable(tree->X(), tree->Y(), true);
						Game::Inst()->RemoveNatureObject(tree);
						TaskFinished(TASKSUCCESS);
						break;
					}
					//Job underway
					break;
				}
				TaskFinished(TASKFAILFATAL);
				break;

			case HARVESTWILDPLANT:
				if (boost::shared_ptr<NatureObject> plant = boost::static_pointer_cast<NatureObject>(currentEntity().lock())) {
					tmp = plant->Harvest();
					if (tmp <= 0) {
						for (std::list<ItemType>::iterator iti = NatureObject::Presets[plant->Type()].components.begin(); iti != NatureObject::Presets[plant->Type()].components.end(); ++iti) {
							Game::Inst()->CreateItem(plant->Position(), *iti, true);
						}
						Map::Inst()->SetWalkable(plant->X(), plant->Y(), true);
						Map::Inst()->Buildable(plant->X(), plant->Y(), true);
						Game::Inst()->RemoveNatureObject(plant);
						TaskFinished(TASKSUCCESS);
						break;
					}
					//Job underway
					break;
				}
				TaskFinished(TASKFAILFATAL);
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
					tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity());
					if (tmpCoord.X() >= 0) {
						findPath(tmpCoord);
					}
					taskBegun = true;
					lastMoveResult = TASKCONTINUE;
				}

				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) { TaskFinished(lastMoveResult, std::string("Could not find path to target")); break; }
				else if (lastMoveResult == PATHEMPTY) {
					findPath(currentEntity().lock()->Position());
				}
				break;

			case FLEEMAP:
				if (x == 0 || x == Map::Inst()->Width()-1 ||
					y == 0 || y == Map::Inst()->Height()-1) {
						//We are at the edge, escape!
						Escape();
						return AIMOVE;
				}

				//Find the closest edge and change into a MOVE task and a new FLEEMAP task
				//Unfortunately this assumes that FLEEMAP is the last task in a job,
				//which might not be.
				tmp = std::abs((signed int)x - Map::Inst()->Width() / 2);
				if (tmp > std::abs((signed int)y - Map::Inst()->Height() / 2)) {
					currentJob().lock()->tasks[taskIndex] = Task(MOVE, Coordinate(x, 
						(y < (unsigned int)Map::Inst()->Height() / 2) ? 0 : Map::Inst()->Height()-1));
				} else {
					currentJob().lock()->tasks[taskIndex] = Task(MOVE, 
						Coordinate(((unsigned int)Map::Inst()->Width() / 2) ? 0 : Map::Inst()->Width()-1, 
						y));
				}
				currentJob().lock()->tasks.push_back(Task(FLEEMAP));
				break;

			case SLEEP:
				AddEffect(SLEEPING);
				AddEffect(BADSLEEP);
				weariness -= 50;
				if (weariness <= 0) {
					if (boost::shared_ptr<Entity> entity = currentEntity().lock()) {
						if (boost::static_pointer_cast<Construction>(entity)->HasTag(BED)) {
							RemoveEffect(BADSLEEP);
						}
					}
					TaskFinished(TASKSUCCESS);
					break;
				}
				break;

			case DISMANTLE:
				if (boost::shared_ptr<Construction> construct = boost::static_pointer_cast<Construction>(currentEntity().lock())) {
					construct->Condition(construct->Condition()-10);
					if (construct->Condition() <= 0) {
						Game::Inst()->RemoveConstruction(construct);
						TaskFinished(TASKSUCCESS);
						break;
					}
				}
				break;

			case WIELD:
				if (carried.lock()) {
					if (mainHand.lock()) { //Drop currently wielded weapon if such exists
						inventory->RemoveItem(mainHand);
						mainHand.lock()->Position(Position());
						mainHand.lock()->PutInContainer(boost::weak_ptr<Item>());
						mainHand.reset();
					}
					mainHand = carried;
					carried.reset();
					TaskFinished(TASKSUCCESS);
#ifdef DEBUG
					std::cout<<"Wielded "<<mainHand.lock()->Name()<<" successfully\n";
#endif
					break;
				}
				TaskFinished(TASKFAILFATAL);
				break;

			default: TaskFinished(TASKFAILFATAL, "*BUG*Unknown task*BUG*"); break;
			}
		} else {
			if (HasEffect(PANIC)) {
				bool enemyFound = false;
				if (jobs.empty() && !nearNpcs.empty()) {
					boost::shared_ptr<Job> fleeJob(new Job("Flee"));
					fleeJob->internal = true;
					run = true;
					for (std::list<boost::weak_ptr<NPC> >::iterator npci = nearNpcs.begin(); npci != nearNpcs.end(); ++npci) {
						if (npci->lock() && npci->lock()->faction != faction) {
							int dx = x - npci->lock()->x;
							int dy = y - npci->lock()->y;
							if (Map::Inst()->Walkable(x + dx, y + dy, (void *)this)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(x+dx,y+dy)));
								jobs.push_back(fleeJob);
							} else if (Map::Inst()->Walkable(x + dx, y, (void *)this)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(x+dx,y)));
								jobs.push_back(fleeJob);
							} else if (Map::Inst()->Walkable(x, y + dy, (void *)this)) {
								fleeJob->tasks.push_back(Task(MOVE, Coordinate(x,y+dy)));
								jobs.push_back(fleeJob);
							}
							enemyFound = true;
							break;
						}
					}
				}
			} else if (!GetSquadJob(boost::static_pointer_cast<NPC>(shared_from_this())) && 
				!FindJob(boost::static_pointer_cast<NPC>(shared_from_this()))) {
					boost::shared_ptr<Job> idleJob(new Job("Idle"));
					idleJob->internal = true;
					idleJob->tasks.push_back(Task(MOVENEAR, faction == 0 ? Camp::Inst()->Center() : Position()));
					idleJob->tasks.push_back(Task(WAIT, Coordinate(rand() % 10, 0)));
					jobs.push_back(idleJob);
					if (Distance(Camp::Inst()->Center().X(), Camp::Inst()->Center().Y(), x, y) < 15) run = false;
					else run = true;
			}
		}
	}

	return AINOTHING;
}

TaskResult NPC::Move(TaskResult oldResult) {
	int moveX,moveY;
	nextMove += run ? effectiveStats[MOVESPEED] : effectiveStats[MOVESPEED]/3;
	while (nextMove > 100) {
		nextMove -= 100;
		boost::mutex::scoped_try_lock pathLock(pathMutex);
		if (pathLock.owns_lock()) {
			if (nopath) {nopath = false; return TASKFAILFATAL;}
			if (pathIndex < path->size()) {
				path->get(pathIndex, &moveX, &moveY);
				if (Map::Inst()->Walkable(moveX, moveY, (void*)this)) {
					Position(Coordinate(moveX,moveY));
					++pathIndex;
				} else {
					return TASKFAILNONFATAL;
				}
				return TASKCONTINUE;
			} else if (!findPathWorking) return PATHEMPTY;
		}
	}
	//Can't move yet, so the earlier result is still valid
	return oldResult;
}

void NPC::findPath(Coordinate target) {
	findPathWorking = true;
	pathIndex = 0;
	boost::thread pathThread(boost::bind(tFindPath, path, x, y, target.X(), target.Y(), &pathMutex, &nopath, &findPathWorking));
}

void NPC::speed(unsigned int value) {baseStats[MOVESPEED]=value;}
unsigned int NPC::speed() {return effectiveStats[MOVESPEED];}

void NPC::Draw(Coordinate upleft, TCODConsole *console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
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

Coordinate NPC::Position() {return Coordinate(x,y);}

bool NPC::Dead() { return dead; }
void NPC::Kill() {
	if (!dead) {//You can't be killed if you're already dead!
		dead = true;
		health = 0;
		int corpse = Game::Inst()->CreateItem(Position(), Item::StringToItemType("Corpse"), false);
		Game::Inst()->GetItem(corpse).lock()->Color(_color);
		Game::Inst()->GetItem(corpse).lock()->Name(Game::Inst()->GetItem(corpse).lock()->Name() + "(" + name + ")");
		while (!jobs.empty()) TaskFinished(TASKFAILFATAL, std::string("dead"));
		if (boost::shared_ptr<Item> weapon = mainHand.lock()) {
			weapon->Position(Position());
			weapon->PutInContainer();
			mainHand.reset();
		}

		if (boost::iequals(NPC::NPCTypeToString(type), "orc")) Announce::Inst()->AddMsg("An orc has died!", TCODColor::red);
		else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) Announce::Inst()->AddMsg("A goblin has died!", TCODColor::red);
	}
}

void NPC::DropCarriedItem() {
	if (carried.lock()) {
		inventory->RemoveItem(carried);
		carried.lock()->Position(Position());
		carried.lock()->PutInContainer(boost::weak_ptr<Item>());
		carried.reset();
	}
}

Coordinate NPC::currentTarget() {
	if (currentTask()->target == Coordinate(0,0) && foundItem.lock()) {
		return foundItem.lock()->Position();
	}
	return currentTask()->target;
}

boost::weak_ptr<Entity> NPC::currentEntity() {
	if (currentTask()->entity.lock()) return currentTask()->entity;
	else if (foundItem.lock()) return boost::weak_ptr<Entity>(foundItem.lock());
	return boost::weak_ptr<Entity>();
}


void tFindPath(TCODPath *path, int x0, int y0, int x1, int y1, boost::try_mutex *pathMutex, bool *nopath, bool *findPathWorking) {
	boost::mutex::scoped_lock pathLock(*pathMutex);
	*nopath = !path->compute(x0, y0, x1, y1);
	*findPathWorking = false;
}

bool NPC::GetSquadJob(boost::shared_ptr<NPC> npc) {
	if (boost::shared_ptr<Squad> squad = npc->MemberOf().lock()) {
		npc->aggressive = true;
		boost::shared_ptr<Job> newJob(new Job("Follow orders"));
		newJob->internal = true;

		//Priority #1, if the creature can wield a weapon get one if possible
		/*TODO: Right now this only makes friendlies take a weapon from a stockpile
		It should be expanded to allow all npc's to search for nearby weapons lying around. */
		if (!npc->mainHand.lock() && npc->Faction() == 0 && squad->Weapon() >= 0) {
			for (std::list<Attack>::iterator attacki = npc->attacks.begin(); attacki != npc->attacks.end();
				++attacki) {
					if (attacki->Type() == DAMAGE_WIELDED) {
						if (Game::Inst()->FindItemByCategoryFromStockpiles(
							squad->Weapon()).lock()) {
								newJob->tasks.push_back(Task(FIND, Coordinate(0,0), boost::shared_ptr<Entity>(), 
									squad->Weapon()));
								newJob->tasks.push_back(Task(MOVE));
								newJob->tasks.push_back(Task(TAKE));
								newJob->tasks.push_back(Task(WIELD));
								npc->jobs.push_back(newJob);
								npc->run = true;
								return true;
						}
						break;
					}
			}
		}

		switch (squad->Order()) {
		case GUARD:
			if (squad->TargetCoordinate().X() >= 0) {
				newJob->tasks.push_back(Task(MOVENEAR, squad->TargetCoordinate()));
				//WAIT waits Coordinate.x / 5 seconds
				newJob->tasks.push_back(Task(WAIT, Coordinate(5*5, 0)));
				npc->jobs.push_back(newJob);
				if (Distance(npc->Position(), squad->TargetCoordinate()) < 10) npc->run = false;
				else npc->run = true;
				return true;
			}
			break;

		case ESCORT:
			if (squad->TargetEntity().lock()) {
				newJob->tasks.push_back(Task(MOVENEAR, squad->TargetEntity().lock()->Position(), squad->TargetEntity()));
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
	if (!npc->MemberOf().lock()) {
		boost::shared_ptr<Job> newJob(JobManager::Inst()->GetJob(npc->uid).lock());
		if (newJob)  {
			npc->jobs.push_back(newJob);
			npc->run = true;
			return true;
		}
	}
	return false;
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
	} else if (npc->coward) { //Aggressiveness trumps cowardice
		Game::Inst()->FindNearbyNPCs(npc);
		for (std::list<boost::weak_ptr<NPC> >::iterator npci = npc->nearNpcs.begin(); npci != npc->nearNpcs.end(); ++npci) {
			if (npci->lock()->Faction() != npc->faction && npci->lock()->aggressive) {
				while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILNONFATAL);
				npc->AddEffect(PANIC);
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
	animal->aggressive = false;
	if (animal->aggressor.lock() && NPC::Presets[animal->type].tags.find("angers") != NPC::Presets[animal->type].tags.end()) {
		//Turn into a hostile animal if attacked by the player's creatures
		if (animal->aggressor.lock()->Faction() == 0) animal->FindJob = boost::bind(NPC::HostileAnimalFindJob, _1);
	}
	return false;
}

void NPC::HostileAnimalReact(boost::shared_ptr<NPC> animal) {
	animal->aggressive = true;
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
	//For now hostile animals will simply attempt to get inside the settlement
	if (animal->Position() != Camp::Inst()->Center()) {
		boost::shared_ptr<Job> attackJob(new Job("Attack settlement"));
		attackJob->internal = true;
		attackJob->tasks.push_back(Task(MOVENEAR, Camp::Inst()->Center()));
		animal->jobs.push_back(attackJob);
		return true;
	}
	return false;
}

//A hungry animal will attempt to find food in the player's stockpiles and eat it,
//alternatively it will change into a "normal" hostile animal if no food is available
bool NPC::HungryAnimalFindJob(boost::shared_ptr<NPC> animal) {
	//We could use Task(FIND for this, but it doesn't give us feedback if there's
	//any food available
	boost::weak_ptr<Item> wfood = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Food"));
	if (boost::shared_ptr<Item> food = wfood.lock()) {
		//Found a food item
		boost::shared_ptr<Job> stealJob(new Job("Steal food"));
		stealJob->internal = true;
		stealJob->tasks.push_back(Task(MOVE, food->Position()));
		stealJob->tasks.push_back(Task(TAKE, food->Position(), food));
		stealJob->tasks.push_back(Task(FLEEMAP));
		animal->jobs.push_back(stealJob);
		return true;
	} else {
		animal->FindJob = boost::bind(NPC::HostileAnimalFindJob, _1);
	}
	return false;
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

			for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
				if (attacki->Cooldown() <= 0) {
					attacki->ResetCooldown();
					//First check if the target dodges the attack
					if (rand() % 100 < npc->effectiveStats[DODGE]) {
#ifdef DEBUG
						std::cout<<npc->name<<"("<<npc->uid<<") dodged\n";
#endif
						continue;
					}

					Attack attack = *attacki;
#ifdef DEBUG
					std::cout<<"attack.addsub: "<<attack.Amount().addsub<<"\n";
#endif
					
					if (attack.Type() == DAMAGE_WIELDED) {
#ifdef DEBUG
						std::cout<<"Wielded attack\n";
#endif
						GetMainHandAttack(attack);
					}
#ifdef DEBUG
					std::cout<<"attack.addsub after: "<<attack.Amount().addsub<<"\n";
#endif

					Resistance res;
					switch (attacki->Type()) {
					case DAMAGE_SLASH:
					case DAMAGE_PIERCE:
					case DAMAGE_BLUNT: res = PHYSICAL_RES; break;

					case DAMAGE_MAGIC: res = MAGIC_RES; break;

					case DAMAGE_FIRE: res = FIRE_RES; break;

					case DAMAGE_COLD: res = COLD_RES; break;

					case DAMAGE_POISON: res = POISON_RES; break;

					default: res = PHYSICAL_RES; break;
					}
						
					double resistance = (100.0 - (float)npc->effectiveResistances[res]) / 100.0;
					int damage = (int)(Game::DiceToInt(attack.Amount()) * resistance);
					npc->health -= damage;

#ifdef DEBUG
					std::cout<<"Resistance: "<<resistance<<"\n";
					std::cout<<name<<"("<<uid<<") inflicted "<<damage<<" damage\n";
#endif

					for (unsigned int effecti = 0; effecti < attack.StatusEffects()->size(); ++effecti) {
						if (rand() % 100 < attack.StatusEffects()->at(effecti).second) {
							npc->AddEffect(attack.StatusEffects()->at(effecti).first);
						}
					}

					if (npc->health <= 0) npc->Kill();

					if (damage > 0) {
						Game::Inst()->CreateBlood(Coordinate(npc->Position().X() + ((rand() % 3) - 1), 
							npc->Position().Y() + ((rand() % 3) - 1)));
						npc->aggressor = boost::static_pointer_cast<NPC>(shared_from_this());
					}
				}
			}
		}
	}
}

void NPC::MemberOf(boost::weak_ptr<Squad> newSquad) {
	squad = newSquad;
	if (!squad.lock()) { //NPC was removed from a squad
		if (boost::shared_ptr<Item> weapon = mainHand.lock()) {
			inventory->RemoveItem(weapon);
			weapon->Position(Position());
			weapon->PutInContainer();
			mainHand.reset();
		}
		aggressive = false;
	}
}
boost::weak_ptr<Squad> NPC::MemberOf() {return squad;}

void NPC::Escape() {
	DestroyAllItems();
	escaped = true;
}

void NPC::DestroyAllItems() {
	if (carried.lock()) Game::Inst()->RemoveItem(carried);
}

bool NPC::Escaped() { return escaped; }

class NPCListener : public ITCODParserListener {
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<boost::format("new %s structure: ") % str->getName();
#endif
		if (boost::iequals(str->getName(), "npc_type")) {
			NPC::Presets.push_back(NPCPreset(name));
			NPC::NPCTypeNames[name] = NPC::Presets.size()-1;
#ifdef DEBUG
			std::cout<<name<<"\n";
#endif
		} else if (boost::iequals(str->getName(), "attack")) {
			NPC::Presets.back().attacks.push_back(Attack());
#ifdef DEBUG
			std::cout<<name<<"\n";
#endif
		} else if (boost::iequals(str->getName(), "resistances")) {
#ifdef DEBUG
			std::cout<<"\n";
#endif
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name,"generateName")) { NPC::Presets.back().generateName = true; }
		else if (boost::iequals(name,"needsNutrition")) { NPC::Presets.back().needsNutrition = true; }
		else if (boost::iequals(name,"needsSleep")) { NPC::Presets.back().needsSleep = true; }
		else if (boost::iequals(name,"expert")) { NPC::Presets.back().expert = true; }
		else if (boost::iequals(name,"ranged")) { NPC::Presets.back().attacks.back().Ranged(true); }
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name,"name")) { NPC::Presets.back().name = value.s; }
		else if (boost::iequals(name,"plural")) { NPC::Presets.back().plural = value.s; }
		else if (boost::iequals(name,"speed")) { NPC::Presets.back().stats[MOVESPEED] = value.i; }
		else if (boost::iequals(name,"color")) { NPC::Presets.back().color = value.col; }
		else if (boost::iequals(name,"graphic")) { NPC::Presets.back().graphic = value.c; }
		else if (boost::iequals(name,"health")) { NPC::Presets.back().health = value.i; }
		else if (boost::iequals(name,"AI")) { NPC::Presets.back().ai = value.s; }
		else if (boost::iequals(name,"dodge")) { NPC::Presets.back().stats[DODGE] = value.i; }
		else if (boost::iequals(name,"spawnAsGroup")) { 
			NPC::Presets.back().spawnAsGroup = true;
			NPC::Presets.back().group = value.dice;
		} else if (boost::iequals(name,"type")) {
			NPC::Presets.back().attacks.back().Type(Attack::StringToDamageType(value.s));
		} else if (boost::iequals(name,"damage")) {
			NPC::Presets.back().attacks.back().Amount(value.dice);
		} else if (boost::iequals(name,"cooldown")) {
			NPC::Presets.back().attacks.back().CooldownMax(value.i);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NPC::Presets.back().attacks.back().StatusEffects()->push_back(std::pair<StatusEffectType, int>(StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i)), 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NPC::Presets.back().attacks.back().StatusEffects()->at(i).second = (int)TCOD_list_get(value.list,i);
			}
		} else if (boost::iequals(name,"projectile")) {
			NPC::Presets.back().attacks.back().Projectile(Item::StringToItemType(value.s));
		} else if (boost::iequals(name,"physical")) {
			NPC::Presets.back().resistances[PHYSICAL_RES] = value.i;
		} else if (boost::iequals(name,"magic")) {
			NPC::Presets.back().resistances[MAGIC_RES] = value.i;
		} else if (boost::iequals(name,"cold")) {
			NPC::Presets.back().resistances[COLD_RES] = value.i;
		} else if (boost::iequals(name,"fire")) {
			NPC::Presets.back().resistances[FIRE_RES] = value.i;
		} else if (boost::iequals(name,"poison")) {
			NPC::Presets.back().resistances[POISON_RES] = value.i;
		} else if (boost::iequals(name,"tags")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				std::string tag = (char*)TCOD_list_get(value.list,i);
				NPC::Presets.back().tags.insert(boost::to_lower_copy(tag));
			}
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<boost::format("end of %s\n") % str->getName();
#endif
		if (NPC::Presets.back().plural == "") NPC::Presets.back().plural = NPC::Presets.back().name + "s";
		return true;
	}
	void error(const char *msg) {
		Logger::Inst()->output<<"NPCListener: "<<msg<<"\n";
		Game::Inst()->Exit(false);
	}
};

void NPC::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct *npcTypeStruct = parser.newStructure("npc_type");
	npcTypeStruct->addProperty("name", TCOD_TYPE_STRING, true);
	npcTypeStruct->addProperty("plural", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("speed", TCOD_TYPE_INT, true);
	npcTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	npcTypeStruct->addProperty("graphic", TCOD_TYPE_CHAR, true);
	npcTypeStruct->addFlag("expert");
	npcTypeStruct->addProperty("health", TCOD_TYPE_INT, true);
	const char* aiTypes[] = { "PlayerNPC", "PeacefulAnimal", "HungryAnimal", "HostileAnimal", NULL }; 
	npcTypeStruct->addValueList("AI", aiTypes, true);
	npcTypeStruct->addFlag("needsNutrition");
	npcTypeStruct->addFlag("needsSleep");
	npcTypeStruct->addFlag("generateName");
	npcTypeStruct->addProperty("dodge", TCOD_TYPE_INT, true);
	npcTypeStruct->addProperty("spawnAsGroup", TCOD_TYPE_DICE, false);
	npcTypeStruct->addListProperty("tags", TCOD_TYPE_STRING, false);
	
	TCODParserStruct *attackTypeStruct = parser.newStructure("attack");
	const char* damageTypes[] = { "slashing", "piercing", "blunt", "magic", "fire", "cold", "poison", "wielded", NULL };
	attackTypeStruct->addValueList("type", damageTypes, true);
	attackTypeStruct->addProperty("damage", TCOD_TYPE_DICE, false);
	attackTypeStruct->addProperty("cooldown", TCOD_TYPE_INT, false);
	attackTypeStruct->addListProperty("statusEffects", TCOD_TYPE_STRING, false);
	attackTypeStruct->addListProperty("effectChances", TCOD_TYPE_INT, false);
	attackTypeStruct->addFlag("ranged");
	attackTypeStruct->addProperty("projectile", TCOD_TYPE_STRING, false);

	TCODParserStruct *resistancesStruct = parser.newStructure("resistances");
	resistancesStruct->addProperty("physical", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("magic", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("cold", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("fire", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("poison", TCOD_TYPE_INT, false);

	npcTypeStruct->addStructure(attackTypeStruct);
	npcTypeStruct->addStructure(resistancesStruct);

	parser.run(filename.c_str(), new NPCListener());
}

std::string NPC::NPCTypeToString(NPCType type) {
	return Presets[type].typeName;
}

NPCType NPC::StringToNPCType(std::string typeName) {
	return NPCTypeNames[typeName];
}

void NPC::InitializeAIFunctions() {
	if (NPC::Presets[type].ai == "PlayerNPC") {
		FindJob = boost::bind(NPC::JobManagerFinder, _1);
		React = boost::bind(NPC::PlayerNPCReact, _1);
		faction = 0;
	} else if (NPC::Presets[type].ai == "PeacefulAnimal") {
		FindJob = boost::bind(NPC::PeacefulAnimalFindJob, _1);
		React = boost::bind(NPC::PeacefulAnimalReact, _1);
		faction = 1;
	} else if (NPC::Presets[type].ai == "HungryAnimal") {
		FindJob = boost::bind(NPC::HungryAnimalFindJob, _1);
		React = boost::bind(NPC::HostileAnimalReact, _1);
		faction = 2;
	} else if (NPC::Presets[type].ai == "HostileAnimal") {
		FindJob = boost::bind(NPC::HostileAnimalFindJob, _1);
		React = boost::bind(NPC::HostileAnimalReact, _1);
		faction = 2;
	}
}

void NPC::GetMainHandAttack(Attack &attack) {
	attack.Type(DAMAGE_BLUNT);
	if (boost::shared_ptr<Item> weapon = mainHand.lock()) {
		Attack wAttack = weapon->GetAttack();
		attack.Type(wAttack.Type());
		attack.AddDamage(wAttack.Amount());
		attack.Ranged(wAttack.Ranged());
		attack.Projectile(wAttack.Projectile());
		for (std::vector<std::pair<StatusEffectType, int> >::iterator effecti = wAttack.StatusEffects()->begin();
			effecti != wAttack.StatusEffects()->end(); ++effecti) {
				attack.StatusEffects()->push_back(*effecti);
		}
	}
}

void NPC::FindNewWeapon() {
	int weaponValue = 0;
	if (mainHand.lock() && mainHand.lock()->IsCategory(squad.lock()->Weapon())) {
		weaponValue = mainHand.lock()->RelativeValue();
	}
	ItemCategory weaponCategory = squad.lock() ? squad.lock()->Weapon() : Item::StringToItemCategory("Weapon");
	boost::weak_ptr<Item> newWeapon = Game::Inst()->FindItemByCategoryFromStockpiles(weaponCategory, BETTERTHAN, weaponValue);
	if (boost::shared_ptr<Item> weapon = newWeapon.lock()) {
		boost::shared_ptr<Job> weaponJob(new Job("Grab weapon"));
		weaponJob->internal = true;
		weaponJob->ReserveEntity(weapon);
		weaponJob->tasks.push_back(Task(MOVE, weapon->Position()));
		weaponJob->tasks.push_back(Task(TAKE, weapon->Position(), weapon));
		weaponJob->tasks.push_back(Task(WIELD));
		jobs.push_back(weaponJob);
		run = true;
	}
}

boost::weak_ptr<Item> NPC::Wielding() {
	return mainHand;
}

bool NPC::HasHands() { return hasHands; }

NPCPreset::NPCPreset(std::string typeNameVal) : 
typeName(typeNameVal),
	name("AA Club"),
	plural(""),
	color(TCODColor::pink),
	graphic('?'),
	expert(false),
	health(10),
	ai("PeacefulAnimal"),
	needsNutrition(false),
	needsSleep(false),
	generateName(false),
	spawnAsGroup(false),
	group(TCOD_dice_t()),
	attacks(std::list<Attack>()),
	tags(std::set<std::string>())
{
	for (int i = 0; i < STAT_COUNT; ++i) {
		stats[i] = 1;
	}
	for (int i = 0; i < RES_COUNT; ++i) {
		resistances[i] = 0;
	}
	group.addsub = 0;
	group.multiplier = 1;
	group.nb_dices = 1;
	group.nb_faces = 1;
}
