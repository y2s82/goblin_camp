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

#include "Squad.hpp"
#include "Game.hpp"

Squad::Squad(std::string nameValue, int memberValue, int pri) :
name(nameValue),
	memberReq(memberValue),
	order(NOORDER),
	targetCoordinate(Coordinate(-1,-1)),
	targetEntity(boost::weak_ptr<Entity>()),
	priority(pri)
{}

Squad::~Squad() {
#ifdef DEBUG
	std::cout<<"Squad "<<name<<" destructed.\n";
#endif
}

bool Squad::UpdateMembers() {
	if ((signed int)members.size() < memberReq) {
		int newMember = Game::Inst()->FindMilitaryRecruit();
		if (newMember >= 0) { 
			members.push_back(newMember);
			Game::Inst()->npcList[newMember]->MemberOf(shared_from_this());
		}
	} else if ((signed int)members.size() > memberReq) {
		Game::Inst()->npcList[members.back()]->MemberOf(boost::weak_ptr<Squad>());
		members.pop_back();
	}

	if ((signed int)members.size() < memberReq) return true;
	return false;
}

Orders Squad::Order() {return order;}

//Setting an order resets the target of the order
void Squad::Order(Orders newOrder) {
	targetCoordinate = Coordinate(-1,-1);
	targetEntity = boost::weak_ptr<Entity>();
	order = newOrder;
}

Coordinate Squad::TargetCoordinate() {return targetCoordinate;}
void Squad::TargetCoordinate(Coordinate newTarget) {targetCoordinate = newTarget;}

boost::weak_ptr<Entity> Squad::TargetEntity() {return targetEntity;}
void Squad::TargetEntity(boost::weak_ptr<Entity> newEntity) {targetEntity = newEntity;}

int Squad::MemberCount() { return members.size(); }
int Squad::MemberLimit() { return memberReq; }
void Squad::MemberLimit(int val) { memberReq = val; }
std::string Squad::Name() { return name; }
void Squad::Name(std::string value) { name = value; }

void Squad::Leave(int member) {
	for (std::list<int>::iterator membi = members.begin(); membi != members.end(); ++membi) {
		if (*membi == member) {
			members.erase(membi);
			break;
		}
	}
}

void Squad::Priority(int value) { priority = value; }
int Squad::Priority() { return priority; }

void Squad::RemoveAllMembers() {
	for (std::list<int>::iterator membi = members.begin(); membi != members.end(); ++membi) {
		Game::Inst()->npcList[*membi]->MemberOf(boost::weak_ptr<Squad>());
	}
	members.clear();
}

