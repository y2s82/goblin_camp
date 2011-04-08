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

#include <boost/serialization/deque.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>

#include "Random.hpp"
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
#include "Faction.hpp"
#include "Stats.hpp"

SkillSet::SkillSet() {
	for (int i = 0; i < SKILLAMOUNT; ++i) { skills[i] = 0; }
}

int SkillSet::operator()(Skill skill) {return skills[skill];}
void SkillSet::operator()(Skill skill, int value) {skills[skill] = value;}

void SkillSet::save(OutputArchive& ar, const unsigned int version) const {
	ar & skills;
}

void SkillSet::load(InputArchive& ar, const unsigned int version) {
	ar & skills;
}

std::map<std::string, NPCType> NPC::NPCTypeNames = std::map<std::string, NPCType>();
std::vector<NPCPreset> NPC::Presets = std::vector<NPCPreset>();

NPC::NPC(Coordinate pos, boost::function<bool(boost::shared_ptr<NPC>)> findJob,
	boost::function<void(boost::shared_ptr<NPC>)> react) : Entity(),
	type(0),
	timeCount(0),
	taskIndex(0),
	orderIndex(0),
	pathIndex(0),
	nopath(false),
	findPathWorking(false),
	pathIsDangerous(false),
	timer(0),
	nextMove(0),
	lastMoveResult(TASKCONTINUE),
	run(true),
	taskBegun(false),
	jobBegun(false),
	expert(false),
	carried(boost::weak_ptr<Item>()),
	mainHand(boost::weak_ptr<Item>()),
	offHand(boost::weak_ptr<Item>()),
	armor(boost::weak_ptr<Item>()),
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
	isTunneler(false),
	isFlying(false),
	aggressive(false),
	coward(false),
	aggressor(boost::weak_ptr<NPC>()),
	dead(false),
	squad(boost::weak_ptr<Squad>()),
	attacks(std::list<Attack>()),
	addedTasksToCurrentJob(0),
	hasMagicRangedAttacks(false),
	threatLocation(Coordinate(-1,-1)),
	seenFire(false),
	traits(std::set<Trait>()),
	damageDealt(0),
	damageReceived(0),
	statusEffectsChanged(false),
	FindJob(findJob),
	React(react),
	escaped(false)
{
	while (!Map::Inst()->IsWalkable(pos.X(),pos.Y())) {
		pos.X(pos.X()+1);
		pos.Y(pos.Y()+1);
	}
	inventory->SetInternal();
	Position(pos,true);

	thirst = thirst - (THIRST_THRESHOLD / 2) + Random::Generate(THIRST_THRESHOLD - 1);
	hunger = hunger - (HUNGER_THRESHOLD / 2) + Random::Generate(HUNGER_THRESHOLD - 1);
	weariness = weariness - (WEARY_THRESHOLD / 2) + Random::Generate(WEARY_THRESHOLD - 1);

	path = new TCODPath(Map::Inst()->Width(), Map::Inst()->Height(), Map::Inst(), (void*)this);

	for (int i = 0; i < STAT_COUNT; ++i) {baseStats[i] = 0; effectiveStats[i] = 0;}
	for (int i = 0; i < RES_COUNT; ++i) {baseResistances[i] = 0; effectiveResistances[i] = 0;}
}

NPC::~NPC() {
	pathMutex.lock(); /* In case a pathing thread is active we need to wait until we can lock the pathMutex,
					  because it signals that the thread has finished */
	Map::Inst()->NPCList(x, y)->erase(uid);
	if (squad.lock()) squad.lock()->Leave(uid);

	if (boost::iequals(NPC::NPCTypeToString(type), "orc")) Game::Inst()->OrcCount(-1);
	else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) Game::Inst()->GoblinCount(-1);
	else if (NPC::Presets[type].tags.find("localwildlife") != NPC::Presets[type].tags.end()) Game::Inst()->PeacefulFaunaCount(-1);

	delete path;
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
	inventory->Position(pos);
}

void NPC::Position(Coordinate pos) { Position(pos, false); }

Task* NPC::currentTask() { return jobs.empty() ? 0 : &(jobs.front()->tasks[taskIndex]); }
Task* NPC::nextTask() { 
	if (!jobs.empty()) {
		if ((signed int)jobs.front()->tasks.size() > taskIndex+1) {
			return &jobs.front()->tasks[taskIndex+1];
		}
	}
	return 0;
}

boost::weak_ptr<Job> NPC::currentJob() { return jobs.empty() ? boost::weak_ptr<Job>() : boost::weak_ptr<Job>(jobs.front()); }

void NPC::TaskFinished(TaskResult result, std::string msg) {
#ifdef DEBUG
	if (msg.size() > 0) {
		std::cout<<name<<":"<<msg<<"\n";
	}
#endif
	RemoveEffect(EATING);
	RemoveEffect(DRINKING);
	RemoveEffect(WORKING);
	if (jobs.size() > 0) {
		if (result == TASKSUCCESS) {
			if (++taskIndex >= (signed int)jobs.front()->tasks.size()) {
				jobs.front()->Complete();
				jobs.pop_front();
				taskIndex = 0;
				foundItem = boost::weak_ptr<Item>();
				addedTasksToCurrentJob = 0;
				jobBegun = false;
			}
		} else {
			//Remove any tasks this NPC added onto the front before sending it back to the JobManager
			for (int i = 0; i < addedTasksToCurrentJob; ++i) {
				if (!jobs.front()->tasks.empty()) jobs.front()->tasks.erase(jobs.front()->tasks.begin());
			}
			if (!jobs.front()->internal) JobManager::Inst()->CancelJob(jobs.front(), msg, result);
			jobs.pop_front();
			taskIndex = 0;
			DropItem(carried);
			carried.reset();
			foundItem = boost::weak_ptr<Item>();
			addedTasksToCurrentJob = 0;
			jobBegun = false;
		}
	}
	taskBegun = false;
	run = true;

	if (result != TASKSUCCESS) {
		//If we're wielding a container (ie. a tool) spill it's contents
		if (mainHand.lock() && boost::dynamic_pointer_cast<Container>(mainHand.lock())) {
			boost::shared_ptr<Container> cont(boost::static_pointer_cast<Container>(mainHand.lock()));
			if (cont->ContainsWater() > 0) {
				Game::Inst()->CreateWater(Position(), cont->ContainsWater());
				cont->RemoveWater(cont->ContainsWater());
			} else if (cont->ContainsFilth() > 0) {
				Game::Inst()->CreateFilth(Position(), cont->ContainsFilth());
				cont->RemoveFilth(cont->ContainsFilth());
			}
		}
	}
}

void NPC::HandleThirst() {
	Coordinate tmpCoord;
	bool found = false;

	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Drink") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Drink"), Position());
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
			} else {
				for (int ix = tmpCoord.X()-1; ix <= tmpCoord.X()+1; ++ix) {
					for (int iy = tmpCoord.Y()-1; iy <= tmpCoord.Y()+1; ++iy) {
						if (Map::Inst()->IsWalkable(ix,iy,(void*)this)) {
							newJob->tasks.push_back(Task(MOVE, Coordinate(ix,iy)));
							goto CONTINUEDRINKBLOCK;
						}
					}
				}
CONTINUEDRINKBLOCK:
				newJob->tasks.push_back(Task(DRINK, tmpCoord));
				if (jobs.empty()) JobManager::Inst()->NPCNotWaiting(uid);
				jobs.push_back(newJob);
			}
		}
	}
}

void NPC::HandleHunger() {
	Coordinate tmpCoord;
	bool found = false;

	if (hunger > 48000 && !jobs.empty() &&  jobs.front()->name.find("Eat") == std::string::npos) { //Starving and doing something else
		TaskFinished(TASKFAILNONFATAL);
	}
		
	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Eat") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Prepared food"), Position(), MOSTDECAYED);
		if (!item.lock()) {item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Food"), Position(), MOSTDECAYED | AVOIDGARBAGE);}
		if (!item.lock()) { //Nothing to eat!
			if (hunger > 48000) { //Nearing death
				ScanSurroundings();
				boost::shared_ptr<NPC> weakest;
				for (std::list<boost::weak_ptr<NPC> >::iterator npci = nearNpcs.begin(); npci != nearNpcs.end(); ++npci) {
					if (npci->lock() && (!weakest || npci->lock()->health < weakest->health)) {
						weakest = npci->lock();
					}
				}

				if (weakest) { //Found a creature nearby, eat it
					boost::shared_ptr<Job> newJob(new Job("Eat", HIGH, 0, !expert));
					newJob->internal = true;
					newJob->tasks.push_back(Task(GETANGRY));
					newJob->tasks.push_back(Task(KILL, weakest->Position(), weakest, 0, 1));
					newJob->tasks.push_back(Task(EAT));
					newJob->tasks.push_back(Task(CALMDOWN));
					jobs.push_back(newJob);
				}				
			}
		} else { //Something to eat!
			boost::shared_ptr<Job> newJob(new Job("Eat", MED, 0, !expert));
			newJob->internal = true;

			newJob->ReserveEntity(item);
			newJob->tasks.push_back(Task(MOVE,item.lock()->Position()));
			newJob->tasks.push_back(Task(TAKE,item.lock()->Position(), item));
			newJob->tasks.push_back(Task(EAT));
			if (jobs.empty()) JobManager::Inst()->NPCNotWaiting(uid);
			jobs.push_back(newJob);
		}
	}
}

void NPC::HandleWeariness() {
	bool found = false;
	for (std::deque<boost::shared_ptr<Job> >::iterator jobIter = jobs.begin(); jobIter != jobs.end(); ++jobIter) {
		if ((*jobIter)->name.find("Sleep") != std::string::npos) found = true;
		else if ((*jobIter)->name.find("Get rid of") != std::string::npos) found = true;
	}
	if (!found) {
		boost::weak_ptr<Construction> wbed = Game::Inst()->FindConstructionByTag(BED, Position());
		boost::shared_ptr<Job> sleepJob(new Job("Sleep"));
		sleepJob->internal = true;
		if (!squad.lock() && mainHand.lock()) { //Only soldiers go to sleep gripping their weapons
			sleepJob->tasks.push_back(Task(UNWIELD));
			sleepJob->tasks.push_back(Task(TAKE));
			sleepJob->tasks.push_back(Task(STOCKPILEITEM));
		}
		if (boost::shared_ptr<Construction> bed = wbed.lock()) {
			if (bed->Built()) {
				sleepJob->ReserveEntity(bed);
				sleepJob->tasks.push_back(Task(MOVE, bed->Position()));
				sleepJob->tasks.push_back(Task(SLEEP, bed->Position(), bed));
				jobs.push_back(sleepJob);
				return;
			}
		}
		sleepJob->tasks.push_back(Task(SLEEP, Position()));
		if (jobs.empty()) JobManager::Inst()->NPCNotWaiting(uid);
		jobs.push_back(sleepJob);
	}
}

void NPC::Update() {
	if (Map::Inst()->NPCList(x,y)->size() > 1) _bgcolor = TCODColor::darkGrey;
	else _bgcolor = TCODColor::black;

	UpdateStatusEffects();
	//Apply armor effects if present
	if (boost::shared_ptr<Item> arm = armor.lock()) {
		for (int i = 0; i < RES_COUNT; ++i) {
			effectiveResistances[i] += arm->Resistance(i);
		}
	}

	if (Random::Generate(UPDATES_PER_SECOND) == 0) { //Recalculate bulk once a second, items may get unexpectedly destroyed
		bulk = 0;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = inventory->begin(); itemi != inventory->end(); ++itemi) {
			if (itemi->lock())
				bulk += itemi->lock()->GetBulk();
		}
	}
	if (!HasEffect(FLYING) && effectiveStats[MOVESPEED] > 0) effectiveStats[MOVESPEED] = std::max(1, effectiveStats[MOVESPEED]-Map::Inst()->GetMoveModifier(x,y));
	if (effectiveStats[MOVESPEED] > 0) effectiveStats[MOVESPEED] = std::max(1, effectiveStats[MOVESPEED]-bulk);

	if (needsNutrition) {
		++thirst;
		++hunger;

		if (thirst >= THIRST_THRESHOLD) AddEffect(THIRST);
		else RemoveEffect(THIRST);
		if (hunger >= HUNGER_THRESHOLD) AddEffect(HUNGER);
		else RemoveEffect(HUNGER);

		if (thirst > THIRST_THRESHOLD && Random::Generate(UPDATES_PER_SECOND * 5 - 1) == 0) {
			HandleThirst();
		} else if (thirst > THIRST_THRESHOLD * 2) Kill();
		if (hunger > HUNGER_THRESHOLD && Random::Generate(UPDATES_PER_SECOND * 5 - 1) == 0) {
			HandleHunger();
		} else if (hunger > 72000) Kill();
	}

	if (needsSleep) {
		++weariness;

		if (weariness >= WEARY_THRESHOLD) { 
			AddEffect(DROWSY);
			if (weariness > WEARY_THRESHOLD) HandleWeariness(); //Give the npc a chance to find a sleepiness curing item
		} else RemoveEffect(DROWSY);
	}

	if (!HasEffect(FLYING)) {
		if (boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(x,y).lock()) {
			boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(x,y)).lock();
			if (water->Depth() > WALKABLE_WATER_DEPTH && (!construct || !construct->HasTag(BRIDGE) || !construct->Built())) {
				AddEffect(SWIM);
				RemoveEffect(BURNING);
			} else { RemoveEffect(SWIM); }
		} else { RemoveEffect(SWIM); }

		if (Map::Inst()->GetNatureObject(x,y) >= 0 && 
			Game::Inst()->natureList[Map::Inst()->GetNatureObject(x,y)]->IsIce() &&
			Random::Generate(UPDATES_PER_SECOND*5) == 0) AddEffect(TRIPPED);
	}

	for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
		attacki->Update();
	}

	if (faction == PLAYERFACTION && Random::Generate(MONTH_LENGTH - 1) == 0) Game::Inst()->CreateFilth(Position());

	if (carried.lock()) {
		AddEffect(StatusEffect(CARRYING, carried.lock()->GetGraphic(), carried.lock()->Color()));
	} else RemoveEffect(CARRYING);

	if (HasTrait(CRACKEDSKULL) && Random::Generate(MONTH_LENGTH * 6) == 0) GoBerserk();
	if (HasEffect(BURNING)) {
		if (Random::Generate(UPDATES_PER_SECOND * 3) == 0) {
			boost::shared_ptr<Spell> spark = Game::Inst()->CreateSpell(Position(), Spell::StringToSpellType("spark"));
			spark->CalculateFlightPath(Position()+Coordinate(Random::Generate(-1,1),Random::Generate(-1,1)), 50, GetHeight());
		}
		if (effectiveResistances[FIRE_RES] < 90 && !HasEffect(RAGE) && (jobs.empty() || jobs.front()->name != "Jump into water")) {
			if (Random::Generate(UPDATES_PER_SECOND) == 0) {
				RemoveEffect(PANIC);
				while (!jobs.empty()) TaskFinished(TASKFAILFATAL);
				boost::shared_ptr<Job> jumpJob(new Job("Jump into water"));
				jumpJob->internal = true;
				Coordinate waterPos = Game::Inst()->FindWater(Position());
				if (waterPos.X() != -1) {
					jumpJob->tasks.push_back(Task(MOVE, waterPos));
					jobs.push_back(jumpJob);
				}
			} else if (!coward && boost::iequals(NPC::NPCTypeToString(type), "orc") && Random::Generate(2) == 0) {RemoveEffect(PANIC); GoBerserk();}
			else {AddEffect(PANIC);}
		}
	}

	UpdateHealth();
}

