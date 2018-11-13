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

class ScrollPanel: public Drawable {
private:
	int scroll, scrollBar;
	int step;
	bool drawFrame;
	Scrollable *contents;
public:
	ScrollPanel(
		int x, int y, int nwidth, int nheight, Scrollable *ncontents, bool ndrawFrame = true, int nstep = 1
	):
		Drawable(x, y, nwidth, nheight),
		scroll(0), scrollBar(0),
		step(nstep),
		drawFrame(ndrawFrame),
		contents(ncontents) {}
	~ScrollPanel() { delete contents; }
	void Draw(int, int, TCODConsole *);
	MenuResult Update(int, int, bool, TCOD_key_t);
	void GetTooltip(int, int, Tooltip *);
};
