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
#pragma once

#include <queue>
#include <list>

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

#include "data/Serialization.hpp"

#define LOS_DISTANCE 12
#define MAXIMUM_JOB_ATTEMPTS 5

#define THIRST_THRESHOLD (UPDATES_PER_SECOND * 60 * 10)
#define HUNGER_THRESHOLD (MONTH_LENGTH * 6)
#define WEARY_THRESHOLD (UPDATES_PER_SECOND * 60 * 12)
#define DRINKABLE_WATER_DEPTH 2
#define WALKABLE_WATER_DEPTH 1

typedef int NPCType;

class Faction;

enum Trait {
	FRESH,
	CHICKENHEART,
	VETERAN,
	BOSS,
	CRACKEDSKULL
};

enum Skill {
	MASONRY,
	CARPENTRY,
	SKILLAMOUNT
};

class SkillSet {
	GC_SERIALIZABLE_CLASS
	
	int skills[SKILLAMOUNT];
public:
	SkillSet();
	int operator()(Skill);
	void operator()(Skill, int);
};

BOOST_CLASS_VERSION(SkillSet, 0)

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
	std::string fallbackGraphicsSet;
	int graphicsHint;
	std::vector<std::vector<int> > possibleEquipment;
	int faction;
};

class NPC : public Entity {
	GC_SERIALIZABLE_CLASS
	
	friend class Game;
	friend class NPCListener;
	friend class Faction;
	friend void tFindPath(TCODPath*, int, int, int, int, NPC*, bool);
	
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
	bool pathIsDangerous;

	int timer;
	unsigned int nextMove;
	TaskResult lastMoveResult;
	bool run;

	TCODColor _color, _bgcolor;
	int _graphic;

	bool taskBegun;
	bool jobBegun;
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

	std::list<boost::weak_ptr<NPC> > nearNpcs;
	std::list<boost::weak_ptr<NPC> > adjacentNpcs;
	std::list<boost::weak_ptr<Construction> > nearConstructions;
	bool needsNutrition;
	bool needsSleep;
	bool hasHands;
	bool isTunneler;
	bool isFlying;

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
	bool Escaped() const;
	void Escape();
	void DestroyAllItems();
	void UpdateStatusEffects();

	static std::map<std::string, NPCType> NPCTypeNames;

	void UpdateVelocity();
	int addedTasksToCurrentJob;

	bool hasMagicRangedAttacks;

	void ScanSurroundings(bool onlyHostiles=false);
	Coordinate threatLocation;
	bool seenFire;

	std::set<Trait> traits;
	int damageDealt, damageReceived;
	bool statusEffectsChanged;

	void UpdateHealth();
	boost::shared_ptr<Faction> factionPtr;

	std::string GetDeathMsg();
	std::string GetDeathMsgStrengthLoss();
	std::string GetDeathMsgCombat(boost::weak_ptr<NPC> other, DamageType);
	std::string GetDeathMsgThirst();
	std::string GetDeathMsgHunger();

	Map* map;

public:
	typedef std::list<StatusEffect>::iterator StatusEffectIterator;

	~NPC();
	virtual Coordinate Position() const;
	virtual void Position(const Coordinate&);
	void Position(const Coordinate&, bool firstTime);
	SkillSet Skills;
	void Think();
	void Update();
	void Draw(Coordinate, TCODConsole*);
	virtual void GetTooltip(int x, int y, Tooltip *tooltip);
	void speed(unsigned int);
	unsigned int speed() const;
	void color(TCODColor,TCODColor=TCODColor::black);
	void graphic(int);

	int GetGraphicsHint() const;

	Task* currentTask() const;
	Task* nextTask() const;
	boost::weak_ptr<Job> currentJob() const;
	Coordinate currentTarget() const;
	boost::weak_ptr<Entity> currentEntity() const;
	void TaskFinished(TaskResult, std::string = "");
	TaskResult Move(TaskResult);
	void findPath(Coordinate);
	bool IsPathWalkable();
	void StartJob(boost::shared_ptr<Job>);
	void AddEffect(StatusEffectType);
	void AddEffect(StatusEffect);
	void RemoveEffect(StatusEffectType);
	bool HasEffect(StatusEffectType) const;
	std::list<StatusEffect>* StatusEffects();
	void AbortCurrentJob(bool);
	void AbortJob(boost::weak_ptr<Job>);

	bool Expert() const;
	void Expert(bool);

	bool Dead() const;
	void Kill(std::string deathMessage);
	void PickupItem(boost::weak_ptr<Item>);
	void DropItem(boost::weak_ptr<Item>);
	void Hit(boost::weak_ptr<Entity>, bool careful = false);
	void FireProjectile(boost::weak_ptr<Entity>);
	void Damage(Attack*, boost::weak_ptr<NPC> aggr = boost::weak_ptr<NPC>());
	void CastOffensiveSpell(boost::weak_ptr<Entity>);

	void MemberOf(boost::weak_ptr<Squad>);
	boost::weak_ptr<Squad> MemberOf() const;
	void GetMainHandAttack(Attack&);
	bool WieldingRangedWeapon();
	void FindNewWeapon();
	boost::weak_ptr<Item> Wielding() const;
	boost::weak_ptr<Item> Carrying() const;
	bool HasHands() const;
	bool IsTunneler() const;
	bool IsFlying() const; //Special case for pathing's sake. Equivalent to HasEffect(FLYING) except it's threadsafe
	void FindNewArmor();
	boost::weak_ptr<Item> Wearing() const;
	void DecreaseItemCondition(boost::weak_ptr<Item>);

	int GetHealth() const;
	int GetMaxHealth() const;

	static void LoadPresets(std::string);
	static std::vector<NPCPreset> Presets;
	static std::string NPCTypeToString(NPCType);
	static NPCType StringToNPCType(std::string);
	int GetNPCSymbol() const;

	void InitializeAIFunctions();

	static bool GetSquadJob(boost::shared_ptr<NPC>);
	static bool JobManagerFinder(boost::shared_ptr<NPC>);
	static void PlayerNPCReact(boost::shared_ptr<NPC>);
	static void AnimalReact(boost::shared_ptr<NPC>);
	
	static unsigned int pathingThreadCount;
	static boost::mutex threadCountMutex;

	void AddTrait(Trait);
	void RemoveTrait(Trait);
	bool HasTrait(Trait) const;

	void GoBerserk();
	void ApplyEffects(boost::shared_ptr<Item>);

	void DumpContainer(Coordinate);
	void ValidateCurrentJob();

	virtual int GetHeight() const;
	virtual void SetFaction(int);

	void TransmitEffect(StatusEffect);

	virtual void SetMap(Map* map);
};

BOOST_CLASS_VERSION(NPC, 1)

void tFindPath(TCODPath*, int, int, int, int, NPC*, bool);
