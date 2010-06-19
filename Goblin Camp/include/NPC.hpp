#ifndef NPC_HEADER
#define NPC_HEADER

#include <queue>

#include <boost/thread/thread.hpp>
#include <boost/multi_array.hpp>
#include <boost/function.hpp>

#include <libtcod.hpp>

#include "Coordinate.hpp"
#include "Job.hpp"
#include "GameEntity.hpp"
#include "Container.hpp"

#define LOS_DISTANCE 12
#define MAXIMUM_JOB_ATTEMPTS 5

#define THIRST_THRESHOLD UPDATES_PER_SECOND * 60 * 5
#define HUNGER_THRESHOLD UPDATES_PER_SECOND * 60 * 8
#define DRINKABLE_WATER_DEPTH 2
#define WALKABLE_WATER_DEPTH 1

#define NPC_STATUSES 3

typedef int NPCType;

enum NPCStatus {
	HUNGRY = 0,
	THIRSTY,
	FLEEING
};

enum AiThink {
	AINOTHING,
	AIMOVE
};

enum Skill {
	MASONRY,
	CARPENTTY,
	SKILLAMOUNT
};


class SkillSet {
	private:
		int skills[SKILLAMOUNT];
	public:
		SkillSet();
		int operator()(Skill);
		void operator()(Skill, int);
};

class NPC : public GameEntity {
    friend class Game;

	private:
		NPC(Coordinate,
            boost::function<bool(boost::shared_ptr<NPC>)> findJob,
            boost::function<void(boost::shared_ptr<NPC>)> react);

		bool _visArray[LOS_DISTANCE*2 * LOS_DISTANCE*2];
		int timeCount;
		std::deque<boost::shared_ptr<Job> > jobs;
		int taskIndex;
		boost::try_mutex pathMutex;
		TCODPath *path;
		bool nopath;
		bool findPathWorking;
		int timer;
		unsigned int _speed, nextMove;
		TCODColor _color, _bgcolor;
		int _graphic;
		bool taskBegun;
		bool expert;
		boost::weak_ptr<Item> carried;
		int thirst, hunger;
		int thinkSpeed;
		bool status[NPC_STATUSES];
		int statusGraphicCounter;
		void HandleThirst();
		void HandleHunger();
		int health;
		boost::weak_ptr<Item> foundItem;
        boost::shared_ptr<Container> bag;

        std::list<boost::weak_ptr<NPC> >nearNpcs;

		//bool FindJob();
		boost::function<bool(boost::shared_ptr<NPC>)> FindJob;
		boost::function<void(boost::shared_ptr<NPC>)> React;

	public:
		SkillSet Skills;
		AiThink Think();
		void Draw(Coordinate);
		void Position(Coordinate,bool);
		virtual void Position(Coordinate);
		virtual Coordinate Position();
		void speed(unsigned int);
		unsigned int speed();
		void color(TCODColor,TCODColor=TCODColor::black);
		void graphic(int);

		bool *visArray();

		Task* currentTask();
		boost::weak_ptr<Job> currentJob();
		Coordinate currentTarget();
		boost::weak_ptr<GameEntity> currentEntity();
		void TaskFinished(TaskResult, std::string = "");
		TaskResult Move();
		void findPath(Coordinate);

		void Flee();

		bool Expert();
		void Expert(bool);

		bool Dead();
		void Kill();
		void DropCarriedItem();

		static bool JobManagerFinder(boost::shared_ptr<NPC>);
		static void PlayerNPCReact(boost::shared_ptr<NPC>);
		static void PeacefulAnimalReact(boost::shared_ptr<NPC>);
		static bool PeacefulAnimalFindJob(boost::shared_ptr<NPC>);

		int faction;
};

void tFindPath(TCODPath*, int, int, int, int, boost::try_mutex*, bool*, bool*);

#endif
