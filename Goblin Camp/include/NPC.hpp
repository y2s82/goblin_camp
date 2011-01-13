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

#include <queue>
#include <list>

#include <boost/serialization/split_member.hpp>

#include <boost/thread/thread.hpp>
#include <boost/multi_array.hpp>
#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>

#include <libtcod.hpp>

#include "Coordinate.hpp"
#include "Job.hpp"
#include "Entity.hpp"
#include "Container.hpp"
#include "StatusEffect.hpp"
#include "Squad.hpp"
#include "Attack.hpp"

#define LOS_DISTANCE 12
#define MAXIMUM_JOB_ATTEMPTS 5

#define THIRST_THRESHOLD (UPDATES_PER_SECOND * 60 * 10)
#define HUNGER_THRESHOLD 18000
#define WEARY_THRESHOLD (UPDATES_PER_SECOND * 60 * 12)
#define DRINKABLE_WATER_DEPTH 2
#define WALKABLE_WATER_DEPTH 1

typedef int NPCType;

enum AiThink {
	AINOTHING,
	AIMOVE
};

enum Skill {
	MASONRY,
	CARPENTRY,
	SKILLAMOUNT
};

class SkillSet {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	int skills[SKILLAMOUNT];
public:
	SkillSet();
	int operator()(Skill);
	void operator()(Skill, int);
};

struct NPCPreset {
	NPCPreset(std::string);
	std::string typeName;
	std::string name;
	std::string plural;
	TCODColor color;
	int graphic;
	bool expert;
	int health;
	std::string ai;
	bool needsNutrition;
	bool needsSleep;
	bool generateName;
	int stats[STAT_COUNT];
	int resistances[RES_COUNT];
	bool spawnAsGroup;
	TCOD_dice_t group;
	std::list<Attack> attacks;
	std::set<std::string> tags;
	int tier;
	ItemType deathItem;
	std::string graphicsSet;
	std::string fallbackGraphicsSet;
	int graphicsHint;
};

class NPC : public Entity {
	friend class boost::serialization::access;
	friend class Game;
	friend class NPCListener;

private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	NPC(Coordinate = Coordinate(0,0),
		boost::function<bool(boost::shared_ptr<NPC>)> findJob = boost::function<bool(boost::shared_ptr<NPC>)>(),
		boost::function<void(boost::shared_ptr<NPC>)> react = boost::function<void(boost::shared_ptr<NPC>)>());
	NPCType type;
	int timeCount;
	std::deque<boost::shared_ptr<Job> > jobs;
	int taskIndex;
	int orderIndex;
	boost::mutex pathMutex;
	TCODPath *path;
	int pathIndex;
	bool nopath;
	bool findPathWorking;
	int timer;
	unsigned int nextMove;
	TaskResult lastMoveResult;
	bool run;
	TCODColor _color, _bgcolor;
	int _graphic;
	bool taskBegun;
	bool expert;

	boost::weak_ptr<Item> carried;
	boost::weak_ptr<Item> mainHand;
	boost::weak_ptr<Item> offHand;
	boost::weak_ptr<Item> armor;
	boost::weak_ptr<Container> quiver;

	int thirst, hunger, weariness;
	int thinkSpeed;
	std::list<StatusEffect> statusEffects;
	std::list<StatusEffect>::iterator statusEffectIterator;
	int statusGraphicCounter;
	void HandleThirst();
	void HandleHunger();
	void HandleWeariness();
	int health, maxHealth;
	boost::weak_ptr<Item> foundItem;
	boost::shared_ptr<Container> inventory;

	std::list<boost::weak_ptr<NPC> >nearNpcs;
	bool needsNutrition;
	bool needsSleep;
	bool hasHands;
	bool isTunneler;

	boost::function<bool(boost::shared_ptr<NPC>)> FindJob;
	boost::function<void(boost::shared_ptr<NPC>)> React;

	int baseStats[STAT_COUNT];
	int effectiveStats[STAT_COUNT];
	int baseResistances[RES_COUNT];
	int effectiveResistances[RES_COUNT];
	bool aggressive, coward;
	boost::weak_ptr<NPC> aggressor;
	bool dead;
	boost::weak_ptr<Squad> squad;

	std::list<Attack> attacks;

	bool escaped;
	bool Escaped();
	void Escape();
	void DestroyAllItems();
	void UpdateStatusEffects();

	static std::map<std::string, NPCType> NPCTypeNames;

	void UpdateVelocity();
	int addedTasksToCurrentJob;

public:
	~NPC();
	SkillSet Skills;
	AiThink Think();
	void Update();
	void Draw(Coordinate, TCODConsole*);
	virtual void GetTooltip(int x, int y, Tooltip *tooltip);
	void Position(Coordinate,bool);
	virtual void Position(Coordinate);
	virtual Coordinate Position();
	void speed(unsigned int);
	unsigned int speed();
	void color(TCODColor,TCODColor=TCODColor::black);
	void graphic(int);

	int GraphicsHint() const;

	Task* currentTask();
	Task* nextTask();
	boost::weak_ptr<Job> currentJob();
	Coordinate currentTarget();
	boost::weak_ptr<Entity> currentEntity();
	void TaskFinished(TaskResult, std::string = "");
	TaskResult Move(TaskResult);
	void findPath(Coordinate);
	void StartJob(boost::shared_ptr<Job>);
	void AddEffect(StatusEffectType);
	void AddEffect(StatusEffect);
	void RemoveEffect(StatusEffectType);
	bool HasEffect(StatusEffectType);
	std::list<StatusEffect>* StatusEffects();
	void AbortCurrentJob(bool);
	void AbortJob(boost::weak_ptr<Job>);

	bool Expert();
	void Expert(bool);

	bool Dead();
	void Kill();
	void PickupItem(boost::weak_ptr<Item>);
	void DropItem(boost::weak_ptr<Item>);
	void Hit(boost::weak_ptr<Entity>, bool careful = false);
	void FireProjectile(boost::weak_ptr<Entity>);
	void Damage(Attack*, boost::weak_ptr<NPC> aggr = boost::weak_ptr<NPC>());

	void MemberOf(boost::weak_ptr<Squad>);
	boost::weak_ptr<Squad> MemberOf();
	void GetMainHandAttack(Attack&);
	bool WieldingRangedWeapon();
	void FindNewWeapon();
	boost::weak_ptr<Item> Wielding();
	bool HasHands();
	bool IsTunneler();
	void FindNewArmor();
	boost::weak_ptr<Item> Wearing();

	int GetHealth();
	int GetMaxHealth();

	static void LoadPresets(std::string);
	static std::vector<NPCPreset> Presets;
	static std::string NPCTypeToString(NPCType);
	static NPCType StringToNPCType(std::string);
	int GetNPCSymbol();

	void InitializeAIFunctions();

	static bool GetSquadJob(boost::shared_ptr<NPC>);
	static bool JobManagerFinder(boost::shared_ptr<NPC>);
	static void PlayerNPCReact(boost::shared_ptr<NPC>);
	static void PeacefulAnimalReact(boost::shared_ptr<NPC>);
	static bool PeacefulAnimalFindJob(boost::shared_ptr<NPC>);
	static void HostileAnimalReact(boost::shared_ptr<NPC>);
	static bool HostileAnimalFindJob(boost::shared_ptr<NPC>);
	static bool HungryAnimalFindJob(boost::shared_ptr<NPC>);

	static unsigned int pathingThreadCount;
	static boost::mutex threadCountMutex;
};

void tFindPath(TCODPath*, int, int, int, int, boost::mutex*, bool*, bool*, bool);
