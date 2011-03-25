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

#include <list>
#include <map>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/weak_ptr.hpp>

#include "data/Serialization.hpp"
#include "Job.hpp"

class NPC;
class Coordinate;

#define PLAYERFACTION 0

typedef int FactionType;

enum FactionGoal {
	FACTIONDESTROY, //Destroy buildings
	FACTIONKILL,    //Kill hostiles
	FACTIONSTEAL,   //Steal items valuable to the faction
	FACTIONPATROL,  //Patrol area
	FACTIONIDLE     //Idle
};

class Faction {
	GC_SERIALIZABLE_CLASS
	
	friend class FactionListener;

	static std::map<std::string, int> factionNames;
	
	std::list<boost::weak_ptr<NPC> > members;
	std::list<int> membersAsUids;
	std::map<Coordinate, bool> trapVisible;
	std::set<FactionType> friends;
	std::list<std::string> friendNames;
	std::string name;
	int index;

	boost::shared_mutex trapVisibleMutex;

	std::vector<boost::weak_ptr<Job> > jobs;
	std::vector<FactionGoal> goals;
	std::vector<int> goalSpecifiers;
	int currentGoal;
	int activeTime;
	int maxActiveTime;
	bool active;

	bool aggressive, coward;

	void TranslateFriends();
public:
	Faction(std::string = "Noname faction", int = -1);
	void AddMember(boost::weak_ptr<NPC>);
	void RemoveMember(boost::weak_ptr<NPC>);
	void TrapDiscovered(Coordinate, bool propagate=true);
	bool IsTrapVisible(Coordinate);
	void TrapSet(Coordinate, bool visible);

	static FactionType StringToFactionType(std::string);
	static std::string FactionTypeToString(FactionType);
	static std::vector<boost::shared_ptr<Faction> > factions;

	static FactionGoal StringToFactionGoal(std::string);
	static std::string FactionGoalToString(FactionGoal);

	void Reset();
	void Update();
	bool FindJob(boost::shared_ptr<NPC>);
	void CancelJob(boost::weak_ptr<Job>, std::string, TaskResult);

	void MakeFriendsWith(FactionType);
	bool IsFriendsWith(FactionType);
	
	static void InitAfterLoad(); //Initialize faction names, required before loading npcs from a save file
	static void TranslateMembers(); //Translate member uids into pointers _after_ loading npcs from a save
	void TransferTrapInfo(boost::shared_ptr<Faction>); //One way transfer, not used for sharing trap data between friendly factions

	FactionGoal GetCurrentGoal() const;
	static void LoadPresets(std::string);

	bool IsCoward();
	bool IsAggressive();
};

BOOST_CLASS_VERSION(Faction, 1)