void NPC::UpdateStatusEffects() {
	//Add job related effects
	if (!jobs.empty()) {
		for (std::list<StatusEffectType>::iterator stati = jobs.front()->statusEffects.begin();
			stati != jobs.front()->statusEffects.end(); ++stati) {
				AddEffect(*stati);
		}
	}

	for (int i = 0; i < STAT_COUNT; ++i) {
		effectiveStats[i] = baseStats[i];
	}
	for (int i = 0; i < RES_COUNT; ++i) {
		effectiveResistances[i] = baseResistances[i];
	}
	++statusGraphicCounter;
	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end();) {
		//Apply effects to stats
		for (int i = 0; i < STAT_COUNT; ++i) {
			effectiveStats[i] = (int)(effectiveStats[i] * statusEffectI->statChanges[i]);
		}
		for (int i = 0; i < RES_COUNT; ++i) {
			effectiveResistances[i] = (int)(effectiveResistances[i] * statusEffectI->resistanceChanges[i]);
		}

		if (statusEffectI->damage.second != 0 && --statusEffectI->damage.first <= 0) {
			statusEffectI->damage.first = UPDATES_PER_SECOND;
			TCOD_dice_t dice;
			dice.addsub = (float)statusEffectI->damage.second;
			dice.multiplier = 1;
			dice.nb_dices = 1;
			dice.nb_faces = std::max(1, (int)dice.addsub / 5);
			Attack attack;
			attack.Amount(dice);
			attack.Type((DamageType)statusEffectI->damageType);
			Damage(&attack);
		}

		if (faction == PLAYERFACTION && statusEffectI->negative && !HasEffect(SLEEPING) && (statusEffectsChanged || Random::Generate(MONTH_LENGTH) == 0)) {
			statusEffectsChanged = false;
			bool removalJobFound = false;
			for (std::deque<boost::shared_ptr<Job> >::iterator jobi = jobs.begin(); jobi != jobs.end(); ++jobi) {
				if ((*jobi)->name.find("Get rid of") != std::string::npos) {
					removalJobFound = true;
					break;
				}
			}
			if (!removalJobFound && Item::EffectRemovers.find((StatusEffectType)statusEffectI->type) != Item::EffectRemovers.end()) {
				boost::shared_ptr<Item> fixItem;
				for (std::multimap<StatusEffectType, ItemType>::iterator fixi = Item::EffectRemovers.equal_range((StatusEffectType)statusEffectI->type).first;
					fixi != Item::EffectRemovers.equal_range((StatusEffectType)statusEffectI->type).second && !fixItem; ++fixi) {
						fixItem = Game::Inst()->FindItemByTypeFromStockpiles(fixi->second, Position()).lock();
				}
				if (fixItem) {
					boost::shared_ptr<Job> rEffJob(new Job("Get rid of "+statusEffectI->name));
					rEffJob->internal = true;
					rEffJob->ReserveEntity(fixItem);
					rEffJob->tasks.push_back(Task(MOVE, fixItem->Position()));
					rEffJob->tasks.push_back(Task(TAKE, fixItem->Position(), fixItem));
					if (fixItem->IsCategory(Item::StringToItemCategory("drink")))
						rEffJob->tasks.push_back(Task(DRINK));
					else
						rEffJob->tasks.push_back(Task(EAT));
					jobs.push_back(rEffJob);
				}
			}
		}

		//Remove the statuseffect if its cooldown has run out
		if (statusEffectI->cooldown > 0 && --statusEffectI->cooldown == 0) {
			if (statusEffectI == statusEffectIterator) {
				++statusEffectIterator;
				statusGraphicCounter = 0;
			}
			statusEffectI = statusEffects.erase(statusEffectI);
			if (statusEffectIterator == statusEffects.end()) statusEffectIterator = statusEffects.begin();
		} else ++statusEffectI;
	}
	
	if (statusGraphicCounter > 10) {
		statusGraphicCounter = 0;
		if (statusEffectIterator != statusEffects.end()) ++statusEffectIterator;
		else statusEffectIterator = statusEffects.begin();

		if (statusEffectIterator != statusEffects.end() && !statusEffectIterator->visible) {
			std::list<StatusEffect>::iterator oldIterator = statusEffectIterator;
			++statusEffectIterator;
			while (statusEffectIterator != oldIterator) {
				if (statusEffectIterator != statusEffects.end()) {
					if (statusEffectIterator->visible) break;
					++statusEffectIterator;
				}
				else statusEffectIterator = statusEffects.begin();
			}
			if (statusEffectIterator != statusEffects.end() && !statusEffectIterator->visible) statusEffectIterator = statusEffects.end();
		}
	}
}

