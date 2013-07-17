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

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "UIComponents.hpp"

class Label: public Drawable {
private:
	std::string text;
	TCOD_alignment_t align;
public:
	Label(std::string ntext, int x, int y, TCOD_alignment_t nalign = TCOD_CENTER) :
	Drawable(x, y, 0, 1), text(ntext), align(nalign) {}
	void Draw(int, int, TCODConsole *);
};

class LiveLabel: public Drawable {
private:
	boost::function<std::string()> text;
	TCOD_alignment_t align;
public:
	LiveLabel(boost::function<std::string()> ntext, int x, int y, TCOD_alignment_t nalign = TCOD_CENTER) :
	Drawable(x, y, 0, 1), text(ntext), align(nalign) {}
	void Draw(int, int, TCODConsole *);
};
