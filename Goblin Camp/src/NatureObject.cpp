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
	minHeight(0.0f),
	maxHeight(2.0f),
	evil(false)
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

NatureObject::~NatureObject() {
	Map::Inst()->SetWalkable(x, y, true);
	Map::Inst()->Buildable(x, y, true);
}

void NatureObject::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, marked ? TCODColor::white : TCODColor::black);
	}
}

void NatureObject::Update() {}

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