void NPC::Think() {
	Coordinate tmpCoord;
	int tmp;
	
	UpdateVelocity();

	lastMoveResult = Move(lastMoveResult);

	if (velocity == 0) {
		timeCount += thinkSpeed; //Can't think while hurtling through the air, sorry
	} else if (!jobs.empty()) {
		TaskFinished(TASKFAILFATAL, "Flying through the air");
		JobManager::Inst()->NPCNotWaiting(uid);
	}
	
	while (timeCount > UPDATES_PER_SECOND) {
		if (Random::GenerateBool()) React(boost::static_pointer_cast<NPC>(shared_from_this()));

		if (aggressor.lock()) {
			JobManager::Inst()->NPCNotWaiting(uid);
			if (Game::Adjacent(Position(), aggressor)) {
				if (currentTask() && currentTask()->action == KILL) Hit(aggressor, currentTask()->flags != 0);
				else Hit(aggressor);
			}
			if (Random::Generate(9) <= 3 && Distance(Position(), aggressor.lock()->Position()) > LOS_DISTANCE) {
				aggressor.reset();
				TaskFinished(TASKFAILFATAL, "Target lost");
			}
		}

		timeCount -= UPDATES_PER_SECOND;
		if (!jobs.empty() && !jobBegun) {
			jobBegun = true;
			ValidateCurrentJob();
		}

		if (!jobs.empty()) {
			switch(currentTask()->action) {
			case MOVE:
				if (!Map::Inst()->IsWalkable(currentTarget().X(), currentTarget().Y(), (void*)this)) {
					TaskFinished(TASKFAILFATAL, "(MOVE)Target unwalkable");
					break;
				}
				if ((signed int)x == currentTarget().X() && (signed int)y == currentTarget().Y()) {
					TaskFinished(TASKSUCCESS);
					break;
				}
				if (!taskBegun) { findPath(currentTarget()); taskBegun = true; lastMoveResult = TASKCONTINUE;}

				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) {
					TaskFinished(lastMoveResult, std::string("(MOVE)Could not find path to target")); break;
				} else if (lastMoveResult == PATHEMPTY) {
					if (!((signed int)x == currentTarget().X() && (signed int)y == currentTarget().Y())) {
						TaskFinished(TASKFAILFATAL, std::string("(MOVE)No path to target")); break;
					}
				}
				break;

			case MOVENEAR:
				/*MOVENEAR first figures out our "real" target, which is a tile near
				to our current target. Near means max 5 tiles away, visible and
				walkable. Once we have our target we can actually switch over
				to a normal MOVE task. In case we can't find a visible tile,
				we'll allow a non LOS one*/
				{bool checkLOS = true;
				for (int i = 0; i < 2; ++i) {
					tmp = 0;
					while (tmp++ < 10) {
						int tarX = Random::Generate(-5, 5) + currentTarget().X();
						int tarY = Random::Generate(-5, 5) + currentTarget().Y();
						if (tarX < 0) tarX = 0;
						if (tarX >= Map::Inst()->Width()) tarX = Map::Inst()->Width()-1;
						if (tarY < 0) tarY = 0;
						if (tarY >= Map::Inst()->Height()) tarY = Map::Inst()->Height()-1;
						if (Map::Inst()->IsWalkable(tarX, tarY, (void *)this) && !Map::Inst()->IsUnbridgedWater(tarX, tarY) 
							&& !Map::Inst()->IsDangerous(tarX, tarY, faction)) {
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
				TaskFinished(TASKFAILFATAL, "(MOVENEAR)Couldn't find NEAR coordinate");
MOVENEARend:
				break;


			case MOVEADJACENT:
				if (currentEntity().lock()) {
					if (Game::Adjacent(Position(), currentEntity())) {
						TaskFinished(TASKSUCCESS);
						break;
					}
				} else {
					if (Game::Adjacent(Position(), currentTarget())) {
						TaskFinished(TASKSUCCESS);
						break;
					}
				}
				if (!taskBegun) {
					if (currentEntity().lock()) tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity(), GetFaction());
					else tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentTarget(), GetFaction());
					if (tmpCoord.X() >= 0) {
						findPath(tmpCoord);
					} else { TaskFinished(TASKFAILFATAL, std::string("(MOVEADJACENT)No walkable adjacent tiles")); break; }
					taskBegun = true;
					lastMoveResult = TASKCONTINUE;
				}
				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) { TaskFinished(lastMoveResult, std::string("Could not find path to target")); break; }
				else if (lastMoveResult == PATHEMPTY) {
					TaskFinished(TASKFAILFATAL, "(MOVEADJACENT)PATHEMPTY");
				}
				break;

			case WAIT:
				if (++timer > currentTarget().X()) { timer = 0; TaskFinished(TASKSUCCESS); }
				break;

			case BUILD:
				if (Game::Adjacent(Position(), currentEntity())) {
					AddEffect(WORKING);
					tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Build();
					if (tmp > 0) {
						Announce::Inst()->AddMsg((boost::format("%s completed") % currentEntity().lock()->Name()).str(), TCODColor::white, currentEntity().lock()->Position());
						TaskFinished(TASKSUCCESS);
						break;
					} else if (tmp == BUILD_NOMATERIAL) {
						TaskFinished(TASKFAILFATAL, "(BUILD)Missing materials");
						break;
					}
				} else {
					TaskFinished(TASKFAILFATAL, "(BUILD)Not adjacent to target");
					break;
				}
				break;

			case TAKE:
				if (!currentEntity().lock()) { TaskFinished(TASKFAILFATAL, "(TAKE)No target entity"); break; }
				if (Position() == currentEntity().lock()->Position()) {
					if (boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()) {
						boost::weak_ptr<Container> cont(boost::static_pointer_cast<Container>(boost::static_pointer_cast<Item>(currentEntity().lock())->ContainedIn().lock()));
						cont.lock()->RemoveItem(
							boost::static_pointer_cast<Item>(currentEntity().lock()));
					}
					PickupItem(boost::static_pointer_cast<Item>(currentEntity().lock()));
					TaskFinished(TASKSUCCESS);
					break;
				} else { TaskFinished(TASKFAILFATAL, "(TAKE)Item not found"); break; }
				break;

			case DROP:
				DropItem(carried);
				carried.reset();
				TaskFinished(TASKSUCCESS);
				break;

			case PUTIN:
				if (carried.lock()) {
					inventory->RemoveItem(carried);
					carried.lock()->Position(Position());
					if (!currentEntity().lock()) {
						TaskFinished(TASKFAILFATAL, "(PUTIN)Target does not exist");
						break;
					}
					if (!Game::Adjacent(Position(), currentEntity().lock()->Position())) {
						TaskFinished(TASKFAILFATAL, "(PUTIN)Not adjacent to container");
						break;
					}
					if (boost::dynamic_pointer_cast<Container>(currentEntity().lock())) {
						boost::shared_ptr<Container> cont = boost::static_pointer_cast<Container>(currentEntity().lock());
						if (!cont->AddItem(carried)) {
							TaskFinished(TASKFAILFATAL, "(PUTIN)Container full");
							break;
						}
						bulk -= carried.lock()->GetBulk();
					} else {
						TaskFinished(TASKFAILFATAL, "(PUTIN)Target not a container");
						break;
					}
				}
				carried.reset();
				TaskFinished(TASKSUCCESS);
				break;

			case DRINK: //Either we have an item target to drink, or a water tile
				if (carried.lock()) { //Drink from an item
					timer = boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
					inventory->RemoveItem(carried);
					bulk -= carried.lock()->GetBulk();
					ApplyEffects(carried.lock());
					Game::Inst()->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
				} else if (timer == 0) { //Drink from a water tile
					if (std::abs((signed int)x - currentTarget().X()) <= 1 &&
						std::abs((signed int)y - currentTarget().Y()) <= 1) {
							if (boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(currentTarget().X(), currentTarget().Y()).lock()) {
								if (water->Depth() > DRINKABLE_WATER_DEPTH) {
									thirst -= (int)(THIRST_THRESHOLD / 10);
									AddEffect(DRINKING);
									if (thirst < 0) { 
										TaskFinished(TASKSUCCESS); 
									}
									break;
								}
							}
					}
					TaskFinished(TASKFAILFATAL, "(DRINK)Not enough water");
				}

				if (timer > 0) {
					AddEffect(DRINKING);
					thirst -= std::min((int)(THIRST_THRESHOLD / 5), timer);
					timer -= (int)(THIRST_THRESHOLD / 5);
					if (timer <= 0) {
						timer = 0;
						TaskFinished(TASKSUCCESS);
					}
				}
				break;

			case EAT:
				if (carried.lock()) {
					//Set the nutrition to the timer variable
					if (boost::dynamic_pointer_cast<OrganicItem>(carried.lock())) {
						timer = boost::static_pointer_cast<OrganicItem>(carried.lock())->Nutrition();
					} else timer = 100;
					inventory->RemoveItem(carried);
					bulk -= carried.lock()->GetBulk();
					ApplyEffects(carried.lock());
					for (std::list<ItemType>::iterator fruiti = Item::Presets[carried.lock()->Type()].fruits.begin(); fruiti != Item::Presets[carried.lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}

					Game::Inst()->RemoveItem(carried);
				} else if (timer == 0) { //Look in all adjacent tiles for anything edible
					for (int ix = (int)x - 1; ix <= (int)x + 1; ++ix) {
						for (int iy = (int)y - 1; iy <= (int)y + 1; ++iy) {
							if (ix >= 0 && ix < Map::Inst()->Width() && iy >= 0 && iy < Map::Inst()->Height()) {
								for (std::set<int>::iterator itemi = Map::Inst()->ItemList(ix, iy)->begin();
									itemi != Map::Inst()->ItemList(ix, iy)->end(); ++itemi) {
										boost::shared_ptr<Item> item = Game::Inst()->GetItem(*itemi).lock();
										if (item && (item->IsCategory(Item::StringToItemCategory("food")) ||
											item->IsCategory(Item::StringToItemCategory("corpse") ))) {
												jobs.front()->ReserveEntity(item);
												jobs.front()->tasks.push_back(Task(MOVE, item->Position()));
												jobs.front()->tasks.push_back(Task(TAKE, item->Position(), item));
												jobs.front()->tasks.push_back(Task(EAT));
												goto CONTINUEEAT;
										}
								}

							}
						}
					}
				} 
				
				if (timer > 0) {
					AddEffect(EATING);
					hunger -= std::min(5000, timer);
					timer -= 5000;
					if (timer <= 0) {
						timer = 0;
						TaskFinished(TASKSUCCESS);
					}
					break;
				}
CONTINUEEAT:
				carried = boost::weak_ptr<Item>();
				TaskFinished(TASKSUCCESS);
				break;

			case FIND:
				foundItem = Game::Inst()->FindItemByCategoryFromStockpiles(currentTask()->item, currentTask()->target, currentTask()->flags);
				if (!foundItem.lock()) {
					TaskFinished(TASKFAILFATAL, "(FIND)Failed"); 
					break;
				} else {
					if (factionPtr->IsFriendsWith(PLAYERFACTION)) currentJob().lock()->ReserveEntity(foundItem);
					TaskFinished(TASKSUCCESS);
					break;
				}

			case USE:
				if (currentEntity().lock() && boost::dynamic_pointer_cast<Construction>(currentEntity().lock())) {
					tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Use();
					AddEffect(WORKING);
					if (tmp >= 100) {
						TaskFinished(TASKSUCCESS);
					} else if (tmp < 0) {
						TaskFinished(TASKFAILFATAL, "(USE)Can not use (tmp<0)"); break;
					}
				} else { TaskFinished(TASKFAILFATAL, "(USE)Attempted to use non-construct"); break; }
				break;

			case HARVEST:
				if (carried.lock()) {
					bool stockpile = false;
					if (nextTask() && nextTask()->action == STOCKPILEITEM) stockpile = true;

					boost::shared_ptr<Item> plant = carried.lock();
					inventory->RemoveItem(carried);
					bulk -= plant->GetBulk();
					carried = boost::weak_ptr<Item>();

					for (std::list<ItemType>::iterator fruiti = Item::Presets[plant->Type()].fruits.begin(); fruiti != Item::Presets[plant->Type()].fruits.end(); ++fruiti) {
						if (stockpile) {
							int item = Game::Inst()->CreateItem(Position(), *fruiti, false);
							Stats::Inst()->ItemBuilt(Item::Presets[*fruiti].name); //Harvesting counts as production
							PickupItem(Game::Inst()->GetItem(item));
							stockpile = false;
						} else {
							Game::Inst()->CreateItem(Position(), *fruiti, true);
						}
					}

					Game::Inst()->RemoveItem(plant);

					TaskFinished(TASKSUCCESS);
					break;
				} else {
					inventory->RemoveItem(carried);
					carried = boost::weak_ptr<Item>();
					TaskFinished(TASKFAILFATAL, "(HARVEST)Carrying nonexistant item");
					break;
				}

			case FELL:
				if (boost::shared_ptr<NatureObject> tree = boost::static_pointer_cast<NatureObject>(currentEntity().lock())) {
					tmp = tree->Fell(); //This'll be called about 100-150 times per tree
					if (mainHand.lock() && Random::Generate(300) == 0) DecreaseItemCondition(mainHand);
					AddEffect(WORKING);
					if (tmp <= 0) {
						bool stockpile = false;
						if (nextTask() && nextTask()->action == STOCKPILEITEM) stockpile = true;
						for (std::list<ItemType>::iterator iti = NatureObject::Presets[tree->Type()].components.begin(); iti != NatureObject::Presets[tree->Type()].components.end(); ++iti) {
							if (stockpile) {
								int item = Game::Inst()->CreateItem(tree->Position(), *iti, false);
								Stats::Inst()->ItemBuilt(Item::Presets[*iti].name); //Felling trees counts as item production
								DropItem(carried);
								PickupItem(Game::Inst()->GetItem(item));
								stockpile = false;
							} else {
								Game::Inst()->CreateItem(tree->Position(), *iti, true);
							}
						}
						Game::Inst()->RemoveNatureObject(tree);
						TaskFinished(TASKSUCCESS);
						break;
					}
					//Job underway
					break;
				}
				TaskFinished(TASKFAILFATAL, "(FELL) No NatureObject to fell");
				break;

			case HARVESTWILDPLANT:
				if (boost::shared_ptr<NatureObject> plant = boost::static_pointer_cast<NatureObject>(currentEntity().lock())) {
					tmp = plant->Harvest();
					AddEffect(WORKING);
					if (tmp <= 0) {
						bool stockpile = false;
						if (nextTask() && nextTask()->action == STOCKPILEITEM) stockpile = true;
						for (std::list<ItemType>::iterator iti = NatureObject::Presets[plant->Type()].components.begin(); iti != NatureObject::Presets[plant->Type()].components.end(); ++iti) {
							if (stockpile) {
								int item = Game::Inst()->CreateItem(plant->Position(), *iti, false);
								DropItem(carried);
								PickupItem(Game::Inst()->GetItem(item));			
								stockpile = false;
							} else {
								Game::Inst()->CreateItem(plant->Position(), *iti, true);
							}
						}
						Game::Inst()->RemoveNatureObject(plant);
						TaskFinished(TASKSUCCESS);
						break;
					}
					//Job underway
					break;
				}
				TaskFinished(TASKFAILFATAL, "(HARVESTWILDPLANT)Harvest target doesn't exist");
				break;

			case KILL:
				//The reason KILL isn't a combination of MOVEADJACENT and something else is that the other moving actions
				//assume their target isn't a moving one
				if (!currentEntity().lock()) {
					TaskFinished(TASKSUCCESS);
					break;
				}

				if (Game::Adjacent(Position(), currentEntity())) {
					Hit(currentEntity(), currentTask()->flags != 0);
					break;
				} else if (currentTask()->flags == 0 && WieldingRangedWeapon() && quiver.lock() && 
					!quiver.lock()->empty()) {
					FireProjectile(currentEntity());
					break;
				} else if (hasMagicRangedAttacks) {
					CastOffensiveSpell(currentEntity());
					break;
				}

				if (!taskBegun || Random::Generate(UPDATES_PER_SECOND * 2 - 1) == 0) { //Repath every ~2 seconds
					tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity(), GetFaction());
					if (tmpCoord.X() >= 0) {
						findPath(tmpCoord);
					}
					taskBegun = true;
					lastMoveResult = TASKCONTINUE;
				}

				if (lastMoveResult == TASKFAILFATAL || lastMoveResult == TASKFAILNONFATAL) { TaskFinished(lastMoveResult, std::string("(KILL)Could not find path to target")); break; }
				else if (lastMoveResult == PATHEMPTY) {
					tmpCoord = Game::Inst()->FindClosestAdjacent(Position(), currentEntity(), GetFaction());
					if (tmpCoord.X() >= 0) {
						findPath(tmpCoord);
					}
				}
				break;

			case FLEEMAP:
				if (x == 0 || x == Map::Inst()->Width()-1 ||
					y == 0 || y == Map::Inst()->Height()-1) {
						//We are at the edge, escape!
						Escape();
						return;
				}

				//Find the closest edge and change into a MOVE task and a new FLEEMAP task
				//Unfortunately this assumes that FLEEMAP is the last task in a job,
				//which might not be.
				{
					Coordinate target;
					tmp = std::abs((signed int)x - Map::Inst()->Width() / 2);
					if (tmp < std::abs((signed int)y - Map::Inst()->Height() / 2)) {
						target = Coordinate(x, (y < (unsigned int)Map::Inst()->Height() / 2) ? 0 : Map::Inst()->Height()-1);
					} else {
						target = Coordinate((x < (unsigned int)Map::Inst()->Width() / 2) ? 0 : Map::Inst()->Width()-1, 
							y);
					}
					if (Map::Inst()->IsWalkable(target.X(), target.Y(), static_cast<void*>(this)))
						currentJob().lock()->tasks[taskIndex] = Task(MOVE, target);
					else
						currentJob().lock()->tasks[taskIndex] = Task(MOVENEAR, target);
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
					AddEffect(WORKING);
					if (construct->Condition() <= 0) {
						Game::Inst()->RemoveConstruction(construct);
						TaskFinished(TASKSUCCESS);
						break;
					}
				} else {
					TaskFinished(TASKFAILFATAL, "(DISMANTLE)Construction does not exist!");
				}
				break;

			case WIELD:
				if (carried.lock()) {
					if (mainHand.lock()) { //Drop currently wielded weapon if such exists
						DropItem(mainHand);
						mainHand.reset();
					}
					mainHand = carried;
					carried.reset();
					TaskFinished(TASKSUCCESS);
#ifdef DEBUG
					std::cout<<name<<" wielded "<<mainHand.lock()->Name()<<"\n";
#endif
					break;
				}
				TaskFinished(TASKFAILFATAL, "(WIELD)Not carrying an item");
				break;

			case WEAR:
				if (carried.lock()) {
					if (carried.lock()->IsCategory(Item::StringToItemCategory("Armor"))) {
						if (armor.lock()) { //Remove armor and drop if already wearing
							DropItem(armor);
							armor.reset();
						}
						armor = carried;
#ifdef DEBUG
					std::cout<<name<<" wearing "<<armor.lock()->Name()<<"\n";
#endif
					}  else if (carried.lock()->IsCategory(Item::StringToItemCategory("Quiver"))) {
						if (quiver.lock()) { //Remove quiver and drop if already wearing
							DropItem(quiver);
							quiver.reset();
						}
						quiver = boost::static_pointer_cast<Container>(carried.lock());
#ifdef DEBUG
					std::cout<<name<<" wearing "<<quiver.lock()->Name()<<"\n";
#endif
					}
					carried.reset();
					TaskFinished(TASKSUCCESS);
					break;
				}
				TaskFinished(TASKFAILFATAL, "(WEAR)Not carrying an item");
				break;

			case BOGIRON:
				if (Map::Inst()->GetType(x, y) == TILEBOG) {
					AddEffect(WORKING);
					if (Random::Generate(UPDATES_PER_SECOND * 15 - 1) == 0) {
						bool stockpile = false;
						if (nextTask() && nextTask()->action == STOCKPILEITEM) stockpile = true;

						if (stockpile) {
							int item = Game::Inst()->CreateItem(Position(), Item::StringToItemType("Bog iron"), false);
							DropItem(carried);
							PickupItem(Game::Inst()->GetItem(item));
							stockpile = false;
						} else {
							Game::Inst()->CreateItem(Position(), Item::StringToItemType("Bog iron"), true);
						}
						TaskFinished(TASKSUCCESS);
						break;
					} else {
						break;
					}
				}
				TaskFinished(TASKFAILFATAL, "(BOGIRON)Not on a bog tile");
				break;

			case STOCKPILEITEM:
				if (boost::shared_ptr<Item> item = carried.lock()) {
					boost::shared_ptr<Job> stockJob = Game::Inst()->StockpileItem(item, true, true, !item->Reserved());
					if (stockJob) {
						stockJob->internal = true;
						//Add remaining tasks into stockjob
						for (unsigned int i = 1; taskIndex+i < jobs.front()->tasks.size(); ++i) {
							stockJob->tasks.push_back(jobs.front()->tasks[taskIndex+i]);
						}
						jobs.front()->tasks.clear();
						std::deque<boost::shared_ptr<Job> >::const_iterator jobi = jobs.begin();
						++jobi;
						jobs.insert(jobi, stockJob);
						DropItem(item); //The stockpiling job will pickup the item
						carried.reset();
						TaskFinished(TASKSUCCESS);
						break;
					}
				}
				TaskFinished(TASKFAILFATAL, "(STOCKPILEITEM)Not carrying an item");
				break;

			case QUIVER:
				if (carried.lock()) {
					if (!quiver.lock()) {
						DropItem(carried);
						carried.reset();
						TaskFinished(TASKFAILFATAL, "(QUIVER)No quiver");
						break;
					}
					inventory->RemoveItem(carried);
					if (!quiver.lock()->AddItem(carried)) {
						DropItem(carried);
						carried.reset();
						TaskFinished(TASKFAILFATAL, "(QUIVER)Quiver full");
						break;
					}
					carried.reset();
					TaskFinished(TASKSUCCESS);
					break;
				}
				TaskFinished(TASKFAILFATAL, "(QUIVER)Not carrying an item");
				break;

			case FILL: {
				boost::shared_ptr<Container> cont;
				if (carried.lock() && 
					(carried.lock()->IsCategory(Item::StringToItemCategory("Container")) || 
					carried.lock()->IsCategory(Item::StringToItemCategory("Bucket")))) {
					cont = boost::static_pointer_cast<Container>(carried.lock());
				} else if (mainHand.lock() && 
					(mainHand.lock()->IsCategory(Item::StringToItemCategory("Container")) ||
					mainHand.lock()->IsCategory(Item::StringToItemCategory("Bucket")))) {
					cont = boost::static_pointer_cast<Container>(mainHand.lock());
				}
					
				if (cont) {
					if (!cont->empty() && cont->ContainsWater() == 0 && cont->ContainsFilth() == 0) {
						//Not empty, but doesn't have water/filth, so it has items in it
						TaskFinished(TASKFAILFATAL, "(FILL)Attempting to fill non-empty container");
						break;
					}
					
					boost::weak_ptr<WaterNode> wnode = Map::Inst()->GetWater(currentTarget().X(), 
						currentTarget().Y());
					if (wnode.lock() && wnode.lock()->Depth() > 0 && cont->ContainsFilth() == 0) {
						int waterAmount = std::min(50, wnode.lock()->Depth());
						wnode.lock()->Depth(wnode.lock()->Depth()-waterAmount);
						cont->AddWater(waterAmount);
						TaskFinished(TASKSUCCESS);
						break;
					}

					boost::weak_ptr<FilthNode> fnode = Map::Inst()->GetFilth(currentTarget().X(),
						currentTarget().Y());
					if (fnode.lock() && fnode.lock()->Depth() > 0 && cont->ContainsWater() == 0) {
						int filthAmount = std::min(3, fnode.lock()->Depth());
						fnode.lock()->Depth(fnode.lock()->Depth()-filthAmount);
						cont->AddFilth(filthAmount);
						TaskFinished(TASKSUCCESS);
						break;
					}
					TaskFinished(TASKFAILFATAL, "(FILL)Nothing to fill container with");
					break;
				} 
					   }
				TaskFinished(TASKFAILFATAL, "(FILL)Not carrying a container");
				break;

			case POUR: {
				boost::shared_ptr<Container> sourceContainer;
				if (carried.lock() && 
					(carried.lock()->IsCategory(Item::StringToItemCategory("Container")) || 
					carried.lock()->IsCategory(Item::StringToItemCategory("Bucket")))) {
					sourceContainer = boost::static_pointer_cast<Container>(carried.lock());
				} else if (mainHand.lock() && 
					(mainHand.lock()->IsCategory(Item::StringToItemCategory("Container")) ||
					mainHand.lock()->IsCategory(Item::StringToItemCategory("Bucket")))) {
					sourceContainer = boost::static_pointer_cast<Container>(mainHand.lock());
				}

				if (sourceContainer) {
					if (currentEntity().lock() && boost::dynamic_pointer_cast<Container>(currentEntity().lock())) {
						boost::shared_ptr<Container> targetContainer(boost::static_pointer_cast<Container>(currentEntity().lock()));
						if (sourceContainer->ContainsWater() > 0) {
							targetContainer->AddWater(sourceContainer->ContainsWater());
							sourceContainer->RemoveWater(sourceContainer->ContainsWater());
						} else {
							targetContainer->AddFilth(sourceContainer->ContainsFilth());
							sourceContainer->RemoveFilth(sourceContainer->ContainsFilth());
						}
						TaskFinished(TASKSUCCESS);
						break;
					} else if (currentTarget().X() >= 0 && currentTarget().Y() >= 0 && 
						currentTarget().X() < Map::Inst()->Width() && currentTarget().Y() < Map::Inst()->Height()) {
							if (sourceContainer->ContainsWater() > 0) {
								Game::Inst()->CreateWater(currentTarget(), sourceContainer->ContainsWater());
								sourceContainer->RemoveWater(sourceContainer->ContainsWater());
							} else {
								Game::Inst()->CreateFilth(currentTarget(), sourceContainer->ContainsFilth());
								sourceContainer->RemoveFilth(sourceContainer->ContainsFilth());
							}
							TaskFinished(TASKSUCCESS);
							break;
					}
				} else {
					TaskFinished(TASKFAILFATAL, "(POUR) Not carrying a container!");
					break;
				} }
				TaskFinished(TASKFAILFATAL, "(POUR)No valid target");
				break;

			case DIG:
				if (!taskBegun) {
					timer = 0;
					taskBegun = true;
				} else {
					AddEffect(WORKING);
					if (mainHand.lock() && Random::Generate(300) == 0) DecreaseItemCondition(mainHand);
					if (++timer >= 50) {
						Map::Inst()->SetLow(currentTarget().X(), currentTarget().Y(), true);
						Map::Inst()->ChangeType(currentTarget().X(), currentTarget().Y(), TILEDITCH);
						int amount = 0;
						int chance = Random::Generate(9);
						if (chance < 4) amount = 1;
						else if (chance < 8) amount = 2;
						else amount = 3;
						for (int i = 0; i < amount; ++i)
							Game::Inst()->CreateItem(Position(), Item::StringToItemType("earth"));
						TaskFinished(TASKSUCCESS);
					}
				}
				break;

			case FORGET:
				foundItem.reset();
				TaskFinished(TASKSUCCESS);
				break;

			case UNWIELD:
				if (mainHand.lock()) {
					foundItem = mainHand;
					DropItem(mainHand);
					mainHand.reset();
				}
				TaskFinished(TASKSUCCESS);
				break;

			case GETANGRY:
				aggressive = true;
				TaskFinished(TASKSUCCESS);
				break;

			case CALMDOWN:
				aggressive = false;
				TaskFinished(TASKSUCCESS);
				break;

			case STARTFIRE:
				if (!taskBegun) {
					taskBegun = true;
					timer = 0;
				} else {
					AddEffect(WORKING);
					if (++timer >= 50) {
						Game::Inst()->CreateFire(currentTarget(), 10);
						TaskFinished(TASKSUCCESS);
					}
				}
				break;

			case REPAIR:
				if (currentEntity().lock() && boost::dynamic_pointer_cast<Construction>(currentEntity().lock())) {
					tmp = boost::static_pointer_cast<Construction>(currentEntity().lock())->Repair();
					AddEffect(WORKING);
					if (tmp >= 100) {
						if (carried.lock()) { //Repairjobs usually require some material
							inventory->RemoveItem(carried);
							bulk -= carried.lock()->GetBulk();
							Game::Inst()->RemoveItem(carried);
							carried.reset();
						}
						TaskFinished(TASKSUCCESS);
					} else if (tmp < 0) {
						TaskFinished(TASKFAILFATAL, "(USE)Can not use (tmp<0)"); break;
					}
				} else { TaskFinished(TASKFAILFATAL, "(USE)Attempted to use non-construct"); break; }
				break;

			case FILLDITCH:
				if (carried.lock() && carried.lock()->IsCategory(Item::StringToItemCategory("earth"))) {
					if (Map::Inst()->GetType(currentTarget().X(), currentTarget().Y()) != TILEDITCH) {
						TaskFinished(TASKFAILFATAL, "(FILLDITCH)Target not a ditch");
						break;
					}

					if (!taskBegun) {
						taskBegun = true;
						timer = 0;
					} else {
						AddEffect(WORKING);
						if (++timer >= 50) {
							inventory->RemoveItem(carried);
							bulk -= carried.lock()->GetBulk();
							Game::Inst()->RemoveItem(carried);
							carried.reset();

							Map::Inst()->ChangeType(currentTarget().X(), currentTarget().Y(), TILEMUD);

							TaskFinished(TASKSUCCESS);
							break;
						}
					}
				} else {
					TaskFinished(TASKFAILFATAL, "(FILLDITCH)Not carrying earth");
					break;
				}
				break;

			default: TaskFinished(TASKFAILFATAL, "*BUG*Unknown task*BUG*"); break;
			}
		} else {
			if (HasEffect(DRUNK)) {
				JobManager::Inst()->NPCNotWaiting(uid);
				boost::shared_ptr<Job> drunkJob(new Job("Huh?"));
				drunkJob->internal = true;
				run = false;
				drunkJob->tasks.push_back(Task(MOVENEAR, Position()));
				jobs.push_back(drunkJob);
				if (Random::Generate(75) == 0) GoBerserk();
			} else	if (HasEffect(PANIC)) {
				JobManager::Inst()->NPCNotWaiting(uid);
				if (jobs.empty() && threatLocation.X() != 1 && threatLocation.Y() != -1) {
					boost::shared_ptr<Job> fleeJob(new Job("Flee"));
					fleeJob->internal = true;
					int dx = x - threatLocation.X();
					int dy = y - threatLocation.Y();
					if (Map::Inst()->IsWalkable(x + dx, y + dy, (void *)this)) {
						fleeJob->tasks.push_back(Task(MOVE, Coordinate(x+dx,y+dy)));
						jobs.push_back(fleeJob);
					} else if (Map::Inst()->IsWalkable(x + dx, y, (void *)this)) {
						fleeJob->tasks.push_back(Task(MOVE, Coordinate(x+dx,y)));
						jobs.push_back(fleeJob);
					} else if (Map::Inst()->IsWalkable(x, y + dy, (void *)this)) {
						fleeJob->tasks.push_back(Task(MOVE, Coordinate(x,y+dy)));
						jobs.push_back(fleeJob);
					}
				}
			} else if (!GetSquadJob(boost::static_pointer_cast<NPC>(shared_from_this())) && 
				!FindJob(boost::static_pointer_cast<NPC>(shared_from_this()))) {
				boost::shared_ptr<Job> idleJob(new Job("Idle"));
				idleJob->internal = true;
				if (faction == PLAYERFACTION) {
					if (Random::Generate(8) < 7) {
						idleJob->tasks.push_back(Task(MOVENEAR, Camp::Inst()->Center()));
					} else {
						Coordinate randomLocation(-1,-1);
						for (int tries = 0; tries < 20 && (!Map::Inst()->IsTerritory(randomLocation.X(), randomLocation.Y()) ||
							Map::Inst()->IsDangerous(randomLocation.X(), randomLocation.Y(), PLAYERFACTION)); ++tries) {
							Coordinate upperCorner = Camp::Inst()->GetUprTerritoryCorner();
							Coordinate lowerCorner = Camp::Inst()->GetLowTerritoryCorner();
							randomLocation.X(Random::Generate(upperCorner.X(), lowerCorner.X()));
							randomLocation.Y(Random::Generate(upperCorner.Y(), lowerCorner.Y()));
						}
						if (!Map::Inst()->IsTerritory(randomLocation.X(), randomLocation.Y()) ||
							Map::Inst()->IsDangerous(randomLocation.X(), randomLocation.Y(),PLAYERFACTION)) randomLocation = Camp::Inst()->Center();
						idleJob->tasks.push_back(Task(MOVENEAR, randomLocation));
					}
					if (Map::Inst()->IsTerritory(x,y)) run = false;
				} else {
					idleJob->tasks.push_back(Task(MOVENEAR, Position()));
					run = false;
				}
				idleJob->tasks.push_back(Task(WAIT, Coordinate(Random::Generate(9), 0)));
				jobs.push_back(idleJob);
			}
		}
	}

	return;
}

