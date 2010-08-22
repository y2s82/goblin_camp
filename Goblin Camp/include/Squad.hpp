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

#include <string>
#include <list>

#include <boost/weak_ptr.hpp>
#include <boost/serialization/split_member.hpp>

#include "Coordinate.hpp"
#include "Entity.hpp"
#include "Item.hpp"

enum Orders {
	NOORDER,
	GUARD,
	PATROL,
	ESCORT
};

class Squad : public boost::enable_shared_from_this<Squad> {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	std::string name;
	int memberReq;
	//List of NPC uid's
	std::list<int> members;
	Orders order;
	Coordinate targetCoordinate;
	boost::weak_ptr<Entity> targetEntity;
	int priority;
	ItemCategory weapon;
	ItemCategory armor;
public:
	Squad(std::string name="Noname nancyboys", int members=0, int priority=0);
	~Squad();
	//Recruits one member if needed and returns true if the squad still requires more members
	bool UpdateMembers();
	Orders Order();
	void Order(Orders);
	Coordinate TargetCoordinate();
	void TargetCoordinate(Coordinate);
	boost::weak_ptr<Entity> TargetEntity();
	void TargetEntity(boost::weak_ptr<Entity>);
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
};