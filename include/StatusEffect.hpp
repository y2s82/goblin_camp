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

#include <boost/format.hpp>
#include <libtcod.hpp>

#include "data/Serialization.hpp"

enum NPCStat {
	MOVESPEED,
	DODGE,
	STRENGTH,
	NPCSIZE,
	STAT_COUNT
};

enum Resistance {
	PHYSICAL_RES,
	MAGIC_RES,
	POISON_RES,
	COLD_RES,
	FIRE_RES,
	DISEASE_RES,
	BLEEDING_RES,
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
	BURNING,
	CRACKEDSKULLEFFECT,
	INVIGORATED,
	DRUNK,
	HEALING,
	HELPLESS,
	HIGHGROUND,
	TRIPPED,
	BRAVE,
	COLLYWOBBLES,
	DROOPS,
	RATTLES,
	CHILLS,
	STATUS_EFFECT_COUNT
};

struct StatusEffect {
private:
	GC_SERIALIZABLE_CLASS
public:
	StatusEffect(StatusEffectType=HUNGER, int graphic = 'Y', TCODColor=TCODColor::pink);
	
	static StatusEffectType StringToStatusEffectType(std::string);
	static std::string StatusEffectTypeToString(StatusEffectType);
	static bool IsApplyableStatusEffect(StatusEffectType);

	int graphic;
	TCODColor color;
	std::string name;
	int type;
	int cooldown;
	int cooldownDefault;
	double statChanges[STAT_COUNT]; //These are percentage values of the original value (100% = no change)
	double resistanceChanges[RES_COUNT]; //These are percentage values of the original value (100% = no change)
	std::pair<int,int> damage; //First - counter, second - damage amount
	int damageType;
	bool visible;
	bool negative; //Is this a negative effect? ie. one the creature wants to get rid of
	int contagionChance; //How contagious (if at all) is this effect?
	Resistance applicableResistance; //What resistance helps resist this effect?
};

BOOST_CLASS_VERSION(StatusEffect, 1)

//Version 1 = v0.2 - contagionChance
