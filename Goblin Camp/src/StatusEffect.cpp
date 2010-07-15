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

#include "StatusEffect.hpp"
#include "GCamp.hpp"

StatusEffect::StatusEffect(StatusEffectType typeval) : 
type(typeval)
{
	//Initialize stat changes to nothing, ie. 100%
	for (int i = 0; i < STAT_COUNT; ++i) { statChanges[i] = 1.0; }

	switch (type) {
	case HUNGER:
		name = "Hungry";
		graphic = TCOD_CHAR_ARROW_S;
		color = TCODColor::orange;
		cooldown = -1;
		break;

	case THIRST:
		name = "Thirsty";
		graphic = TCOD_CHAR_ARROW_S;
		color = TCODColor::blue;
		cooldown = -1;
		break;

	case PANIC:
		name = "Panicking";
		graphic = '!';
		color = TCODColor::white;
		cooldown = UPDATES_PER_SECOND * 5;
		break;

	case CONCUSSION:
		name = "Concussed";
		graphic = '?';
		color = TCODColor::grey;
		cooldown = UPDATES_PER_SECOND * 5;
		statChanges[MOVESPEED] = 0.5;
		statChanges[ATTACKSKILL] = 0.5;
		statChanges[DEFENCESKILL] = 0.5;
		break;

	case DROWSY:
		name = "Drowsy";
		graphic = 'z';
		color = TCODColor::lightGrey;
		cooldown = -1;
		statChanges[MOVESPEED] = 0.8;
		statChanges[ATTACKSKILL] = 0.8;
		statChanges[ATTACKPOWER] = 0.8;
		statChanges[DEFENCESKILL] = 0.8;
		break;

	case SLEEPING:
		name = "Sleeping";
		graphic = 'Z';
		color = TCODColor::lightGrey;
		cooldown = UPDATES_PER_SECOND;
		break;
	}
	cooldownDefault = cooldown;
}
