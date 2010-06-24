#include "StatusEffect.hpp"
#include "Gcamp.hpp"

StatusEffect::StatusEffect(StatusEffectType type) : id(type) {
	switch (type) {
	case HUNGER:
		graphic = TCOD_CHAR_ARROW_S;
		color = TCODColor::orange;
		cooldown = -1;
		break;

	case THIRST:
		graphic = TCOD_CHAR_ARROW_S;
		color = TCODColor::blue;
		cooldown = -1;
		break;

	case PANIC:
		graphic = '!';
		color = TCODColor::white;
		cooldown = UPDATES_PER_SECOND * 5;
		break;
	}
}