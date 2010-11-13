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

#include "scripting/_python.hpp"

#include <libtcod.hpp>
#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

#include "scripting/Engine.hpp"
#include "Logger.hpp"
#include "Game.hpp"

// +-------[ DEV CONSOLE ]--------------------------+
// | output                                       ^ |
// | output                                         |
// | ...                                          v |
// +------------------------------------------------+
// | >>> input                                      |
// +------------------------------------------------+

unsigned Render(unsigned& inID, std::string& input, const std::string& output, TCODConsole& canvas, bool error) {
	boost::tokenizer< boost::char_separator<char> > tok(output, boost::char_separator<char>("\n"));
	
	canvas.clear();
	canvas.setAlignment(TCOD_LEFT);
	canvas.setDefaultBackground(TCODColor::black);
	canvas.setDefaultForeground(TCODColor::white);
	
	canvas.print(0, 0, "[In  %d]: %s", inID, input.c_str());
	canvas.print(0, 1, "[Out %d]", inID);
	canvas.setDefaultForeground(error ? TCODColor::amber : TCODColor::chartreuse);
	
	unsigned y = 3;
	BOOST_FOREACH(std::string token, tok) {
		canvas.print(0, y, "%s", token.c_str());
		++y;
	}
	
	++inID;
	input.clear();
	return y;
}

unsigned Eval(unsigned& inID, std::string& input, TCODConsole& canvas) {
	bool error = false;
	std::string output;
	output.reserve(1024);
	
	try {
		py::object ns  = py::import("__gcdevconsole__").attr("__dict__");
		py::object ret = py::eval(input.c_str(), ns, ns);
		
		py::object retRepr = py::object(py::handle<>(PyObject_Repr(ret.ptr())));
		output = py::extract<char*>(retRepr);
	} catch (const py::error_already_set&) {
		py::object excType, excVal, excTB;
		Script::ExtractException(excType, excVal, excTB);
		Script::LogException();
		
		error = true;
		if (!excType.is_none()) {
			output = py::extract<char*>(py::str(excType));
			if (!excVal.is_none()) {
				output += std::string(": ") + std::string(py::extract<char*>(py::str(excVal)));
			}
		} else {
			output = "Internal error: exception with None type.";
		}
	}
	
	return Render(inID, input, output, canvas, error);
}

void ShowDevConsole() {
	int w = Game::Inst()->ScreenWidth() - 4;
	int h = 25;
	int x = 2;
	int y = Game::Inst()->ScreenHeight() - h - 2;
	
	std::string input("");
	TCOD_key_t key;
	TCOD_mouse_t mouse;
	TCODConsole *c = TCODConsole::root;
	
	bool clicked = false;
	int scroll = 0;
	unsigned maxScroll = 0;
	TCODConsole output(w - 2, 200);
	output.clear();
	
	unsigned inID = 0;
	
	// I tried to use the UI code. Really. I can't wrap my head around it.
	while (true) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.vk == TCODK_ESCAPE) {
			return;
		} else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
			maxScroll = Eval(inID, input, output);
		} else if (key.vk == TCODK_BACKSPACE && input.size() > 0) {
			input.erase(input.end() - 1);
		} else if (key.c >= ' ' && key.c <= '~') {
			input.push_back(key.c);
		}
		
		c->setDefaultForeground(TCODColor::white);
		c->setDefaultBackground(TCODColor::black);
		c->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Developer console");
		c->setAlignment(TCOD_LEFT);
		
		TCODConsole::blit(&output, 0, scroll, w - 2, h - 5, c, x + 1, y + 1);
		
		c->putChar(x + w - 2, y + 1,     TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		c->putChar(x + w - 2, y + h - 4, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
		
		for (unsigned i = 1; i < w - 1; ++i) {
			c->putChar(x + i, y + h - 3, TCOD_CHAR_HLINE, TCOD_BKGND_SET);
		}
		
		c->putChar(x,         y + h - 3, TCOD_CHAR_TEEE, TCOD_BKGND_SET);
		c->putChar(x + w - 1, y + h - 3, TCOD_CHAR_TEEW, TCOD_BKGND_SET);
		
		c->print(x + 1, y + h - 2, "[In %d]: %s", inID, input.c_str());
		
		c->flush();
		
		mouse = TCODMouse::getStatus();
		if (mouse.lbutton) {
			clicked = true;
		}
		
		if (clicked && !mouse.lbutton) {
			if (mouse.cx == x + w - 2) {
				if (mouse.cy == y + 1) {
					scroll = std::max(0, scroll - 1);
				} else if (mouse.cy == y + h - 4) {
					scroll = std::min((int)maxScroll - h + 3, scroll + 1);
				}
			}
			clicked = false;
		}
	}
}
