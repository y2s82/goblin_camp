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
#include "stdafx.hpp"

#ifdef DEBUG
#include <iostream>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "NatureObject.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "StockManager.hpp"

NatureObjectPreset::NatureObjectPreset() :
	name("NATUREOBJECT PRESET"),
	graphic('?'),
	color(TCODColor::pink),
	components(std::list<ItemType>()),
	rarity(100),
	cluster(1),
	condition(1),
	tree(false),
	harvestable(false),
	walkable(false),
	minHeight(0.04f),
	maxHeight(0.8f)
{}

std::vector<NatureObjectPreset> NatureObject::Presets = std::vector<NatureObjectPreset>();

NatureObject::NatureObject(Coordinate pos, NatureObjectType typeVal) : Entity(),
	type(typeVal),
	marked(false)
{
	Position(pos);

	name = Presets[type].name;
	graphic = Presets[type].graphic;
	color = Presets[type].color;
	condition = Presets[type].condition;
	tree = Presets[type].tree;
	harvestable = Presets[type].harvestable;
}

NatureObject::~NatureObject() {}

void NatureObject::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, marked ? TCODColor::white : TCODColor::black);
	}
}

void NatureObject::Update() {}

class NatureObjectListener : public ITCODParserListener {

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("new %s structure: '%s'\n") % str->getName() % name).str();
#endif
		NatureObject::Presets.push_back(NatureObjectPreset());
		NatureObject::Presets.back().name = name;
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "walkable")) {
			NatureObject::Presets.back().walkable = true;
		} else if (boost::iequals(name, "harvestable")) {
			NatureObject::Presets.back().harvestable = true;
		} else if (boost::iequals(name, "tree")) {
			NatureObject::Presets.back().tree = true;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "graphic")) {
			NatureObject::Presets.back().graphic = value.i;
		} else if (boost::iequals(name, "components")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NatureObject::Presets.back().components.push_back(Item::StringToItemType((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "color")) {
			NatureObject::Presets.back().color = value.col;
		} else if (boost::iequals(name, "rarity")) {
			NatureObject::Presets.back().rarity = value.i;
		} else if (boost::iequals(name, "cluster")) {
			NatureObject::Presets.back().cluster = value.i;
		} else if (boost::iequals(name, "condition")) {
			NatureObject::Presets.back().condition = value.i;
		} else if (boost::iequals(name, "minheight")) {
			NatureObject::Presets.back().minHeight = value.f;
		} else if (boost::iequals(name, "maxheight")) {
			NatureObject::Presets.back().maxHeight = value.f;
		}
		return true;
	}

	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("end of %s structure\n") % name).str();
#endif
		return true;
	}
	void error(const char *msg) {
		LOG("NatureObjectListener: " << msg);
		Game::Inst()->Exit();
	}
};

void NatureObject::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* natureObjectTypeStruct = parser.newStructure("plant_type");
	natureObjectTypeStruct->addProperty("graphic", TCOD_TYPE_INT, true);
	natureObjectTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	natureObjectTypeStruct->addListProperty("components", TCOD_TYPE_STRING, false);
	natureObjectTypeStruct->addProperty("rarity", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("condition", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("cluster", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addFlag("tree");
	natureObjectTypeStruct->addFlag("harvestable");
	natureObjectTypeStruct->addFlag("walkable");
	natureObjectTypeStruct->addProperty("minheight", TCOD_TYPE_FLOAT, false);
	natureObjectTypeStruct->addProperty("maxheight", TCOD_TYPE_FLOAT, false);

	parser.run(filename.c_str(), new NatureObjectListener());
}


void NatureObject::Mark() { marked = true; }
void NatureObject::Unmark() { marked = false; }
bool NatureObject::Marked() { return marked; }

void NatureObject::CancelJob(int) { 
	marked = false; 
}

int NatureObject::Fell() { return --condition; }
int NatureObject::Harvest() { return --condition; }

int NatureObject::Type() { return type; }
bool NatureObject::Tree() { return tree; }
bool NatureObject::Harvestable() { return harvestable; }
