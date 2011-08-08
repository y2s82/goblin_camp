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

#ifdef DEBUG
#include <iostream>
#endif

#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Squad.hpp"
#include "Game.hpp"

Squad::Squad(std::string nameValue, int memberValue, int pri) :
	name(nameValue),
	memberReq(memberValue),
	generalOrder(NOORDER),
	orders(std::vector<Order>()),
	targetCoordinates(std::vector<Coordinate>()),
	targetEntities(std::vector<boost::weak_ptr<Entity> >()),
	priority(pri),
	weapon(-1),
	armor(-1)
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
			boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(newMember);
			if (npc) {
				members.push_back(newMember);
				npc->MemberOf(shared_from_this());
			}
		}
	} else if ((signed int)members.size() > memberReq) {
		boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(members.back());
		if (npc) npc->MemberOf(boost::weak_ptr<Squad>());
		members.pop_back();
	}

	if ((signed int)members.size() < memberReq) return true;
	return false;
}

Order Squad::GetOrder(int &orderIndex) {
	++orderIndex;
	if (orderIndex < 0 || orderIndex >= static_cast<int>(orders.size()))
		orderIndex = 0;
	return orders.empty() ? NOORDER : orders[orderIndex];
}

//Setting an order resets the target of the order
void Squad::AddOrder(Order newOrder) {
	orders.push_back(newOrder);
	targetCoordinates.push_back(Coordinate(-1,-1));
	targetEntities.push_back(boost::weak_ptr<Entity>());
}

void Squad::ClearOrders() {
	orders.clear();
	targetCoordinates.clear();
	targetEntities.clear();
	generalOrder = NOORDER;
}

Coordinate Squad::TargetCoordinate(int index) {
	if (index >= 0 && index < static_cast<int>(targetCoordinates.size())) {
		return targetCoordinates[index];
	} else return Coordinate(-1,-1);
}
void Squad::AddTargetCoordinate(Coordinate newTarget) {targetCoordinates.back() = newTarget;}

boost::weak_ptr<Entity> Squad::TargetEntity(int index) {
	if (index >= 0 && index < static_cast<int>(targetEntities.size())) {
		return targetEntities[index];
	} else return boost::weak_ptr<Entity>();
}

void Squad::AddTargetEntity(boost::weak_ptr<Entity> newEntity) {targetEntities.back() = newEntity;}

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
		boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(*membi);
		if (npc) npc->MemberOf(boost::weak_ptr<Squad>());
	}
	members.clear();
}

ItemCategory Squad::Weapon() { return weapon; }
void Squad::Weapon(ItemCategory value) { weapon = value; }

void Squad::Rearm() {
	for (std::list<int>::iterator memberi = members.begin(); memberi != members.end(); ++memberi) {
		boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(*memberi);
		if (npc) npc->FindNewWeapon();
	}
}

ItemCategory Squad::Armor() { return armor; }
void Squad::Armor(ItemCategory value) { armor = value; }

void Squad::Reequip() {
	for (std::list<int>::iterator memberi = members.begin(); memberi != members.end(); ++memberi) {
		boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(*memberi);
		if (npc) npc->FindNewArmor();
	}
}

Order Squad::GetGeneralOrder() { 
	return generalOrder;
}

void Squad::SetGeneralOrder(Order order) { generalOrder = order; }

void Squad::save(OutputArchive& ar, const unsigned int version) const {
	ar & name;
	ar & memberReq;
	ar & members;
	ar & generalOrder;
	ar & orders;
	ar & targetCoordinates;
	ar & targetEntities;
	ar & priority;
	ar & weapon;
	ar & armor;
}

void Squad::load(InputArchive& ar, const unsigned int version) {
	ar & name;
	ar & memberReq;
	ar & members;
	ar & generalOrder;
	ar & orders;
	ar & targetCoordinates;
	ar & targetEntities;
	ar & priority;
	ar & weapon;
	ar & armor;
}
