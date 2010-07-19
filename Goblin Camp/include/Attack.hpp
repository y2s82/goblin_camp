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

#include <vector>

#include "Item.hpp"
#include "StatusEffect.hpp"

enum DamageType {
	DAMAGE_SLASH,
	DAMAGE_PIERCE,
	DAMAGE_BLUNT,
	DAMAGE_MAGIC,
	DAMAGE_FIRE,
	DAMAGE_COLD,
	DAMAGE_POISON,
	DAMAGE_WIELDED
};

class Attack {
private:
	DamageType damageType;
	int damageAmount;
	int cooldown;
	int cooldownMax;
	std::vector<std::pair<StatusEffect, int> > statusEffects;
	bool ranged;
	ItemType projectile;
public:
	Attack();
	DamageType Type();
	void Type(DamageType);
	int Amount();
	void Amount(int);
	int Cooldown();
	void CooldownMax(int);
	void Update();
	void ResetCooldown();
	std::vector<std::pair<StatusEffect, int> >* StatusEffects();
	bool Ranged();
	void Ranged(bool);
	ItemType Projectile();
	void Projectile(ItemType);
};