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
#include <boost/serialization/list.hpp>

#include "Spell.hpp"
#include "Game.hpp"
#include "Random.hpp"

boost::unordered_map<std::string, SpellType> Spell::spellTypeNames = boost::unordered_map<std::string, SpellType>();
std::vector<SpellPreset> Spell::Presets = std::vector<SpellPreset>();

SpellPreset::SpellPreset(std::string vname) : 
	name(vname),
	attacks(std::list<Attack>()),
	immaterial(false),
	graphic('?'),
	speed(1),
	color(TCODColor::pink),
	fallbackGraphicsSet(""),
	graphicsHint(-1)
{}

Spell::Spell(const Coordinate& pos, int vtype) : Entity(),
	type(vtype), dead(false), immaterial(false)
{
	Position(pos);
	color = Spell::Presets[type].color;

	//TODO: I don't like making special cases like this, perhaps a flag instead?
	if (boost::iequals(Spell::Presets[type].name, "smoke") || boost::iequals(Spell::Presets[type].name, "steam")) {
		int add = Random::Generate(50);
		color.r += add;
		color.g += add;
		color.b += add;
	}

	graphic = Spell::Presets[type].graphic;
	attacks = Spell::Presets[type].attacks;
	immaterial = Spell::Presets[type].immaterial;
}

Spell::~Spell() {}

void Spell::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = (pos - upleft).X();
	int screeny = (pos - upleft).Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, Map::Inst()->GetBackColor(pos));
	}
}

int Spell::GetGraphicsHint() const {
	return Spell::Presets[type].graphicsHint;
}

void Spell::Impact(int speedChange) {
	SetVelocity(0);
	flightPath.clear();

	for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
		if (attacki->Type() == DAMAGE_FIRE) {
			Game::Inst()->CreateFire(Position());
			break;
		}
	}

	dead = true;
}

void Spell::UpdateVelocity() {
	if (velocity > 0) {
		nextVelocityMove += velocity;
		while (nextVelocityMove > 100) {
			nextVelocityMove -= 100;
			if (flightPath.size() > 0) {

				if (flightPath.back().height < ENTITYHEIGHT) { //We're flying low enough to hit things
					Coordinate t = flightPath.back().coord;

					if (!immaterial) {
						if (Map::Inst()->BlocksWater(t) || !Map::Inst()->IsWalkable(t)) { //We've hit an obstacle
							if (Map::Inst()->GetConstruction(t) > -1) {
								if (boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(t)).lock()) {
									for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
										construct->Damage(&*attacki);
									}
								}
							}
							for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
								if (attacki->Type() == DAMAGE_FIRE) {
								/*The spell's attack was a fire attack, so theres a chance it'll create fire on the 
								obstacle it hit */
									Game::Inst()->CreateFire(flightPath.back().coord, 5);
									break;
								}
							}
							Impact(velocity);
							return;
						}
						if (Map::Inst()->NPCList(t)->size() > 0) { //Hit a creature
							if (Random::Generate(std::max(1, flightPath.back().height) - 1) < (signed int)(2 + Map::Inst()->NPCList(t)->size())) {

								boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(*Map::Inst()->NPCList(t)->begin());
								for (std::list<Attack>::iterator attacki = attacks.begin(); attacki != attacks.end(); ++attacki) {
									npc->Damage(&*attacki);
								}

								Position(flightPath.back().coord);
								Impact(velocity);
								return;
							}
						}
					}
				}

				Position(flightPath.back().coord);

				if (flightPath.back().height <= 0) { //Hit the ground early
					Impact(velocity);
					return;
				}

				flightPath.pop_back();
			} else { Impact(velocity); return; } //No more flightpath
		}
	} 
}

bool Spell::IsDead() {return dead;}

int Spell::StringToSpellType (std::string spell) {
	if (spellTypeNames.find(spell) != spellTypeNames.end()) return spellTypeNames[spell];
	return -1;
}

std::string Spell::SpellTypeToString(SpellType type) {
	if (type >= 0 && type < static_cast<int>(Presets.size())) return Presets[type].name;
	return "";
}

