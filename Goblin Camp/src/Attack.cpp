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

#include "Attack.hpp"
#include "GCamp.hpp"

Attack::Attack() : damageType(DAMAGE_BLUNT),
	damageAmount(1),
	cooldown(0),
	cooldownMax(UPDATES_PER_SECOND),
	statusEffects(std::vector<std::pair<StatusEffect, int> >()),
	ranged(false),
	projectile(0)
{
}

DamageType Attack::Type() {return damageType;}
void Attack::Type(DamageType value) {damageType = value;}

int Attack::Amount() {return damageAmount;}
void Attack::Amount(int value) {damageAmount = value;}

int Attack::Cooldown() {return cooldown;}

void Attack::CooldownMax(int value) {cooldownMax = value;}

void Attack::Update() {
	if (cooldown > 0) --cooldown;
}

void Attack::ResetCooldown() {cooldown = cooldownMax;}

std::vector<std::pair<StatusEffect, int> >* Attack::StatusEffects() {return &statusEffects;}

bool Attack::Ranged() {return ranged;}
void Attack::Ranged(bool value) {ranged = value;}

ItemType Attack::Projectile() {return projectile;}