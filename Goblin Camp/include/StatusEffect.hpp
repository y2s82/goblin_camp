#pragma once

#include <libtcod.hpp>
#include <string>

enum StatusEffectType {
	HUNGER = 0,
	THIRST,
	PANIC
};

struct StatusEffect {
	StatusEffect(StatusEffectType);
	int graphic;
	TCODColor color;
	std::string name;
	int type;
	int cooldown;
	int cooldownDefault;
};