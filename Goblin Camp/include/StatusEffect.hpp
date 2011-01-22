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

#include <boost/serialization/split_member.hpp>
#include <boost/format.hpp>
#include <libtcod.hpp>

enum NPCStat {
	MOVESPEED,
	DODGE,
	STRENGTH,
	SIZE,
	STAT_COUNT
};

enum Resistance {
	PHYSICAL_RES,
	MAGIC_RES,
	POISON_RES,
	COLD_RES,
	FIRE_RES,
	RES_COUNT
};

enum StatusEffectType {
	HUNGER = 0,
	THIRST,
	PANIC,
	CONCUSSION,
	DROWSY,
	SLEEPING,
	POISON,
	BLEEDING,
	FLYING,
	BADSLEEP,
	RAGE,
	SWIM,
	EATING,
	DRINKING,
	CARRYING,
	WORKING,
	STATUS_EFFECT_COUNT
};

struct StatusEffect {
	friend class boost::serialization::access;
	StatusEffect(StatusEffectType=HUNGER, int graphic = 'Y', TCODColor=TCODColor::pink);

	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	static StatusEffectType StringToStatusEffectType(std::string);
	static std::string StatusEffectTypeToString(StatusEffectType);

	int graphic;
	TCODColor color;
	std::string name;
	int type;
	int cooldown;
	int cooldownDefault;
	double statChanges[STAT_COUNT]; //These are percentage values of the original value (100% = no change)
	double resistanceChanges[RES_COUNT]; //These are percentage values of the original value (100% = no change)
	std::pair<int,int> damage; //First - counter, second - damage amount
	bool bleed;
};
