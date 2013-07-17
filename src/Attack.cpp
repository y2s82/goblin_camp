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

#include <boost/algorithm/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include "Attack.hpp"
#include "GCamp.hpp"
#include "Game.hpp"

Attack::Attack() : damageType(DAMAGE_BLUNT),
	damageAmount(TCOD_dice_t()),
	cooldown(0),
	cooldownMax(UPDATES_PER_SECOND),
	statusEffects(std::vector<std::pair<StatusEffectType, int> >()),
	projectile(0),
	magicProjectile(false)
{
	damageAmount.addsub = 1;
	damageAmount.multiplier = 1;
	damageAmount.nb_rolls = 1;
	damageAmount.nb_faces = 1;
}

DamageType Attack::Type() {return damageType;}
void Attack::Type(DamageType value) {damageType = value;}

TCOD_dice_t Attack::Amount() {return damageAmount;}
void Attack::Amount(TCOD_dice_t value) {damageAmount = value;}

int Attack::Cooldown() {return cooldown;}

void Attack::CooldownMax(int value) {cooldownMax = value;}
int Attack::CooldownMax() {return cooldownMax;}

void Attack::Update() {
	if (cooldown > 0) --cooldown;
}

void Attack::ResetCooldown() {cooldown = cooldownMax;}

std::vector<std::pair<StatusEffectType, int> >* Attack::StatusEffects() {return &statusEffects;}

bool Attack::Ranged() {return damageType == DAMAGE_RANGED;}

int Attack::Projectile() {return projectile;}
void Attack::Projectile(int value) {projectile = value;}

DamageType Attack::StringToDamageType(std::string type) {
	if (boost::iequals(type, "slashing")) {
		return DAMAGE_SLASH;
	} else if (boost::iequals(type, "piercing")) {
		return DAMAGE_PIERCE;
	} else if (boost::iequals(type, "blunt")) {
		return DAMAGE_BLUNT;
	} else if (boost::iequals(type, "magic")) {
		return DAMAGE_MAGIC;
	} else if (boost::iequals(type, "fire")) {
		return DAMAGE_FIRE;
	} else if (boost::iequals(type, "cold")) {
		return DAMAGE_COLD;
	} else if (boost::iequals(type, "poison")) {
		return DAMAGE_POISON;
	} else if (boost::iequals(type, "wielded")) {
		return DAMAGE_WIELDED;
	} else if (boost::iequals(type, "ranged")) {
		return DAMAGE_RANGED;
	}
	return DAMAGE_SLASH;
}

std::string Attack::DamageTypeToString(DamageType type) {
	//TODO (easy) use switch
	if (type == DAMAGE_SLASH) {
		return "slashing";
	} else if (type == DAMAGE_PIERCE) {
		return "piercing";
	} else if (type == DAMAGE_BLUNT) {
		return "blunt";
	} else if (type == DAMAGE_MAGIC) {
		return "magic";
	} else if (type == DAMAGE_FIRE) {
		return "fire";
	} else if (type == DAMAGE_COLD) {
		return "cold";
	} else if (type == DAMAGE_POISON) {
		return "poison";
	} else if (type == DAMAGE_WIELDED) {
		return "wielded";
	} else if (type == DAMAGE_RANGED) {
		return "ranged";
	}
	return "";
}

void Attack::AddDamage(TCOD_dice_t value) {
	damageAmount.addsub += Game::DiceToInt(value);
}

void Attack::SetMagicProjectile() { magicProjectile = true; }
bool Attack::IsProjectileMagic() { return magicProjectile; }

void Attack::save(OutputArchive& ar, const unsigned int version) const {
	ar & damageType;
	ar & damageAmount.addsub;
	ar & damageAmount.multiplier;
	ar & damageAmount.nb_rolls;
	ar & damageAmount.nb_faces;
	ar & cooldown;
	ar & cooldownMax;
	ar & statusEffects;
	ar & projectile;
	ar & magicProjectile;
}

void Attack::load(InputArchive& ar, const unsigned int version) {
	ar & damageType;
	ar & damageAmount.addsub;
	ar & damageAmount.multiplier;
	ar & damageAmount.nb_rolls;
	ar & damageAmount.nb_faces;
	ar & cooldown;
	ar & cooldownMax;
	ar & statusEffects;
	ar & projectile;
	ar & magicProjectile;
}
