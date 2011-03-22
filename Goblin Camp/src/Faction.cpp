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
#include <boost/serialization/map.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Faction.hpp"
#include "NPC.hpp"
#include "Game.hpp"
#include "Camp.hpp"
#include "Random.hpp"
#include "Announce.hpp"

std::map<std::string, int> Faction::factionNames = std::map<std::string, int>();
std::vector<boost::shared_ptr<Faction> > Faction::factions = std::vector<boost::shared_ptr<Faction> >();

Faction::Faction(std::string vname) : 
members(std::list<boost::weak_ptr<NPC> >()), 
	trapVisible(std::map<Coordinate,bool>()),
	name(vname),
	currentGoal(0),
	activeTime(0),
	maxActiveTime(MONTH_LENGTH),
	active(false)
{
	//Hardcoding these for now, they will be moved to a data file eventually
	if (boost::iequals(name, "Wolves")) {
		goals.push_back(FACTIONSTEAL);
	} else if (boost::iequals(name, "Giants")) {
		goals.push_back(FACTIONDESTROY);
	} else if (boost::iequals(name, "Bees")) {
		goals.push_back(FACTIONKILL);
	} else if (boost::iequals(name, "Trolls")) {
		goals.push_back(FACTIONPATROL);
		friends.insert(PLAYERFACTION);
	} else if (boost::iequals(name, "Peaceful animal")) {
		goals.push_back(FACTIONIDLE);
	} else if (boost::iequals(name, "Hostile monster")) {
		goals.push_back(FACTIONKILL);
	}
}

void Faction::AddMember(boost::weak_ptr<NPC> newMember) {
	members.push_back(newMember);
}

void Faction::RemoveMember(boost::weak_ptr<NPC> member) {
	for (std::list<boost::weak_ptr<NPC> >::iterator membi = members.begin(); membi != members.end();) {
		if (membi->lock()) {
			if (member.lock() && member.lock() == membi->lock()) {
				members.erase(membi);
				return;
			}
			++membi;
		} else membi = members.erase(membi);
	}
}

void Faction::TrapDiscovered(Coordinate trapLocation) {
	boost::unique_lock<boost::shared_mutex> writeLock(trapVisibleMutex);
	trapVisible[trapLocation] = true;
}

bool Faction::IsTrapVisible(Coordinate trapLocation) {
	boost::shared_lock<boost::shared_mutex> readLock(trapVisibleMutex);
	std::map<Coordinate, bool>::iterator trapi = trapVisible.find(trapLocation);
	if (trapi == trapVisible.end()) return false;
	return trapi->second;
}

void Faction::TrapSet(Coordinate trapLocation, bool visible) {
	boost::unique_lock<boost::shared_mutex> writeLock(trapVisibleMutex);
	trapVisible[trapLocation] = visible;
}

FactionType Faction::StringToFactionType(std::string name) {
	if (!boost::iequals(name, "Faction name not found")) {
		if (factionNames.find(name) == factionNames.end()) {
			factions.push_back(boost::shared_ptr<Faction>(new Faction(name)));
			factionNames[name] = factions.size() - 1;
			factions.back()->MakeFriendsWith(factions.size()-1); //A faction is always friendly with itself
		}
		return factionNames[name];
	}
	return -1;
}

std::string Faction::FactionTypeToString(FactionType faction) {
	if (faction >= 0 && faction < factions.size()) {
		return factions[faction]->name;
	}
	return "Faction name not found";
}

//Reset() does not erase names or goals because these are defined at startup and
//remain constant
void Faction::Reset() {
	boost::unique_lock<boost::shared_mutex> writeLock(trapVisibleMutex);
	members.clear();
	membersAsUids.clear();
	trapVisible.clear();
	jobs.clear();
	currentGoal = 0;
	activeTime = 0;
	maxActiveTime = MONTH_LENGTH;
	active = false;
}

void Faction::Update() {};

namespace {
	inline bool GenerateDestroyJob(boost::shared_ptr<Job> job, boost::shared_ptr<NPC> npc) {
		boost::shared_ptr<Construction> construction;
		TCODLine::init(npc->Position().X(), npc->Position().Y(), Camp::Inst()->Center().X(), Camp::Inst()->Center().Y());
		int x = npc->Position().X();
		int y = npc->Position().Y();
		do {
			int constructionID = Map::Inst()->GetConstruction(x,y);
			if (constructionID >= 0) {
				construction = Game::Inst()->GetConstruction(constructionID).lock();
				if (construction && !construction->HasTag(WORKSHOP) && !construction->HasTag(WALL))
					construction.reset();
			}
		} while (!TCODLine::step(&x, &y) && !construction);
		if (construction) {
			job->tasks.push_back(Task(MOVEADJACENT, construction->Position(), construction));
			job->tasks.push_back(Task(KILL, construction->Position(), construction));
			job->internal = true;
			return true;
		}
		return false;
	}

	inline bool GenerateKillJob(boost::shared_ptr<Job> job) {
		job->internal = true;
		job->tasks.push_back(Task(MOVENEAR, Camp::Inst()->Center()));
		return true;
	}

	inline bool GenerateStealJob(boost::shared_ptr<Job> job, boost::shared_ptr<Item> item) {
		job->internal = true;
		if (item) {
			job->tasks.push_back(Task(MOVE, item->Position()));
			job->tasks.push_back(Task(TAKE, item->Position(), item));
		}
		job->tasks.push_back(Task(FLEEMAP));
		return true;
	}
}

