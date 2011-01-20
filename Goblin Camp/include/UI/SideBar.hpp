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
#pragma once

#include <libtcod.hpp>
#include <vector>
#include <string>
#include <boost/weak_ptr.hpp>

#include "Entity.hpp"

class SideBar {
	boost::weak_ptr<Entity> entity;
	boost::shared_ptr<Drawable> contents;
	int width, height, topY, leftX;
	bool npc, construction, stockpile, farmplot;
public:
	SideBar();
	void SetEntity(boost::weak_ptr<Entity>);
	MenuResult Update(int, int, bool);
	void Draw(TCODConsole*);
	void GetTooltip(int, int, Tooltip *, TCODConsole *);
	static void DrawStatusEffect(StatusEffect, int, int, int, int, bool, TCODConsole *);
	static void DrawSeed(std::pair<ItemType, bool>, int, int, int, int, bool, TCODConsole *);
	static std::string NPCSquadLabel(NPC *);
	static std::string NPCWeaponLabel(NPC *);
	static std::string NPCArmorLabel(NPC *);
};
