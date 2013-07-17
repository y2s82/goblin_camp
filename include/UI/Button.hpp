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

class Button: public Drawable {
protected:
	std::string text;
	bool selected;
	char shortcut;
	bool dismiss;
	boost::function<void()> callback;
public:
	Button(std::string ntext, boost::function<void()> ncallback, int x, int y, int nwidth, char nshortcut = 0, bool ndismiss = false):
		Drawable(x, y, nwidth, 0),
		text(ntext),
		selected(false),
		shortcut(nshortcut),
		dismiss(ndismiss),
		callback(ncallback) {}
	void Draw(int, int, TCODConsole *);
	MenuResult Update(int, int, bool, TCOD_key_t);
};

class LiveButton: public Button {
private:
	boost::function<std::string()> textFunc;
public:
	LiveButton(boost::function<std::string()> ntextFunc, boost::function<void()> ncallback, int x, int y, int nwidth, char nshortcut = 0):
		Button("", ncallback, x, y, nwidth, nshortcut), textFunc(ntextFunc) {}
	void Draw(int, int, TCODConsole *);
};

class ToggleButton: public Button {
private:
	boost::function<bool()> isOn;
public:
	ToggleButton(std::string ntext, boost::function<void()> ncallback, boost::function<bool()> nisOn, int x, int y, int nwidth, char nshortcut = 0):
		Button(ntext, ncallback, x, y, nwidth, nshortcut), isOn(nisOn) {}
	void Draw(int, int, TCODConsole *);
};