void NPC::StartJob(boost::shared_ptr<Job> job) {
	TaskFinished(TASKOWNDONE, "");

	if (job->RequiresTool() && (!mainHand.lock() || !mainHand.lock()->IsCategory(job->GetRequiredTool()))) {
		//We insert each one into the beginning, so these are inserted in reverse order
		job->tasks.insert(job->tasks.begin(), Task(FORGET)); /*"forget" the item we found, otherwise later tasks might
															 incorrectly refer to it */
		job->tasks.insert(job->tasks.begin(), Task(WIELD));
		job->tasks.insert(job->tasks.begin(), Task(TAKE));
		job->tasks.insert(job->tasks.begin(), Task(MOVE));
		job->tasks.insert(job->tasks.begin(), Task(FIND, Position(), boost::weak_ptr<Entity>(), job->GetRequiredTool()));
		addedTasksToCurrentJob = 5;
	}

	jobs.push_back(job);
}

TaskResult NPC::Move(TaskResult oldResult) {
	int moveX,moveY;
	if (run)
		nextMove += effectiveStats[MOVESPEED];
	else {
		if (effectiveStats[MOVESPEED]/3 == 0 && effectiveStats[MOVESPEED] != 0) ++nextMove;
		else nextMove += effectiveStats[MOVESPEED]/3;
	}
	while (nextMove > 100) {
		nextMove -= 100;
		boost::mutex::scoped_try_lock pathLock(pathMutex);
		if (pathLock.owns_lock()) {
			if (nopath) {nopath = false; return TASKFAILFATAL;}
			if (pathIndex < path->size() && pathIndex >= 0) {
				path->get(pathIndex, &moveX, &moveY); //Get next move

				if (pathIndex != path->size()-1 && Map::Inst()->NPCList(moveX, moveY)->size() > 0) {
					//Our next move target has an npc on it, and it isn't our target
					int nextX, nextY;
					path->get(pathIndex+1, &nextX, &nextY);
					/*Find a new target that is adjacent to our current, next, and the next after targets
					Effectively this makes the npc try and move around another npc, instead of walking onto
					the same tile and slowing down*/
					Map::Inst()->FindEquivalentMoveTarget(x, y, moveX, moveY, nextX, nextY, (void*)this);
				}

				//If we're about to step on a dangerous tile that we didn't plan to, repath
				if (!pathIsDangerous && Map::Inst()->IsDangerous(moveX, moveY, faction)) return TASKFAILNONFATAL;

				if (Map::Inst()->IsWalkable(moveX, moveY, (void*)this)) { //If the tile is walkable, move there
					Position(Coordinate(moveX,moveY));
					Map::Inst()->WalkOver(moveX, moveY);
					++pathIndex;
				} else { //Encountered an obstacle. Fail if the npc can't tunnel
					if (IsTunneler() && Map::Inst()->GetConstruction(moveX, moveY) >= 0) {
						Hit(Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(moveX, moveY)));
						return TASKCONTINUE;
					}
					return TASKFAILNONFATAL;
				}
				return TASKCONTINUE; //Everything is ok
			} else if (!findPathWorking) return PATHEMPTY; //No path
		}
	}
	//Can't move yet, so the earlier result is still valid
	return oldResult;
}

