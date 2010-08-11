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

#include <libtcod.hpp>

#include "UI/Tooltip.hpp"

Tooltip* Tooltip::instance = 0;
Tooltip* Tooltip::Inst() {
	if(!instance) {
		instance = new Tooltip();
	}
	return instance;
}

void Tooltip::Clear() {
	entries.clear();
}

void Tooltip::AddEntry(TooltipEntry entry) {
	entries.push_back(entry);
}

void Tooltip::Draw(int x, int y, TCODConsole *console) {
	console->setBackgroundColor(TCODColor::black);
	int width = 0;
	for(std::vector<TooltipEntry>::iterator it = entries.begin(); it != entries.end(); it++) {
		width = std::max(width, (int)it->text.length());
	}
	x = std::min(std::min(console->getWidth() - 1, x + 1), console->getWidth() - width);
	y = std::min(std::max(0, y - 1), std::max(0, console->getHeight() - (int)entries.size()));
	if(width > 0) {
		for(std::vector<TooltipEntry>::iterator it = entries.begin(); it != entries.end() && y < console->getHeight(); it++) {
			TooltipEntry entry = *it;
			console->setForegroundColor(entry.color);
			console->printEx(x, y, TCOD_BKGND_ALPHA(0.8), TCOD_LEFT, entry.text.c_str());
			y++;
		}
	}
}
