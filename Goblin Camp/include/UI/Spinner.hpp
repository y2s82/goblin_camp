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

class Spinner: public Drawable {
private:
	boost::function<int()> getter;
	boost::function<void(int)> setter;
	int *value;
	int min, max;
public:
	Spinner(
		int x, int y, int nwidth, boost::function<int()> ngetter,
		boost::function<void(int)> nsetter, int nmin = 0, int nmax = std::numeric_limits<int>::max()
	):
		Drawable(x, y, nwidth, 1), getter(ngetter), setter(nsetter), value(0), min(nmin), max(nmax) {}
	Spinner(int x, int y, int nwidth, int *nvalue, int nmin = 0, int nmax = std::numeric_limits<int>::max()):
		Drawable(x, y, nwidth, 1), getter(0), setter(0), value(nvalue), min(nmin), max(nmax) {}
	void Draw(int, int, TCODConsole *);
	MenuResult Update(int, int, bool, TCOD_key_t);
};