unsigned int NPC::pathingThreadCount = 0;
boost::mutex NPC::threadCountMutex;
void NPC::findPath(Coordinate target) {
	pathMutex.lock();
	findPathWorking = true;
	pathIsDangerous = false;
	pathIndex = 0;
	
	delete path;
	path = new TCODPath(Map::Inst()->Width(), Map::Inst()->Height(), Map::Inst(), (void*)this);

	threadCountMutex.lock();
	if (pathingThreadCount < 12) {
		++pathingThreadCount;
		threadCountMutex.unlock();
		pathMutex.unlock();
		boost::thread pathThread(boost::bind(tFindPath, path, x, y, target.X(), target.Y(), this, true));
	} else {
		threadCountMutex.unlock();
		pathMutex.unlock();
		tFindPath(path, x, y, target.X(), target.Y(), this, false);
	}
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

void NPC::GetTooltip(int x, int y, Tooltip *tooltip) {
	Entity::GetTooltip(x, y, tooltip);
	if(faction == PLAYERFACTION && !jobs.empty()) {
		boost::shared_ptr<Job> job = jobs.front();
		if(job->name != "Idle") {
			tooltip->AddEntry(TooltipEntry((boost::format("  %s") % job->name).str(), TCODColor::grey));
		}
	}
}

void NPC::color(TCODColor value, TCODColor bvalue) { _color = value; _bgcolor = bvalue; }
void NPC::graphic(int value) { _graphic = value; }
int NPC::GetGraphicsHint() const { return NPC::Presets[type].graphicsHint; }

bool NPC::Expert() {return expert;}
void NPC::Expert(bool value) {expert = value;}

Coordinate NPC::Position() {return Coordinate(x,y);}

bool NPC::Dead() { return dead; }
void NPC::Kill() {
	if (!dead) {//You can't be killed if you're already dead!
		dead = true;
		health = 0;
		if (NPC::Presets[type].deathItem >= 0) {
			int corpsenum = Game::Inst()->CreateItem(Position(), NPC::Presets[type].deathItem, false);
			boost::shared_ptr<Item> corpse = Game::Inst()->GetItem(corpsenum).lock();
			corpse->Color(_color);
			corpse->Name(corpse->Name() + "(" + name + ")");
			if (velocity > 0) {
				corpse->CalculateFlightPath(GetVelocityTarget(), velocity, GetHeight());
			}
		} else if (NPC::Presets[type].deathItem == -1) {
			Game::Inst()->CreateFilth(Position());
		}

		while (!jobs.empty()) TaskFinished(TASKFAILFATAL, std::string("dead"));

		while (!inventory->empty()) {
			boost::weak_ptr<Item> witem = inventory->GetFirstItem();
			if (boost::shared_ptr<Item> item = witem.lock()) {
				item->Position(Position());
				item->PutInContainer();
				item->SetFaction(PLAYERFACTION);
			}
			inventory->RemoveItem(witem);
		}

		if (boost::iequals(NPC::NPCTypeToString(type), "orc")) Announce::Inst()->AddMsg("An orc has died!", TCODColor::red, Position());
		else if (boost::iequals(NPC::NPCTypeToString(type), "goblin")) Announce::Inst()->AddMsg("A goblin has died!", TCODColor::red, Position());
		
		Stats::Inst()->deaths[NPC::NPCTypeToString(type)] += 1;
	}
}

void NPC::DropItem(boost::weak_ptr<Item> witem) {
	if (boost::shared_ptr<Item> item = witem.lock()) {
		inventory->RemoveItem(item);
		item->Position(Position());
		item->PutInContainer();
		bulk -= item->GetBulk();

		//If the item is a container with filth in it, spill it on the ground
		if (boost::dynamic_pointer_cast<Container>(item)) {
			boost::shared_ptr<Container> cont(boost::static_pointer_cast<Container>(item));
			if (cont->ContainsFilth() > 0) {
				Game::Inst()->CreateFilth(Position(), cont->ContainsFilth());
				cont->RemoveFilth(cont->ContainsFilth());
			}
		}
	}
}

Coordinate NPC::currentTarget() {
	if (currentTask()->target == Coordinate(-1,-1) && foundItem.lock()) {
		return foundItem.lock()->Position();
	}
	return currentTask()->target;
}

boost::weak_ptr<Entity> NPC::currentEntity() {
	if (currentTask()->entity.lock()) return currentTask()->entity;
	else if (foundItem.lock()) return boost::weak_ptr<Entity>(foundItem.lock());
	return boost::weak_ptr<Entity>();
}


void tFindPath(TCODPath *path, int x0, int y0, int x1, int y1, NPC* npc, bool threaded) {
	boost::mutex::scoped_lock pathLock(npc->pathMutex);
	boost::shared_lock<boost::shared_mutex> readCacheLock(Map::Inst()->cacheMutex);
	npc->nopath = !path->compute(x0, y0, x1, y1);

	for (int i = 0; i < path->size(); ++i) {
		int pathX, pathY;
		path->get(i, &pathX, &pathY);
		if (Map::Inst()->IsDangerousCache(pathX, pathY, npc->faction)) {
			npc->pathIsDangerous = true;
			break;//One dangerous tile = whole path considered dangerous
		}
	}

	npc->findPathWorking = false;
	if (threaded) {
		NPC::threadCountMutex.lock();
		--NPC::pathingThreadCount;
		NPC::threadCountMutex.unlock();
	}
}

bool NPC::GetSquadJob(boost::shared_ptr<NPC> npc) {
	if (boost::shared_ptr<Squad> squad = npc->MemberOf().lock()) {
		JobManager::Inst()->NPCNotWaiting(npc->uid);
		npc->aggressive = true;
		boost::shared_ptr<Job> newJob(new Job("Follow orders"));
		newJob->internal = true;

		//Priority #1, if the creature can wield a weapon get one if possible
		/*TODO: Right now this only makes friendlies take a weapon from a stockpile
		It should be expanded to allow all npc's to search for nearby weapons lying around. */
		if (!npc->mainHand.lock() && npc->GetFaction() == PLAYERFACTION && squad->Weapon() >= 0) {
			for (std::list<Attack>::iterator attacki = npc->attacks.begin(); attacki != npc->attacks.end();
				++attacki) {
					if (attacki->Type() == DAMAGE_WIELDED) {
						if (Game::Inst()->FindItemByCategoryFromStockpiles(
							squad->Weapon(), npc->Position()).lock()) {
								newJob->tasks.push_back(Task(FIND, npc->Position(), boost::shared_ptr<Entity>(), 
									squad->Weapon()));
								newJob->tasks.push_back(Task(MOVE));
								newJob->tasks.push_back(Task(TAKE));
								newJob->tasks.push_back(Task(WIELD));
								npc->jobs.push_back(newJob);
								return true;
						}
						break;
					}
			}
		}

		if (npc->WieldingRangedWeapon()) {
			if (!npc->quiver.lock()) {
				if (Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Quiver"), npc->Position()).lock()) {
						newJob->tasks.push_back(Task(FIND, npc->Position(), boost::shared_ptr<Entity>(), 
							Item::StringToItemCategory("Quiver")));
						newJob->tasks.push_back(Task(MOVE));
						newJob->tasks.push_back(Task(TAKE));
						newJob->tasks.push_back(Task(WEAR));
						npc->jobs.push_back(newJob);
						return true;
				}
			} else if (npc->quiver.lock()->empty()) {
				if (Game::Inst()->FindItemByCategoryFromStockpiles(
					npc->mainHand.lock()->GetAttack().Projectile(), npc->Position()).lock()) {
						for (int i = 0; i < 20; ++i) {
							newJob->tasks.push_back(Task(FIND, npc->Position(), boost::shared_ptr<Entity>(), 
								npc->mainHand.lock()->GetAttack().Projectile()));
							newJob->tasks.push_back(Task(MOVE));
							newJob->tasks.push_back(Task(TAKE));
							newJob->tasks.push_back(Task(QUIVER));
						}
						npc->jobs.push_back(newJob);
						return true;
				}
			}
		}

		if (!npc->armor.lock() && npc->GetFaction() == PLAYERFACTION && squad->Armor() >= 0) {
			npc->FindNewArmor();
		}

		switch (squad->GetOrder(npc->orderIndex)) { //GetOrder handles incrementing orderIndex
		case GUARD:
			if (squad->TargetCoordinate(npc->orderIndex).X() >= 0) {
				if (squad->Weapon() == Item::StringToItemCategory("Ranged weapon")) {
					Coordinate position = Map::Inst()->FindRangedAdvantage(squad->TargetCoordinate(npc->orderIndex));
					if (position.X() >= 0) {
						newJob->tasks.push_back(Task(MOVE, position));
						newJob->tasks.push_back(Task(WAIT, Coordinate(5*15)));
					}
				}

				if (newJob->tasks.empty()) {
					newJob->tasks.push_back(Task(MOVENEAR, squad->TargetCoordinate(npc->orderIndex)));
					//WAIT waits Coordinate.x / 5 seconds
					newJob->tasks.push_back(Task(WAIT, Coordinate(5*5, 0)));
				}

				if (!newJob->tasks.empty()) {
					npc->jobs.push_back(newJob);
					if (Distance(npc->Position(), squad->TargetCoordinate(npc->orderIndex)) < 10) npc->run = false;
					return true;
				}
			}
			break;

		case FOLLOW:
			if (squad->TargetEntity(npc->orderIndex).lock()) {
				newJob->tasks.push_back(Task(MOVENEAR, squad->TargetEntity(npc->orderIndex).lock()->Position(), squad->TargetEntity(npc->orderIndex)));
				npc->jobs.push_back(newJob);
				return true;
			}
			break;

		default:
			break;
		}
	} 
	return false;
}

bool NPC::JobManagerFinder(boost::shared_ptr<NPC> npc) {
	if (!npc->MemberOf().lock()) {
		JobManager::Inst()->NPCWaiting(npc->uid);
	}
	return false;
}

void NPC::PlayerNPCReact(boost::shared_ptr<NPC> npc) {
	bool surroundingsScanned = false;

	//If carrying a container and adjacent to fire, dump it on it immediately
	if (boost::shared_ptr<Item> carriedItem = npc->Carrying().lock()) {
		if (carriedItem->IsCategory(Item::StringToItemCategory("bucket")) ||
			carriedItem->IsCategory(Item::StringToItemCategory("container"))) {
				npc->ScanSurroundings(true);
				surroundingsScanned = true;
				if (npc->seenFire && Game::Inst()->Adjacent(npc->threatLocation, npc->Position())) {
					npc->DumpContainer(npc->threatLocation);
					npc->TaskFinished(TASKFAILNONFATAL);
				}
		}
	}

	if (npc->coward) {
		if (!surroundingsScanned) npc->ScanSurroundings();
		surroundingsScanned = true;
		
		//NPCs with the CHICKENHEART trait panic more than usual if they see fire
		if (npc->HasTrait(CHICKENHEART) && npc->seenFire && (npc->jobs.empty() || npc->jobs.front()->name != "Aaaaaaaah!!")) {
			while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILNONFATAL, "(FAIL)Chickenheart");
			boost::shared_ptr<Job> runAroundLikeAHeadlessChickenJob(new Job("Aaaaaaaah!!"));
			for (int i = 0; i < 30; ++i)
				runAroundLikeAHeadlessChickenJob->tasks.push_back(Task(MOVE, npc->Position() + Coordinate(Random::Generate(-2, 2), Random::Generate(-2, 2))));
			runAroundLikeAHeadlessChickenJob->internal = true;
			npc->jobs.push_back(runAroundLikeAHeadlessChickenJob);
			npc->AddEffect(PANIC);
			return;
		}

		//Cowards panic if they see aggressive unfriendlies or their attacker
		for (std::list<boost::weak_ptr<NPC> >::iterator npci = npc->nearNpcs.begin(); npci != npc->nearNpcs.end(); ++npci) {
			if ((!npc->factionPtr->IsFriendsWith(npci->lock()->GetFaction()) && npci->lock()->aggressive) || 
				npci->lock() == npc->aggressor.lock()) {
				JobManager::Inst()->NPCNotWaiting(npc->uid);
				while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILNONFATAL, "(FAIL)Enemy sighted");
				npc->AddEffect(PANIC);
			}
		}
	} else {

		//Aggressive npcs attack unfriendlies
		if (npc->aggressive) {
			if (npc->jobs.empty() || npc->currentTask()->action != KILL) {
				if (!surroundingsScanned) npc->ScanSurroundings(true);
				surroundingsScanned = true;
				for (std::list<boost::weak_ptr<NPC> >::iterator npci = npc->nearNpcs.begin(); npci != npc->nearNpcs.end(); ++npci) {
					if (!npc->factionPtr->IsFriendsWith(npci->lock()->GetFaction())) {
						JobManager::Inst()->NPCNotWaiting(npc->uid);
						boost::shared_ptr<Job> killJob(new Job("Kill "+npci->lock()->name));
						killJob->internal = true;
						killJob->tasks.push_back(Task(KILL, npci->lock()->Position(), *npci));
						while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILNONFATAL, "(FAIL)Kill enemy");
						npc->jobs.push_back(killJob);
						return;
					}
				}
			}
		}

		//Npcs without BRAVE panic if they see fire
		if (!npc->HasEffect(BRAVE)) {
			if (!surroundingsScanned) npc->ScanSurroundings();
			surroundingsScanned = true;
			if (npc->seenFire) {
				npc->AddEffect(PANIC);
				while (!npc->jobs.empty()) npc->TaskFinished(TASKFAILFATAL, "(FAIL)Seen fire");
			}
		}
	}
}


