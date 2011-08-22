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

#include <string>
#include <list>
#include <vector>

#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "data/Serialization.hpp"

class Coordinate;
class Entity;
typedef int ItemCategory;

enum Order {
	NOORDER,
	GUARD,
	PATROL,
	FOLLOW
};

class Squad : public boost::enable_shared_from_this<Squad> {
	GC_SERIALIZABLE_CLASS
	
	std::string name;
	int memberReq;
	//List of NPC uid's
	std::list<int> members;
	Order generalOrder;
	std::vector<Order> orders;
	std::vector<Coordinate> targetCoordinates;
	std::vector<boost::weak_ptr<Entity> > targetEntities;
	int priority;
	ItemCategory weapon;
	ItemCategory armor;
public:
	Squad(std::string name="Noname nancyboys", int members=0, int priority=0);
	~Squad();
	//Recruits one member if needed and returns true if the squad still requires more members
	bool UpdateMembers();
	Order GetOrder(int &orderIndex);
	Order GetGeneralOrder(); //Used to highlight correct SquadsDialog button
	void AddOrder(Order);
	void ClearOrders();
	Coordinate TargetCoordinate(int orderIndex);
	void AddTargetCoordinate(Coordinate);
	boost::weak_ptr<Entity> TargetEntity(int orderIndex);
	void AddTargetEntity(boost::weak_ptr<Entity>);
	int MemberCount();
	int MemberLimit();
	void MemberLimit(int);
	std::string Name();
	void Name(std::string);
	void Leave(int);
	void Priority(int);
	int Priority();
	void RemoveAllMembers();
	ItemCategory Weapon();
	void Weapon(ItemCategory);
	void Rearm();
	ItemCategory Armor();
	void Armor(ItemCategory);
	void Reequip();
	void SetGeneralOrder(Order);
};

BOOST_CLASS_VERSION(Squad, 0)
