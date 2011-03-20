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
	if (factionNames.find(name) == factionNames.end()) {
		factions.push_back(boost::shared_ptr<Faction>(new Faction(name)));
		factionNames[name] = factions.size() - 1;
		factions.back()->MakeFriendsWith(factions.size()-1); //A faction is always friendly with itself
	}
	return factionNames[name];
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
	trapVisible.clear();
	jobs.clear();
	currentGoal = 0;
	activeTime = 0;
	maxActiveTime = MONTH_LENGTH;
	active = false;
}

void Faction::Update() {};

bool Faction::FindJob(boost::shared_ptr<NPC> npc) {
	return false;
}

void Faction::CancelJob(boost::weak_ptr<Job> oldJob, std::string msg, TaskResult result) {}

void Faction::MakeFriendsWith(FactionType otherFaction) {
	friends.insert(otherFaction);
}

bool Faction::IsFriendsWith(FactionType otherFaction) {
	return friends.find(otherFaction) != friends.end();
}

void Faction::Init() {
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

void Faction::save(OutputArchive& ar, const unsigned int version) const {
	ar & members;
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
}

void Faction::load(InputArchive& ar, const unsigned int version) {
	ar & members;
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
		for (int i = 0; i < friendCount; ++i) {
			std::string factionName;
			ar & factionName;
			friendNames.push_back(factionName);
		}
	}
}