void NPC::AnimalReact(boost::shared_ptr<NPC> animal) {
	animal->ScanSurroundings();

	//Aggressive animals attack constructions/other creatures depending on faction
	if (animal->aggressive) {
		if (animal->factionPtr->GetCurrentGoal() == FACTIONDESTROY && !animal->nearConstructions.empty()) {
			for (std::list<boost::weak_ptr<Construction> >::iterator consi = animal->nearConstructions.begin(); consi != animal->nearConstructions.end(); ++consi) {
				if (boost::shared_ptr<Construction> construct = consi->lock()) {
					if (construct->HasTag(WORKSHOP) || (construct->HasTag(WALL) && Random::Generate(10) == 0)) {
						boost::shared_ptr<Job> destroyJob(new Job("Destroy "+construct->Name()));
						destroyJob->internal = true;
						destroyJob->tasks.push_back(Task(MOVEADJACENT, construct->Position(), construct));
						destroyJob->tasks.push_back(Task(KILL, construct->Position(), construct));
						while (!animal->jobs.empty()) animal->TaskFinished(TASKFAILNONFATAL);
						animal->jobs.push_back(destroyJob);
						return;
					}
				}
			}
		} else {
			for (std::list<boost::weak_ptr<NPC> >::iterator npci = animal->nearNpcs.begin(); npci != animal->nearNpcs.end(); ++npci) {
				boost::shared_ptr<NPC> otherNPC = npci->lock();
				if (otherNPC && !animal->factionPtr->IsFriendsWith(otherNPC->GetFaction())) {
					boost::shared_ptr<Job> killJob(new Job("Kill "+otherNPC->name));
					killJob->internal = true;
					killJob->tasks.push_back(Task(KILL, otherNPC->Position(), *npci));
					while (!animal->jobs.empty()) animal->TaskFinished(TASKFAILNONFATAL);
					animal->jobs.push_back(killJob);
					return;
				}
			}
		}
	}

	//Cowards run away from others
	if (animal->coward) {
		for (std::list<boost::weak_ptr<NPC> >::iterator npci = animal->nearNpcs.begin(); npci != animal->nearNpcs.end(); ++npci) {
			if (!animal->factionPtr->IsFriendsWith(npci->lock()->GetFaction())) {
				animal->AddEffect(PANIC);
			}
		}
	}

	//Animals with the 'angers' tag get angry if attacked
	if (animal->aggressor.lock() && NPC::Presets[animal->type].tags.find("angers") != NPC::Presets[animal->type].tags.end()) {
		//Turn into a hostile animal if attacked by the player's creatures
		animal->aggressive = true;
		animal->RemoveEffect(PANIC);
		animal->AddEffect(RAGE);
	}

	//All animals avoid fire
	if (animal->seenFire) {
		animal->AddEffect(PANIC);
		while (!animal->jobs.empty()) animal->TaskFinished(TASKFAILFATAL);
		return;
	}
}

void NPC::AddEffect(StatusEffectType effect) {
	AddEffect(StatusEffect(effect));
}

void NPC::AddEffect(StatusEffect effect) {
	if (effect.type == PANIC && HasEffect(BRAVE)) return; //BRAVE prevents PANIC
	if (effect.type == BRAVE && HasTrait(CHICKENHEART)) return; //CHICKENHEARTs can't be BRAVE
	if (effect.type == BRAVE && HasEffect(PANIC)) RemoveEffect(PANIC); //Becoming BRAVE stops PANIC

	if (effect.type == FLYING) isFlying = true;

	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->type == effect.type) {
			statusEffectI->cooldown = statusEffectI->cooldownDefault;
			return;
		}
	}

	statusEffects.push_back(effect);
	statusEffectsChanged = true;
}

void NPC::RemoveEffect(StatusEffectType effect) {
	if (effect == FLYING) isFlying = false;

	for (std::list<StatusEffect>::iterator statusEffectI = statusEffects.begin(); statusEffectI != statusEffects.end(); ++statusEffectI) {
		if (statusEffectI->type == effect) {
			if (statusEffectIterator == statusEffectI) ++statusEffectIterator;
			statusEffects.erase(statusEffectI);
			if (statusEffectIterator == statusEffects.end()) statusEffectIterator = statusEffects.begin();

			if (statusEffectIterator != statusEffects.end() && !statusEffectIterator->visible) {
				std::list<StatusEffect>::iterator oldIterator = statusEffectIterator;
				++statusEffectIterator;
				while (statusEffectIterator != oldIterator) {
					if (statusEffectIterator != statusEffects.end()) {
						if (statusEffectIterator->visible) break;
						++statusEffectIterator;
					}
					else statusEffectIterator = statusEffects.begin();
				}
				if (statusEffectIterator != statusEffects.end() && !statusEffectIterator->visible) statusEffectIterator = statusEffects.end();
			}

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

void NPC::AbortCurrentJob(bool remove_job) {
	boost::shared_ptr<Job> job = jobs.front();
	TaskFinished(TASKFAILFATAL, "Job aborted");
	if (remove_job) { JobManager::Inst()->RemoveJob(job); }
}

void NPC::Hit(boost::weak_ptr<Entity> target, bool careful) {
	if (target.lock()) {
		boost::shared_ptr<NPC> npc = boost::dynamic_pointer_cast<NPC>(target.lock());
		boost::shared_ptr<Construction> construction = boost::dynamic_pointer_cast<Construction>(target.lock());
		for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
			if (attacki->Cooldown() <= 0) {
				attacki->ResetCooldown();

				if (npc) {
					//First check if the target dodges the attack
					if (Random::Generate(99) < npc->effectiveStats[DODGE]) {
						continue;
					}
				}

				Attack attack = *attacki;

				if (attack.Type() == DAMAGE_WIELDED) {
					GetMainHandAttack(attack);
					if (mainHand.lock() && Random::Generate(9) == 0) DecreaseItemCondition(mainHand);
				}
				if (npc && !careful && effectiveStats[STRENGTH] >= npc->effectiveStats[NPCSIZE]) {
					if (attack.Type() == DAMAGE_BLUNT || Random::GenerateBool()) {
						Coordinate tar;
						tar.X((npc->Position().X() - x) * std::max(effectiveStats[STRENGTH] - npc->effectiveStats[NPCSIZE], 1));
						tar.Y((npc->Position().Y() - y) * std::max(effectiveStats[STRENGTH] - npc->effectiveStats[NPCSIZE], 1));
						npc->CalculateFlightPath(npc->Position()+tar, Random::Generate(25, 19 + 25));
						npc->pathIndex = -1;
					}
				}

				if (npc) {
					npc->Damage(&attack, boost::static_pointer_cast<NPC>(shared_from_this()));
					Random::Dice dice(attack.Amount());
					damageDealt += dice.Roll();
					if (HasTrait(FRESH) && damageDealt > 50) RemoveTrait(FRESH);
				} else if (construction) construction->Damage(&attack);
			}
		}	
	}
}

void NPC::FireProjectile(boost::weak_ptr<Entity> target) {
	if (boost::shared_ptr<Entity> targetEntity = target.lock()) {
		for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
			if (attacki->Type() == DAMAGE_WIELDED) {
				if (attacki->Cooldown() <= 0) {
					attacki->ResetCooldown();

					if (!quiver.lock()->empty()) {
						boost::shared_ptr<Item> projectile = quiver.lock()->GetFirstItem().lock();
						quiver.lock()->RemoveItem(projectile);
						projectile->PutInContainer();
						projectile->Position(Position());
						projectile->CalculateFlightPath(targetEntity->Position(), 100, GetHeight());
					}
				}
				break;
			}
		}
	}
}

void NPC::CastOffensiveSpell(boost::weak_ptr<Entity> target) {
	if (boost::shared_ptr<Entity> targetEntity = target.lock()) {
		for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
			if (attacki->IsProjectileMagic()) {
				if (attacki->Cooldown() <= 0) {
					attacki->ResetCooldown();
					boost::shared_ptr<Spell> spell = Game::Inst()->CreateSpell(Position(), attacki->Projectile());
					if (spell) {
						spell->CalculateFlightPath(target.lock()->Position(), Spell::Presets[attacki->Projectile()].speed, GetHeight());
					}
				}
				break;
			}
		}
	}
}


void NPC::Damage(Attack* attack, boost::weak_ptr<NPC> aggr) {
	Resistance res;

	switch (attack->Type()) {
	case DAMAGE_SLASH:
	case DAMAGE_PIERCE:
	case DAMAGE_BLUNT: res = PHYSICAL_RES; break;

	case DAMAGE_MAGIC: res = MAGIC_RES; break;

	case DAMAGE_FIRE: res = FIRE_RES; break;

	case DAMAGE_COLD: res = COLD_RES; break;

	case DAMAGE_POISON: res = POISON_RES; break;

	default: res = PHYSICAL_RES; break;
	}
						
	double resistance = (100.0 - (float)effectiveResistances[res]) / 100.0;
	int damage = (int)(Game::DiceToInt(attack->Amount()) * resistance);
	health -= damage;

	for (unsigned int effecti = 0; effecti < attack->StatusEffects()->size(); ++effecti) {
		if (Random::Generate(99) < attack->StatusEffects()->at(effecti).second) {
			AddEffect(attack->StatusEffects()->at(effecti).first);
		}
	}

	if (health <= 0) Kill();

	if (damage > 0) {
		damageReceived += damage;
		if (res == PHYSICAL_RES) {
			Game::Inst()->CreateBlood(Coordinate(
				Position().X() + Random::Generate(-1, 1),
				Position().Y() + Random::Generate(-1, 1)),
				Random::Generate(50, 50+damage*10));

			if (damage >= maxHealth / 3 && attack->Type() == DAMAGE_BLUNT && Random::Generate(10) == 0) {
				AddTrait(CRACKEDSKULL);
			}
		} else if (res == FIRE_RES && Random::Generate(std::max(2, 10-damage)) == 0) {
			AddEffect(BURNING);
		}
		if (aggr.lock()) aggressor = aggr;
		if (!jobs.empty() && boost::iequals(jobs.front()->name, "Sleep")) {
			TaskFinished(TASKFAILFATAL);
		}
	}
}

void NPC::MemberOf(boost::weak_ptr<Squad> newSquad) {
	squad = newSquad;
	if (!squad.lock()) { //NPC was removed from a squad
		//Drop weapon, quiver and armor
		std::list<boost::shared_ptr<Item> > equipment;
		if (mainHand.lock()) {
			equipment.push_back(mainHand.lock());
			mainHand.reset();
		}
		if (armor.lock()) { 
			equipment.push_back(armor.lock());
			armor.reset();
		}
		if (quiver.lock()) {
			equipment.push_back(quiver.lock());
			quiver.reset();
		}

		for (std::list<boost::shared_ptr<Item> >::iterator eqit = equipment.begin(); eqit != equipment.end(); ++eqit) {
			inventory->RemoveItem(*eqit);
			(*eqit)->Position(Position());
			(*eqit)->PutInContainer();
		}

		aggressive = false;
	}
}
boost::weak_ptr<Squad> NPC::MemberOf() {return squad;}

void NPC::Escape() {
	if (carried.lock()) {
		Announce::Inst()->AddMsg((boost::format("%s has escaped with [%s]!") % name % carried.lock()->Name()).str(), 
			TCODColor::yellow, Position());
	}
	DestroyAllItems();
	escaped = true;
}

void NPC::DestroyAllItems() {
	while (!inventory->empty()) {
		boost::weak_ptr<Item> item = inventory->GetFirstItem();
		inventory->RemoveItem(item);
		Game::Inst()->RemoveItem(item);
	}
}

bool NPC::Escaped() { return escaped; }

class NPCListener : public ITCODParserListener {