class SpellListener : public ITCODParserListener {
	int spellIndex;
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if (boost::iequals(str->getName(), "spell_type")) {
			if (Spell::spellTypeNames.find(name) != Spell::spellTypeNames.end()) {
				spellIndex = Spell::spellTypeNames[name];
				Spell::Presets[spellIndex] = SpellPreset(name);
			} else {
				Spell::Presets.push_back(SpellPreset(name));
				Spell::spellTypeNames[name] = Spell::Presets.size()-1;
				spellIndex = Spell::Presets.size() - 1;
			}
		} else if (boost::iequals(str->getName(), "attack")) {
			Spell::Presets[spellIndex].attacks.push_back(Attack());
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
		if (boost::iequals(name,"immaterial")) { Spell::Presets[spellIndex].immaterial = true; }
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
		if (boost::iequals(name,"name")) { Spell::Presets[spellIndex].name = value.s; }
		else if (boost::iequals(name,"speed")) { Spell::Presets[spellIndex].speed = value.i; }
		else if (boost::iequals(name,"col")) { Spell::Presets[spellIndex].color = value.col; }
		else if (boost::iequals(name,"graphic")) { Spell::Presets[spellIndex].graphic = value.i; }
		else if (boost::iequals(name,"fallbackGraphicsSet")) { Spell::Presets[spellIndex].fallbackGraphicsSet = value.s; }
		else if (boost::iequals(name,"type")) {
			Spell::Presets[spellIndex].attacks.back().Type(Attack::StringToDamageType(value.s));
		} else if (boost::iequals(name,"damage")) {
			Spell::Presets[spellIndex].attacks.back().Amount(value.dice);
		} else if (boost::iequals(name,"cooldown")) {
			Spell::Presets[spellIndex].attacks.back().CooldownMax(value.i);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				StatusEffectType type = StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i));
				if (StatusEffect::IsApplyableStatusEffect(type))
					Spell::Presets[spellIndex].attacks.back().StatusEffects()->push_back(std::pair<StatusEffectType, int>(type, 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Spell::Presets[spellIndex].attacks.back().StatusEffects()->at(i).second = (intptr_t)TCOD_list_get(value.list,i);
			}
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		return true;
	}
	void error(const char *msg) {
		throw std::runtime_error(msg);
	}
};

void Spell::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct *spellTypeStruct = parser.newStructure("spell_type");
	spellTypeStruct->addProperty("name", TCOD_TYPE_STRING, true);
	spellTypeStruct->addProperty("col", TCOD_TYPE_COLOR, true);
	spellTypeStruct->addProperty("graphic", TCOD_TYPE_INT, true);
	spellTypeStruct->addFlag("immaterial");
	spellTypeStruct->addProperty("speed", TCOD_TYPE_INT, true);
	spellTypeStruct->addProperty("fallbackGraphicsSet", TCOD_TYPE_STRING, false);
	
	TCODParserStruct *attackTypeStruct = parser.newStructure("attack");
	const char* damageTypes[] = { "slashing", "piercing", "blunt", "magic", "fire", "cold", "poison", "wielded", NULL };
	attackTypeStruct->addValueList("type", damageTypes, true);
	attackTypeStruct->addProperty("damage", TCOD_TYPE_DICE, false);
	attackTypeStruct->addListProperty("statusEffects", TCOD_TYPE_STRING, false);
	attackTypeStruct->addListProperty("effectChances", TCOD_TYPE_INT, false);

	spellTypeStruct->addStructure(attackTypeStruct);

	SpellListener listener = SpellListener();
	parser.run(filename.c_str(), &listener);
}

void Spell::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & graphic;
	std::string spellType(Spell::SpellTypeToString(type));
	ar & spellType;
	ar & dead;
	ar & attacks;
	ar & immaterial;
}

void Spell::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & graphic;
	std::string typeName;
	ar & typeName;
	type = Spell::StringToSpellType(typeName);
	if (type == -1) type = 0;
	ar & dead;
	ar & attacks;
	ar & immaterial;
}
