#pragma once

#include <libtcod.hpp>
#include <string>

enum NPCStat {
	ATTACKSKILL,
	ATTACKPOWER,
	ATTACKSPEED,
	DEFENCESKILL,
	STAT_COUNT
};

enum StatusEffectType {
	HUNGER = 0,
	THIRST,
	PANIC,
	CONCUSSION
};

struct StatusEffect {
	StatusEffect(StatusEffectType);
	int graphic;
	TCODColor color;
	std::string name;
	int type;
	int cooldown;
	int cooldownDefault;
	double statChanges[STAT_COUNT]; //These are percentage values of the original value (100% = no change)
};