bool Faction::FindJob(boost::shared_ptr<NPC> npc) {
	if (!goals.empty()) {
		if (currentGoal < 0 || currentGoal >= goals.size()) currentGoal = 0;
		switch (goals[currentGoal]) {
		case FACTIONDESTROY: 
			{
				boost::shared_ptr<Job> destroyJob(new Job("Destroy building"));
				if (GenerateDestroyJob(destroyJob, npc) || GenerateKillJob(destroyJob)) {
					npc->StartJob(destroyJob);
					return true;
				}
			}
			break;

		case FACTIONKILL:
			{
				boost::shared_ptr<Job> attackJob(new Job("Attack settlement"));
				if (GenerateKillJob(attackJob)) {
					npc->StartJob(attackJob);
					return true;
				}
			}
			break;

		case FACTIONSTEAL:
			{
				boost::shared_ptr<Job> stealJob(new Job("Steal food"));
				boost::weak_ptr<Item> food = Game::Inst()->FindItemByCategoryFromStockpiles(Item::StringToItemCategory("Food"), npc->Position());
				if (GenerateStealJob(stealJob, food.lock())) {
					npc->StartJob(stealJob);
					return true;
				}
			}
			break;

		case FACTIONPATROL:
			{
				boost::shared_ptr<Job> patrolJob(new Job("Patrol"));
				patrolJob->internal = true;
				int limit = 0;
				Coordinate location(-1,-1);
				if (IsFriendsWith(PLAYERFACTION)) {
					do {
						int x = Random::Generate(Camp::Inst()->GetUprTerritoryCorner().X(),
							Camp::Inst()->GetLowTerritoryCorner().X());
						int y = Random::Generate(Camp::Inst()->GetUprTerritoryCorner().Y(),
							Camp::Inst()->GetLowTerritoryCorner().Y());
						if (Map::Inst()->IsTerritory(x,y)) location = Coordinate(x,y);
						++limit;
					} while (location.X() < 0 && limit < 100);
				} else {
					do {
						int x = Random::Generate(Map::Inst()->Width());
						int y = Random::Generate(Map::Inst()->Height());
						if (!Map::Inst()->IsTerritory(x,y)) location = Coordinate(x,y);
						++limit;
					} while (location.X() < 0 && limit < 100);
				}
				if (location.X() >= 0 && location.Y() >= 0) {
					patrolJob->tasks.push_back(Task(MOVENEAR, location));
					npc->StartJob(patrolJob);
					return true;
				}
			}
			break;

		case FACTIONIDLE:
			{
			}
			break;

		default:
			break;
		}
	}
	return false;
}

void Faction::CancelJob(boost::weak_ptr<Job> oldJob, std::string msg, TaskResult result) {}

void Faction::MakeFriendsWith(FactionType otherFaction) {
	friends.insert(otherFaction);
}

bool Faction::IsFriendsWith(FactionType otherFaction) {
	return friends.find(otherFaction) != friends.end();
}

void Faction::InitAfterLoad() {
	factionNames.clear();
	for (int i = 0; i < factions.size(); ++i) {
		factionNames.insert(std::make_pair(factions[i]->name, i));
	}
	for (int i = 0; i < factions.size(); ++i) {
		factions[i]->MakeFriendsWith(i);
		factions[i]->TranslateFriends();
	}
}

void Faction::TranslateFriends() {
	for (std::list<std::string>::iterator namei = friendNames.begin(); namei != friendNames.end(); ++namei) {
		friends.insert(Faction::StringToFactionType(*namei));
	}
}

void Faction::TranslateMembers() {
	for (int i = 0; i < factions.size(); ++i) {
		for (std::list<int>::iterator uidi = factions[i]->membersAsUids.begin(); 
			uidi != factions[i]->membersAsUids.end(); ++uidi) {
				boost::weak_ptr<NPC> npc = Game::Inst()->GetNPC(*uidi);
				if (npc.lock()) factions[i]->AddMember(npc);
		}
	}
}

void Faction::TransferTrapInfo(boost::shared_ptr<Faction> otherFaction) {
	otherFaction->trapVisible = this->trapVisible;
}

void Faction::save(OutputArchive& ar, const unsigned int version) const {
	std::list< boost::weak_ptr<NPC> > unusedList;
	ar & unusedList;
	ar & trapVisible;
	ar & name;
	ar & jobs;
	ar & goals;
	ar & currentGoal;
	ar & activeTime;
	ar & maxActiveTime;
	ar & active;
	
	std::size_t friendCount = friends.size();
	ar & friendCount;
	for (std::set<FactionType>::iterator factionIter = friends.begin(); factionIter != friends.end(); ++factionIter) {
		std::string factionName = Faction::FactionTypeToString(*factionIter);
		ar & factionName;
	}

	std::size_t memberCount = members.size();
	ar & memberCount;
	for (std::list<boost::weak_ptr<NPC> >::const_iterator membi = members.begin(); membi != members.end(); ++membi) {
		int uid = -1;
		if (membi->lock()) {
			uid = membi->lock()->Uid();
		}
		ar & uid;
	}
}

void Faction::load(InputArchive& ar, const unsigned int version) {
	std::list< boost::weak_ptr<NPC> > unusedList;
	ar & unusedList;
	ar & trapVisible;
	ar & name;
	if (version >= 1) {
		ar & jobs;
		ar & goals;
		ar & currentGoal;
		ar & activeTime;
		ar & maxActiveTime;
		ar & active;
		std::size_t friendCount;
		ar & friendCount;
		for (std::size_t i = 0; i < friendCount; ++i) {
			std::string factionName;
			ar & factionName;
			friendNames.push_back(factionName);
		}

		std::size_t memberCount;
		ar & memberCount;
		for (std::size_t i = 0; i < memberCount; ++i) {
			int uid;
			ar & uid;
			membersAsUids.push_back(uid);
		}
	}
}