	int npcIndex;

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if (boost::iequals(str->getName(), "npc_type")) {
			if (NPC::NPCTypeNames.find(name) != NPC::NPCTypeNames.end()) {
				npcIndex = NPC::NPCTypeNames[name];
				NPC::Presets[npcIndex] = NPCPreset(name);
			} else {
				NPC::Presets.push_back(NPCPreset(name));
				NPC::NPCTypeNames[name] = NPC::Presets.size()-1;
				npcIndex = NPC::Presets.size() - 1;
			}
		} else if (boost::iequals(str->getName(), "attack")) {
			NPC::Presets[npcIndex].attacks.push_back(Attack());
		} else if (boost::iequals(str->getName(), "resistances")) {
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
		if (boost::iequals(name,"generateName")) { NPC::Presets[npcIndex].generateName = true; }
		else if (boost::iequals(name,"needsNutrition")) { NPC::Presets[npcIndex].needsNutrition = true; }
		else if (boost::iequals(name,"needsSleep")) { NPC::Presets[npcIndex].needsSleep = true; }
		else if (boost::iequals(name,"expert")) { NPC::Presets[npcIndex].expert = true; }
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
		if (boost::iequals(name,"name")) { NPC::Presets[npcIndex].name = value.s; }
		else if (boost::iequals(name,"plural")) { NPC::Presets[npcIndex].plural = value.s; }
		else if (boost::iequals(name,"speed")) { NPC::Presets[npcIndex].stats[MOVESPEED] = value.i; }
		else if (boost::iequals(name,"color")) { NPC::Presets[npcIndex].color = value.col; }
		else if (boost::iequals(name,"graphic")) { NPC::Presets[npcIndex].graphic = value.c; }
		else if (boost::iequals(name,"fallbackGraphicsSet")) { NPC::Presets[npcIndex].fallbackGraphicsSet = value.s; }
		else if (boost::iequals(name,"health")) { NPC::Presets[npcIndex].health = value.i; }
		else if (boost::iequals(name,"AI")) { NPC::Presets[npcIndex].ai = value.s; }
		else if (boost::iequals(name,"dodge")) { NPC::Presets[npcIndex].stats[DODGE] = value.i; }
		else if (boost::iequals(name,"spawnAsGroup")) { 
			NPC::Presets[npcIndex].spawnAsGroup = true;
			NPC::Presets[npcIndex].group = value.dice;
		} else if (boost::iequals(name,"type")) {
			NPC::Presets[npcIndex].attacks.back().Type(Attack::StringToDamageType(value.s));
		} else if (boost::iequals(name,"damage")) {
			NPC::Presets[npcIndex].attacks.back().Amount(value.dice);
		} else if (boost::iequals(name,"cooldown")) {
			NPC::Presets[npcIndex].attacks.back().CooldownMax(value.i);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				StatusEffectType type = StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i));
				if (StatusEffect::IsApplyableStatusEffect(type))
					NPC::Presets[npcIndex].attacks.back().StatusEffects()->push_back(std::pair<StatusEffectType, int>(type, 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NPC::Presets[npcIndex].attacks.back().StatusEffects()->at(i).second = (intptr_t)TCOD_list_get(value.list,i);
			}
		} else if (boost::iequals(name,"projectile")) {
			NPC::Presets[npcIndex].attacks.back().Projectile(Item::StringToItemType(value.s));
			if (NPC::Presets[npcIndex].attacks.back().Projectile() == -1) {
				//No item found, probably a spell then
				NPC::Presets[npcIndex].attacks.back().Projectile(Spell::StringToSpellType(value.s));
				if (NPC::Presets[npcIndex].attacks.back().Projectile() >= 0) 
					NPC::Presets[npcIndex].attacks.back().SetMagicProjectile();
			}
		} else if (boost::iequals(name,"physical")) {
			NPC::Presets[npcIndex].resistances[PHYSICAL_RES] = value.i;
		} else if (boost::iequals(name,"magic")) {
			NPC::Presets[npcIndex].resistances[MAGIC_RES] = value.i;
		} else if (boost::iequals(name,"cold")) {
			NPC::Presets[npcIndex].resistances[COLD_RES] = value.i;
		} else if (boost::iequals(name,"fire")) {
			NPC::Presets[npcIndex].resistances[FIRE_RES] = value.i;
		} else if (boost::iequals(name,"poison")) {
			NPC::Presets[npcIndex].resistances[POISON_RES] = value.i;
		} else if (boost::iequals(name,"tags")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				std::string tag = (char*)TCOD_list_get(value.list,i);
				NPC::Presets[npcIndex].tags.insert(boost::to_lower_copy(tag));
			}
		} else if (boost::iequals(name,"strength")) {
			NPC::Presets[npcIndex].stats[STRENGTH] = value.i;
		} else if (boost::iequals(name,"size")) {
			NPC::Presets[npcIndex].stats[NPCSIZE] = value.i;
			if (NPC::Presets[npcIndex].stats[STRENGTH] == 1) NPC::Presets[npcIndex].stats[STRENGTH] = value.i;
		} else if (boost::iequals(name,"tier")) {
			NPC::Presets[npcIndex].tier = value.i;
		} else if (boost::iequals(name,"death")) {
			if (boost::iequals(value.s,"filth")) NPC::Presets[npcIndex].deathItem = -1;
			else NPC::Presets[npcIndex].deathItem = Item::StringToItemType(value.s);
		} else if (boost::iequals(name,"equipOneOf")) {
			NPC::Presets[npcIndex].possibleEquipment.push_back(std::vector<int>());
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				std::string item = (char*)TCOD_list_get(value.list,i);
				NPC::Presets[npcIndex].possibleEquipment.back().push_back(Item::StringToItemType(item));
			}
		} else if (boost::iequals(name,"faction")) {
			NPC::Presets[npcIndex].faction = Faction::StringToFactionType(value.s);
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<boost::format("end of %s\n") % str->getName();
#endif
		if (NPC::Presets[npcIndex].plural == "") NPC::Presets[npcIndex].plural = NPC::Presets[npcIndex].name + "s";
		if (NPC::Presets[npcIndex].faction == -1) {
			if (NPC::Presets[npcIndex].ai == "PlayerNPC") {
				NPC::Presets[npcIndex].faction = PLAYERFACTION;
			} else if (NPC::Presets[npcIndex].ai == "PeacefulAnimal") {
				NPC::Presets[npcIndex].faction = Faction::StringToFactionType("Peaceful animal");
			} else if (NPC::Presets[npcIndex].ai == "HungryAnimal") {
				NPC::Presets[npcIndex].faction = Faction::StringToFactionType("Hostile monster");
			} else if (NPC::Presets[npcIndex].ai == "HostileAnimal") {
				NPC::Presets[npcIndex].faction = Faction::StringToFactionType("Hostile monster");
			}
		}
		return true;
	}
	void error(const char *msg) {
		throw std::runtime_error(msg);
	}
};

