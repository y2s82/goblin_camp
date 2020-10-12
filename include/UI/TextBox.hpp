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

#include <string>
#include <vector>

#include <functional>
#include <functional>

#include <libtcod.hpp>

#include "UIComponents.hpp"

class TextBox: public Drawable {
private:
	std::string *value;
	std::function<std::string()> getter;
	std::function<void(std::string)> setter;
public:
	TextBox(int x, int y, int nwidth, std::string *nvalue):
		Drawable(x, y, nwidth, 1), value(nvalue) {}
	TextBox(int x, int y, int nwidth, std::function<std::string()> ngetter, std::function<void(std::string)> nsetter):
		Drawable(x, y, nwidth, 1), value(0), getter(ngetter), setter(nsetter) {}
	void Draw(int, int, TCODConsole *);
	MenuResult Update(int, int, bool, TCOD_key_t);
};
