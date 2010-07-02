#include "StatusEffect.hpp"
#include "Gcamp.hpp"

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
		statChanges[ATTACKSKILL] = 0.5;
		break;
	}
	cooldownDefault = cooldown;
}