void NPC::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct *npcTypeStruct = parser.newStructure("npc_type");
	npcTypeStruct->addProperty("name", TCOD_TYPE_STRING, true);
	npcTypeStruct->addProperty("plural", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	npcTypeStruct->addProperty("graphic", TCOD_TYPE_CHAR, true);
	npcTypeStruct->addFlag("expert");
	const char* aiTypes[] = { "PlayerNPC", "PeacefulAnimal", "HungryAnimal", "HostileAnimal", NULL }; 
	npcTypeStruct->addValueList("AI", aiTypes, true);
	npcTypeStruct->addFlag("needsNutrition");
	npcTypeStruct->addFlag("needsSleep");
	npcTypeStruct->addFlag("generateName");
	npcTypeStruct->addProperty("spawnAsGroup", TCOD_TYPE_DICE, false);
	npcTypeStruct->addListProperty("tags", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("tier", TCOD_TYPE_INT, false);
	npcTypeStruct->addProperty("death", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("fallbackGraphicsSet", TCOD_TYPE_STRING, false);
	npcTypeStruct->addListProperty("equipOneOf", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("faction", TCOD_TYPE_STRING, false);
	
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

	TCODParserStruct *statsStruct = parser.newStructure("stats");
	statsStruct->addProperty("health", TCOD_TYPE_INT, true);
	statsStruct->addProperty("speed", TCOD_TYPE_INT, true);
	statsStruct->addProperty("dodge", TCOD_TYPE_INT, true);
	statsStruct->addProperty("size", TCOD_TYPE_INT, true);
	statsStruct->addProperty("strength", TCOD_TYPE_INT, false);

	npcTypeStruct->addStructure(attackTypeStruct);
	npcTypeStruct->addStructure(resistancesStruct);
	npcTypeStruct->addStructure(statsStruct);

	NPCListener listener = NPCListener();
	parser.run(filename.c_str(), &listener);
}

std::string NPC::NPCTypeToString(NPCType type) {
	if (type >= 0 && type < Presets.size())
		return Presets[type].typeName;
	return "Nobody";
}

NPCType NPC::StringToNPCType(std::string typeName) {
	if (NPCTypeNames.find(typeName) == NPCTypeNames.end()) {
		return -1;
	}
	return NPCTypeNames[typeName];
}

int NPC::GetNPCSymbol() { return Presets[type].graphic; }

void NPC::InitializeAIFunctions() {
	FindJob = boost::bind(&Faction::FindJob, Faction::factions[faction], _1);
	React = boost::bind(NPC::AnimalReact, _1);

	if (NPC::Presets[type].ai == "PlayerNPC") {
		FindJob = boost::bind(NPC::JobManagerFinder, _1);
		React = boost::bind(NPC::PlayerNPCReact, _1);
	}
}

void NPC::GetMainHandAttack(Attack &attack) {
	attack.Type(DAMAGE_BLUNT);
	if (boost::shared_ptr<Item> weapon = mainHand.lock()) {
		Attack wAttack = weapon->GetAttack();
		attack.Type(wAttack.Type());
		attack.AddDamage(wAttack.Amount());
		attack.Projectile(wAttack.Projectile());
		for (std::vector<std::pair<StatusEffectType, int> >::iterator effecti = wAttack.StatusEffects()->begin();
			effecti != wAttack.StatusEffects()->end(); ++effecti) {
				attack.StatusEffects()->push_back(*effecti);
		}
	}
}

bool NPC::WieldingRangedWeapon() {
	if (boost::shared_ptr<Item> weapon = mainHand.lock()) {
		Attack wAttack = weapon->GetAttack();
		return wAttack.Type() == DAMAGE_RANGED;
	}
	return false;
}

void NPC::FindNewWeapon() {
	int weaponValue = 0;
	if (mainHand.lock() && mainHand.lock()->IsCategory(squad.lock()->Weapon())) {
		weaponValue = mainHand.lock()->RelativeValue();
	}
	ItemCategory weaponCategory = squad.lock() ? squad.lock()->Weapon() : Item::StringToItemCategory("Weapon");
	boost::weak_ptr<Item> newWeapon = Game::Inst()->FindItemByCategoryFromStockpiles(weaponCategory, Position(), BETTERTHAN, weaponValue);
	if (boost::shared_ptr<Item> weapon = newWeapon.lock()) {
		boost::shared_ptr<Job> weaponJob(new Job("Grab weapon"));
		weaponJob->internal = true;
		weaponJob->ReserveEntity(weapon);
		weaponJob->tasks.push_back(Task(MOVE, weapon->Position()));
		weaponJob->tasks.push_back(Task(TAKE, weapon->Position(), weapon));
		weaponJob->tasks.push_back(Task(WIELD));
		jobs.push_back(weaponJob);
	}
}

void NPC::FindNewArmor() {
	int armorValue = 0;
	if (armor.lock() && armor.lock()->IsCategory(squad.lock()->Armor())) {
		armorValue = armor.lock()->RelativeValue();
	}
	ItemCategory armorCategory = squad.lock() ? squad.lock()->Armor() : Item::StringToItemCategory("Armor");
	boost::weak_ptr<Item> newArmor = Game::Inst()->FindItemByCategoryFromStockpiles(armorCategory, Position(), BETTERTHAN, armorValue);
	if (boost::shared_ptr<Item> arm = newArmor.lock()) {
		boost::shared_ptr<Job> armorJob(new Job("Grab armor"));
		armorJob->internal = true;
		armorJob->ReserveEntity(arm);
		armorJob->tasks.push_back(Task(MOVE, arm->Position()));
		armorJob->tasks.push_back(Task(TAKE, arm->Position(), arm));
		armorJob->tasks.push_back(Task(WEAR));
		jobs.push_back(armorJob);
	}
}

boost::weak_ptr<Item> NPC::Wielding() {
	return mainHand;
}

boost::weak_ptr<Item> NPC::Carrying() const {
	return carried;
}

boost::weak_ptr<Item> NPC::Wearing() {
	return armor;
}

bool NPC::HasHands() { return hasHands; }

void NPC::UpdateVelocity() {
	if (velocity > 0) {
		nextVelocityMove += velocity;
		while (nextVelocityMove > 100) {
			nextVelocityMove -= 100;
			if (flightPath.size() > 0) {
				if (flightPath.back().height < ENTITYHEIGHT) { //We're flying low enough to hit things
					int tx = flightPath.back().coord.X();
					int ty = flightPath.back().coord.Y();
					if (Map::Inst()->BlocksWater(tx,ty)) { //We've hit an obstacle
						health -= velocity/5;
						AddEffect(CONCUSSION);

						if (Map::Inst()->GetConstruction(tx,ty) > -1) {
							if (boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(tx,ty)).lock()) {
								Attack attack;
								attack.Type(DAMAGE_BLUNT);
								TCOD_dice_t damage;
								damage.addsub = (float)velocity/5;
								damage.multiplier = 1;
								damage.nb_dices = 1;
								damage.nb_faces = 5 + effectiveStats[NPCSIZE];
								construct->Damage(&attack);
							}
						}

						SetVelocity(0);
						flightPath.clear();
						return;
					}
					if (Map::Inst()->NPCList(tx,ty)->size() > 0 && Random::Generate(9) < (signed int)(2 + Map::Inst()->NPCList(tx,ty)->size())) {
						health -= velocity/5;
						AddEffect(CONCUSSION);
						SetVelocity(0);
						flightPath.clear();
						return;
					}
				}
				if (flightPath.back().height == 0) {
					health -= velocity/5;
					AddEffect(CONCUSSION);
					SetVelocity(0);
				}
				Position(flightPath.back().coord);
				
				flightPath.pop_back();
			} else SetVelocity(0);
		}
	} else { //We're not hurtling through air so let's tumble around if we're stuck on unwalkable terrain
		if (!Map::Inst()->IsWalkable(x, y, (void*)this)) {
			for (int radius = 1; radius < 10; ++radius) {
				for (unsigned int xi = x - radius; xi <= x + radius; ++xi) {
					for (unsigned int yi = y - radius; yi <= y + radius; ++yi) {
						if (Map::Inst()->IsWalkable(xi, yi, (void*)this)) {
							Position(Coordinate(xi, yi));
							return;
						}
					}
				}
			}
		}
	}
}

void NPC::PickupItem(boost::weak_ptr<Item> item) {
	if (item.lock()) {
		carried = boost::static_pointer_cast<Item>(item.lock());
		bulk += item.lock()->GetBulk();
		if (!inventory->AddItem(carried)) Announce::Inst()->AddMsg("No space in inventory");
	}
}

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
	tags(std::set<std::string>()),
	tier(0),
	deathItem(-2),
	fallbackGraphicsSet(),
	graphicsHint(-1),
	faction(-1)
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

int NPC::GetHealth() { return health; }
int NPC::GetMaxHealth() { return maxHealth; }

void NPC::AbortJob(boost::weak_ptr<Job> wjob) {
	if (boost::shared_ptr<Job> job = wjob.lock()) {
		for (std::deque<boost::shared_ptr<Job> >::iterator jobi = jobs.begin(); jobi != jobs.end(); ++jobi) {
			if (*jobi == job) {
				if (job == jobs.front()) {
					TaskFinished(TASKFAILFATAL, "(AbortJob)");
				}
				return;
			}
		}
	}
}

bool NPC::IsTunneler() { return isTunneler; }

void NPC::ScanSurroundings(bool onlyHostiles) {
	nearNpcs.clear();
	nearConstructions.clear();
	threatLocation = Coordinate(-1,-1);
	seenFire = false;
	for (int endx = std::max((signed int)x - LOS_DISTANCE, 0); endx <= std::min((signed int)x + LOS_DISTANCE, Map::Inst()->Width()-1); endx += 2) {
		for (int endy = std::max((signed int)y - LOS_DISTANCE, 0); endy <= std::min((signed int)y + LOS_DISTANCE, Map::Inst()->Height()-1); endy += 2) {
			if (endx == std::max((signed int)x - LOS_DISTANCE, 0) || endx == std::min((signed int)x + LOS_DISTANCE, Map::Inst()->Width()-1)
				|| endy == std::max((signed int)y - LOS_DISTANCE, 0) || endy == std::min((signed int)y + LOS_DISTANCE, Map::Inst()->Height()-1)) {
					int tx = x;
					int ty = y;
					TCODLine::init(tx, ty, endx, endy);
					do {
						/*Check constructions before checking for lightblockage because we can see a wall
						even though we can't see through it*/
						int constructUid = Map::Inst()->GetConstruction(tx,ty);
						if (constructUid >= 0) {
							nearConstructions.push_back(Game::Inst()->GetConstruction(constructUid));
						}

						//Stop moving along this line if our view is blocked
						if (Map::Inst()->BlocksLight(tx,ty) && GetHeight() < ENTITYHEIGHT) break;

						//Add all the npcs on this tile, or only hostiles if that boolean is set
						for (std::set<int>::iterator npci = Map::Inst()->NPCList(tx,ty)->begin(); npci != Map::Inst()->NPCList(tx,ty)->end(); ++npci) {
							if (*npci != uid) {
								if (!factionPtr->IsFriendsWith(Game::Inst()->GetNPC(*npci)->GetFaction())) threatLocation = Coordinate(tx,ty);
								if (!onlyHostiles || 
									(onlyHostiles && !factionPtr->IsFriendsWith(Game::Inst()->GetNPC(*npci)->GetFaction()))) 
									nearNpcs.push_back(Game::Inst()->GetNPC(*npci));
							}
						}

						//Only care about fire if we're not flying and not effectively immune
						if (!HasEffect(FLYING) && Map::Inst()->GetFire(tx,ty).lock()) {
							if (effectiveResistances[FIRE_RES] < 90) {
								threatLocation = Coordinate(tx,ty);
								seenFire = true;
							}
						}

						/*Stop if we already see many npcs, otherwise this can start to bog down in
						high traffic places*/
						if (nearNpcs.size() > 10) break;

					} while(!TCODLine::step(&tx, &ty));
			}
		}
	}
}

void NPC::AddTrait(Trait trait) { 
	traits.insert(trait); 
	switch (trait) {
	case CRACKEDSKULL:
		AddEffect(CRACKEDSKULLEFFECT);
		break;

	default: break;
	}
}

void NPC::RemoveTrait(Trait trait) { 
	traits.erase(trait); 
	switch (trait) {
	case FRESH:
		_color.g = std::max(0, _color.g - 100);
		break;
		
	default: break;
	}
}

bool NPC::HasTrait(Trait trait) { return traits.find(trait) != traits.end(); }

void NPC::GoBerserk() {
	ScanSurroundings();
	if (boost::shared_ptr<Item> carriedItem = carried.lock()) {
		inventory->RemoveItem(carriedItem);
		carriedItem->PutInContainer();
		carriedItem->Position(Position());
		Coordinate target(-1,-1);
		if (!nearNpcs.empty()) {
			boost::shared_ptr<NPC> creature = boost::next(nearNpcs.begin(), Random::ChooseIndex(nearNpcs))->lock();
			if (creature) target = creature->Position();
		}
		if (target.X() == -1) target = Coordinate(Random::Generate(x-7, x+7), Random::Generate(y-7, y+7));
		carriedItem->CalculateFlightPath(target, 50, GetHeight());
	}
	carried.reset();

	while (!jobs.empty()) TaskFinished(TASKFAILFATAL, "(FAIL)Gone berserk");

	if (!nearNpcs.empty()) {
		boost::shared_ptr<NPC> creature = boost::next(nearNpcs.begin(), Random::ChooseIndex(nearNpcs))->lock();
		boost::shared_ptr<Job> berserkJob(new Job("Berserk!"));
		berserkJob->internal = true;
		berserkJob->tasks.push_back(Task(KILL, creature->Position(), creature));
		jobs.push_back(berserkJob);
	}

	AddEffect(RAGE);
}

void NPC::ApplyEffects(boost::shared_ptr<Item> item) {
	if (item) {
		for (std::vector<std::pair<StatusEffectType, int> >::iterator addEffecti = Item::Presets[item->Type()].addsEffects.begin();
			addEffecti != Item::Presets[item->Type()].addsEffects.end(); ++addEffecti) {
				if (Random::Generate(99) < addEffecti->second)
					AddEffect(addEffecti->first);
		}
		for (std::vector<std::pair<StatusEffectType, int> >::iterator remEffecti = Item::Presets[item->Type()].removesEffects.begin();
			remEffecti != Item::Presets[item->Type()].removesEffects.end(); ++remEffecti) {
				if (Random::Generate(99) < remEffecti->second) {
					RemoveEffect(remEffecti->first);
					if (remEffecti->first == DROWSY) weariness = 0; //Special case, the effect would come straight back otherwise
				}
		}
	}
}

void NPC::UpdateHealth() {
	if (health <= 0) {Kill(); return;}
	if (health > maxHealth) health = maxHealth;

	if (Random::Generate(UPDATES_PER_SECOND*10) == 0 && health < maxHealth) ++health;

	if (faction == PLAYERFACTION && health < maxHealth / 2 && !HasEffect(HEALING)) {
		bool healJobFound = false;
		for (std::deque<boost::shared_ptr<Job> >::iterator jobi = jobs.begin(); jobi != jobs.end(); ++jobi) {
			if ((*jobi)->name.find("Heal") != std::string::npos) {
				healJobFound = true;
				break;
			}
		}

		if (!healJobFound && Item::GoodEffectAdders.find(HEALING) != Item::GoodEffectAdders.end()) {
			boost::shared_ptr<Item> healItem;
			for (std::multimap<StatusEffectType, ItemType>::iterator fixi = Item::GoodEffectAdders.equal_range(HEALING).first;
				fixi != Item::GoodEffectAdders.equal_range(HEALING).second && !healItem; ++fixi) {
					healItem = Game::Inst()->FindItemByTypeFromStockpiles(fixi->second, Position()).lock();
			}
			if (healItem) {
				boost::shared_ptr<Job> healJob(new Job("Heal"));
				healJob->internal = true;
				healJob->ReserveEntity(healItem);
				healJob->tasks.push_back(Task(MOVE, healItem->Position()));
				healJob->tasks.push_back(Task(TAKE, healItem->Position(), healItem));
				if (healItem->IsCategory(Item::StringToItemCategory("drink")))
					healJob->tasks.push_back(Task(DRINK));
				else
					healJob->tasks.push_back(Task(EAT));
				jobs.push_back(healJob);
			}
		}
	}
}

/*I opted to place this in NPC instead of it being a method of Item mainly because Item won't know
if it's being wielded, worn or whatever, and that's important information when an axe breaks in an
orc's hand, for exmple*/
void NPC::DecreaseItemCondition(boost::weak_ptr<Item> witem) {
	if (boost::shared_ptr<Item> item = witem.lock()) {
		int condition = item->DecreaseCondition();
		if (condition == 0) { //< 0 == does not break, > 0 == not broken
			inventory->RemoveItem(item);
			if (carried.lock() == item) carried.reset();
			if (mainHand.lock() == item) {
				mainHand.reset();
				if (currentJob().lock() && currentJob().lock()->RequiresTool()) {
					TaskFinished(TASKFAILFATAL, "(FAIL)Wielded item broken");
				}
			}
			if (offHand.lock() == item) offHand.reset();
			if (armor.lock() == item) armor.reset();
			if (quiver.lock() == item) quiver.reset();
			std::vector<boost::weak_ptr<Item> > component(1, item);
			Game::Inst()->CreateItem(Position(), Item::StringToItemType("debris"), false, -1, component);
			Game::Inst()->RemoveItem(item);
		}
	}
}

void NPC::DumpContainer(Coordinate location) {
	boost::shared_ptr<Container> sourceContainer;
	if (carried.lock() && (carried.lock()->IsCategory(Item::StringToItemCategory("Bucket")) ||
		carried.lock()->IsCategory(Item::StringToItemCategory("Container")))) {
		sourceContainer = boost::static_pointer_cast<Container>(carried.lock());
	} else if (mainHand.lock() && (mainHand.lock()->IsCategory(Item::StringToItemCategory("Bucket")) ||
		mainHand.lock()->IsCategory(Item::StringToItemCategory("Container")))) {
		sourceContainer = boost::static_pointer_cast<Container>(mainHand.lock());
	}

	if (sourceContainer) {
		if (location.X() >= 0 && location.Y() >= 0 && 
			location.X() < Map::Inst()->Width() && location.Y() < Map::Inst()->Height()) {
				if (sourceContainer->ContainsWater() > 0) {
					Game::Inst()->CreateWater(location, sourceContainer->ContainsWater());
					sourceContainer->RemoveWater(sourceContainer->ContainsWater());
				} else {
					Game::Inst()->CreateFilth(location, sourceContainer->ContainsFilth());
					sourceContainer->RemoveFilth(sourceContainer->ContainsFilth());
				}
		}
	}
}

//Checks all the current job's tasks to see if they are potentially doable
void NPC::ValidateCurrentJob() {
	if (!jobs.empty()) {
		//Only check tasks from the current one onwards
		for (int i = taskIndex; i < jobs.front()->tasks.size(); ++i) {
			switch (jobs.front()->tasks[i].action) {

			case FELL:
			case HARVESTWILDPLANT:
				if (!jobs.front()->tasks[i].entity.lock() || !boost::dynamic_pointer_cast<NatureObject>(jobs.front()->tasks[i].entity.lock())) {
					TaskFinished(TASKFAILFATAL, "(FELL/HARVESTWILDPLANT)Target doesn't exist");
					return;
				} else if (!boost::static_pointer_cast<NatureObject>(jobs.front()->tasks[i].entity.lock())->Marked()) {
					TaskFinished(TASKFAILFATAL, "(FELL/HARVESTWILDPLANT)Target not marked");
					return;
				}
				break;

			case POUR:
				if (!boost::iequals(jobs.front()->name, "Dump filth")) { //Filth dumping is the one time we want to pour liquid onto an unmarked tile
					if (!jobs.front()->tasks[i].entity.lock() && !Map::Inst()->GroundMarked(jobs.front()->tasks[i].target.X(), jobs.front()->tasks[i].target.Y())) {
						TaskFinished(TASKFAILFATAL, "(POUR)Target does not exist");
						return;
					}
				}
				break;

			case PUTIN:
				if (!jobs.front()->tasks[i].entity.lock()) {
					TaskFinished(TASKFAILFATAL, "(PUTIN)Target doesn't exist");
					return;
				}
				break;

			default: break; //Non-validatable tasks
			}
		}
	}
}

int NPC::GetHeight() {
	if (!flightPath.empty()) return flightPath.back().height;
	if (HasEffect(FLYING) || HasEffect(HIGHGROUND)) return ENTITYHEIGHT+2;
	return 0;
}

bool NPC::IsFlying() { return isFlying; }

void NPC::SetFaction(int newFaction) {
	if (newFaction >= 0 && newFaction < Faction::factions.size()) {
		faction = newFaction;
		factionPtr = Faction::factions[newFaction];
	} else if (!Faction::factions.empty()) {
		factionPtr = Faction::factions[0];
	}
}

void NPC::save(OutputArchive& ar, const unsigned int version) const {
	ar.register_type<Container>();
	ar.register_type<Item>();
	ar.register_type<Entity>();
	ar.register_type<SkillSet>();
	ar & boost::serialization::base_object<Entity>(*this);
	std::string npcType(NPC::NPCTypeToString(type));
	ar & npcType;
	ar & timeCount;
	ar & jobs;
	ar & taskIndex;
	ar & orderIndex;
	ar & nopath;
	ar & findPathWorking;
	ar & timer;
	ar & nextMove;
	ar & run;
	ar & _color.r;
	ar & _color.g;
	ar & _color.b;
	ar & _bgcolor.r;
	ar & _bgcolor.g;
	ar & _bgcolor.b;
	ar & _graphic;
	ar & taskBegun;
	ar & expert;
	ar & carried;
	ar & mainHand;
	ar & offHand;
	ar & armor;
	ar & quiver;
	ar & thirst;
	ar & hunger;
	ar & weariness;
	ar & thinkSpeed;
	ar & statusEffects;
	ar & health;
	ar & maxHealth;
	ar & foundItem;
	ar & inventory;
	ar & needsNutrition;
	ar & needsSleep;
	ar & hasHands;
	ar & isTunneler;
	ar & baseStats;
	ar & effectiveStats;
	ar & baseResistances;
	ar & effectiveResistances;
	ar & aggressive;
	ar & coward;
	ar & aggressor;
	ar & dead;
	ar & squad;
	ar & attacks;
	ar & escaped;
	ar & addedTasksToCurrentJob;
	ar & Skills;
	ar & hasMagicRangedAttacks;
	ar & traits;
	ar & damageDealt;
	ar & damageReceived;
	ar & jobBegun;
}

void NPC::load(InputArchive& ar, const unsigned int version) {
	ar.register_type<Container>();
	ar.register_type<Item>();
	ar.register_type<Entity>();
	ar.register_type<SkillSet>();
	ar & boost::serialization::base_object<Entity>(*this);
	std::string typeName;
	ar & typeName;
	type = -1;
	bool failedToFindType = false;
	type = NPC::StringToNPCType(typeName);
	if (type == -1) { //Apparently a creature type that doesn't exist
		type = 2; //Whatever the first monster happens to be
		failedToFindType = true; //We'll allow loading, this creature will just immediately die
	}

	ar & timeCount;
	ar & jobs;
	ar & taskIndex;
	ar & orderIndex;
	ar & nopath;
	ar & findPathWorking;
	ar & timer;
	ar & nextMove;
	ar & run;
	ar & _color.r;
	ar & _color.g;
	ar & _color.b;
	ar & _bgcolor.r;
	ar & _bgcolor.g;
	ar & _bgcolor.b;
	ar & _graphic;
	ar & taskBegun;
	ar & expert;
	ar & carried;
	ar & mainHand;
	ar & offHand;
	ar & armor;
	ar & quiver;
	ar & thirst;
	ar & hunger;
	ar & weariness;
	ar & thinkSpeed;
	ar & statusEffects;
	ar & health;
	if (failedToFindType) health = 0;
	ar & maxHealth;
	ar & foundItem;
	ar & inventory;
	ar & needsNutrition;
	ar & needsSleep;
	ar & hasHands;
	ar & isTunneler;
	ar & baseStats;
	ar & effectiveStats;
	ar & baseResistances;
	ar & effectiveResistances;
	ar & aggressive;
	ar & coward;
	ar & aggressor;
	ar & dead;
	ar & squad;
	ar & attacks;
	ar & escaped;
	ar & addedTasksToCurrentJob;
	ar & Skills;
	ar & hasMagicRangedAttacks;
	ar & traits;
	ar & damageDealt;
	ar & damageReceived;
	if (version >= 1) {
		ar & jobBegun;
	}

	SetFaction(faction); //Required to initialize factionPtr
	InitializeAIFunctions();
}
