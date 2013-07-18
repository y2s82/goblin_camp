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

#ifdef DEBUG
#include <iostream>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "NatureObject.hpp"
#include "Map.hpp"
#include "Item.hpp"
#include "Game.hpp"
#include "Random.hpp"

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
	minHeight(0.0f),
	maxHeight(2.0f),
	evil(false),
	fallbackGraphicsSet(),
	graphicsHint(-1)
{}

std::vector<NatureObjectPreset> NatureObject::Presets = std::vector<NatureObjectPreset>();

NatureObject::NatureObject(Coordinate pos, NatureObjectType typeVal) : Entity(),
	type(typeVal),
	marked(false),
	ice(false)
{
	Position(pos);

	name = Presets[type].name;
	graphic = Presets[type].graphic;
	color = Presets[type].color;
	condition = Presets[type].condition;
	tree = Presets[type].tree;
	harvestable = Presets[type].harvestable;
}

NatureObject::~NatureObject() {
	if (!NatureObject::Presets[type].walkable) {
		Map::Inst()->SetWalkable(pos, true);
		Map::Inst()->SetBlocksLight(pos, false);
	}
	Map::Inst()->SetBuildable(pos, true);
}

int NatureObject::GetGraphicsHint() const {
	return Presets[type].graphicsHint;
}

void NatureObject::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = (pos - upleft).X();
	int screeny = (pos - upleft).Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, marked ? TCODColor::white : TCODColor::black);
	}
}

void NatureObject::Update() {}

class NatureObjectListener : public ITCODParserListener {
	int natureIndex;

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		bool foundInPreset = false;

		for (size_t i = 0; i < NatureObject::Presets.size(); ++i) {
			if (boost::iequals(NatureObject::Presets[i].name, name)) {
				natureIndex = static_cast<int>(i);
				NatureObject::Presets[i] = NatureObjectPreset();
				foundInPreset = true;
				break;
			}
		}

		if (!foundInPreset) {
			natureIndex = static_cast<int>(NatureObject::Presets.size());
			NatureObject::Presets.push_back(NatureObjectPreset());
		}

		NatureObject::Presets[natureIndex].name = name;
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
		if (boost::iequals(name, "walkable")) {
			NatureObject::Presets[natureIndex].walkable = true;
		} else if (boost::iequals(name, "harvestable")) {
			NatureObject::Presets[natureIndex].harvestable = true;
		} else if (boost::iequals(name, "tree")) {
			NatureObject::Presets[natureIndex].tree = true;
		} else if (boost::iequals(name, "evil")) {
			NatureObject::Presets[natureIndex].evil = true;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
		if (boost::iequals(name, "graphic")) {
			NatureObject::Presets[natureIndex].graphic = value.i;
		} else if (boost::iequals(name, "components")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NatureObject::Presets[natureIndex].components.push_back(Item::StringToItemType((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "col")) {
			NatureObject::Presets[natureIndex].color = value.col;
		} else if (boost::iequals(name, "rarity")) {
			NatureObject::Presets[natureIndex].rarity = value.i;
		} else if (boost::iequals(name, "cluster")) {
			NatureObject::Presets[natureIndex].cluster = value.i;
		} else if (boost::iequals(name, "condition")) {
			NatureObject::Presets[natureIndex].condition = value.i;
		} else if (boost::iequals(name, "minheight")) {
			NatureObject::Presets[natureIndex].minHeight = value.f;
		} else if (boost::iequals(name, "maxheight")) {
			NatureObject::Presets[natureIndex].maxHeight = value.f;
		} else if (boost::iequals(name, "fallbackGraphicsSet")) {
			NatureObject::Presets[natureIndex].fallbackGraphicsSet = value.s;
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

void NatureObject::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* natureObjectTypeStruct = parser.newStructure("plant_type");
	natureObjectTypeStruct->addProperty("graphic", TCOD_TYPE_INT, true);
	natureObjectTypeStruct->addProperty("col", TCOD_TYPE_COLOR, true);
	natureObjectTypeStruct->addListProperty("components", TCOD_TYPE_STRING, false);
	natureObjectTypeStruct->addProperty("rarity", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("condition", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("cluster", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addFlag("tree");
	natureObjectTypeStruct->addFlag("harvestable");
	natureObjectTypeStruct->addFlag("walkable");
	natureObjectTypeStruct->addProperty("minheight", TCOD_TYPE_FLOAT, false);
	natureObjectTypeStruct->addProperty("maxheight", TCOD_TYPE_FLOAT, false);
	natureObjectTypeStruct->addProperty("fallbackGraphicsSet", TCOD_TYPE_STRING, false);
	natureObjectTypeStruct->addFlag("evil");

	NatureObjectListener listener = NatureObjectListener();
	parser.run(filename.c_str(), &listener);
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

bool NatureObject::IsIce() { return ice; }

void NatureObject::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & NatureObject::Presets[type].name;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
	ar & ice;
}

void NatureObject::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	std::string typeName;
	ar & typeName;
	bool failedToFindType = true;
	type = 0; //Default to whatever is the first wildplant
	for (size_t i = 0; i < NatureObject::Presets.size(); ++i) {
		if (boost::iequals(NatureObject::Presets[i].name, typeName)) {
			type = static_cast<int>(i);
			failedToFindType = false;
			break;
		}
	}
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & marked;
	ar & condition;
	ar & tree;
	ar & harvestable;
	if (failedToFindType) harvestable = true;
	if (version >= 1) {
		ar & ice;
	}
}

Ice::Ice(Coordinate pos, NatureObjectType typeVal) : NatureObject(pos, typeVal) {
	ice = true;
	Map::Inst()->SetBlocksWater(pos,true);
	boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(pos).lock();
	if (water) {
		frozenWater = water;
		Game::Inst()->RemoveWater(pos);
	}
}

Ice::~Ice() {
	Map::Inst()->SetBlocksWater(pos,false);
	if (frozenWater) {
		Game::Inst()->CreateWaterFromNode(frozenWater);
		if (Random::Generate(4) == 0) {
			Game::Inst()->CreateItem(Position(), Item::StringToItemType("ice"), false, -1);
		}
	}
}

void Ice::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<NatureObject>(*this);
	ar & frozenWater;
}

void Ice::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<NatureObject>(*this);
	ar & frozenWater;
}
