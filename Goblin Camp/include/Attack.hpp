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

#include <vector>
#include <string>

#include <boost/serialization/split_member.hpp>

#include <libtcod.hpp>

#include "StatusEffect.hpp"

enum DamageType {
	DAMAGE_SLASH,
	DAMAGE_PIERCE,
	DAMAGE_BLUNT,
	DAMAGE_MAGIC,
	DAMAGE_FIRE,
	DAMAGE_COLD,
	DAMAGE_POISON,
	DAMAGE_COUNT, //Nothing can deal "wielded" or "ranged" damage
	DAMAGE_WIELDED,
	DAMAGE_RANGED
};

class Attack {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	DamageType damageType;
	TCOD_dice_t damageAmount;
	int cooldown;
	int cooldownMax;
	std::vector<std::pair<StatusEffectType, int> > statusEffects;
	int projectile;
public:
	Attack();

	static DamageType StringToDamageType(std::string);
	static std::string DamageTypeToString(DamageType);

	DamageType Type();
	void Type(DamageType);
	TCOD_dice_t Amount();
	void Amount(TCOD_dice_t);
	void AddDamage(TCOD_dice_t);
	int Cooldown();
	void CooldownMax(int);
	int CooldownMax();
	void Update();
	void ResetCooldown();
	std::vector<std::pair<StatusEffectType, int> >* StatusEffects();
	bool Ranged();
	int Projectile();
	void Projectile(int);
};
