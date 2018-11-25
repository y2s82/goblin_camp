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

#include <libtcod.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
namespace py = boost::python;

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

#include <cStringIO.h>

struct DevConsole {
	unsigned inputID;
	std::string input;
	std::string output;
	TCODConsole canvas;
	
	PyCompilerFlags cf;
	PyObject *newStdOut, *newStdIn;
	PyObject *oldStdOut, *oldStdErr, *oldStdIn;
	
	DevConsole(unsigned width) : inputID(0), input(""), output(""), canvas(TCODConsole(width - 2, 256)) {
		output.reserve(2048);
		input.reserve(256);
		canvas.clear();
		
		cf.cf_flags = (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_PRINT_FUNCTION);
		
		PycString_IMPORT;
		newStdIn  = PycStringIO->NewInput(PyString_FromString(""));
		
		/* const_cast are workarounds against the fact that PySys_{Get,Set}Object expects
		   a char * instead of const char *
		   see: http://mail.python.org/pipermail/python-dev/2011-February/108140.html
		*/
		oldStdOut = PySys_GetObject(const_cast<char *>("stdout"));
		oldStdErr = PySys_GetObject(const_cast<char *>("stderr"));
		oldStdIn  = PySys_GetObject(const_cast<char *>("stdin"));
	}
	
	std::string GetStreamValue() {
		PyObject *str = PycStringIO->cgetvalue(newStdOut);
		return std::string(py::extract<char*>(py::object(py::handle<>(str))));
	}
	
	void RedirectStreams() {
		newStdOut = PycStringIO->NewOutput(2048);
		
		PySys_SetObject(const_cast<char *>("stdout"), newStdOut);
		PySys_SetObject(const_cast<char *>("stderr"), newStdOut);
		PySys_SetObject(const_cast<char *>("stdin"),  newStdIn);
	}
	
	void RestoreStreams() {
		PySys_SetObject(const_cast<char *>("stdout"), oldStdOut);
		PySys_SetObject(const_cast<char *>("stderr"), oldStdErr);
		PySys_SetObject(const_cast<char *>("stdin"),  oldStdIn);
		
		Py_DECREF(newStdOut);
	}
	
	unsigned Render(bool error) {
		typedef boost::char_separator<char> SepT;
		typedef boost::tokenizer<SepT> TokT;
		
		SepT sep("\n");
		TokT inTok(input, sep);
		TokT outTok(output, sep);
		
		canvas.clear();
		canvas.setAlignment(TCOD_LEFT);
		canvas.setDefaultBackground(TCODColor::black);
		canvas.setDefaultForeground(TCODColor::white);
		
		canvas.print(0, 0, "[In  %d]", inputID);
		canvas.setDefaultForeground(TCODColor::sky);
		
		unsigned y = 1;
		BOOST_FOREACH(std::string token, inTok) {
			canvas.print(0, y, "%s", token.c_str());
			++y;
		}
		
		++y;
		
		canvas.setDefaultForeground(TCODColor::white);
		canvas.print(0, y, "[Out %d]", inputID);
		canvas.setDefaultForeground(error ? TCODColor::amber : TCODColor::chartreuse);
		
		++y;
		BOOST_FOREACH(std::string token, outTok) {
			canvas.print(0, y, "%s", token.c_str());
			++y;
		}
		
		++inputID;
		input.clear();
		return y;
	}
	
	unsigned Eval() {
		bool error = false;
		RedirectStreams();
		output.clear();
		
		try {
			PyCodeObject *co = (PyCodeObject*)Py_CompileStringFlags(
				input.c_str(), "<console>", Py_single_input, &cf
			);
			
			if (co == NULL) {
				py::throw_error_already_set();
			}
			
			py::object ns = py::import("__gcdevconsole__").attr("__dict__");
			PyObject *ret = PyEval_EvalCode(co, ns.ptr(), ns.ptr());
			
			if (ret == NULL) {
				Py_DECREF(co);
				py::throw_error_already_set();
			}
			
			Py_DECREF(ret);
			Py_DECREF(co);
			//py::handle<> retH(ret);
			//py::object repr = py::object(py::handle<>(PyObject_Repr(ret)));
			
			output = GetStreamValue();// + "\n" + std::string(py::extract<char*>(repr));
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
		
		RestoreStreams();
		return Render(error);
	}
};

void ShowDevConsole() {
	int w = Game::Inst()->ScreenWidth() - 4;
	int h = 25;
	int x = 2;
	int y = Game::Inst()->ScreenHeight() - h - 2;
	
	TCOD_key_t key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;

	TCODConsole *c = TCODConsole::root;
	
	bool clicked = false;
	int scroll = 0;
	unsigned maxScroll = 0;
	
	DevConsole console(w - 2);
	
	// I tried to use the UI code. Really. I can't wrap my head around it.
	// FIXME: That's OK. :)
	while (true) {
		c->setDefaultForeground(TCODColor::white);
		c->setDefaultBackground(TCODColor::black);
		c->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Developer console");
		c->setAlignment(TCOD_LEFT);
		
		TCODConsole::blit(&console.canvas, 0, scroll, w - 2, h - 5, c, x + 1, y + 1);
		
		c->putChar(x + w - 2, y + 1,     TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		c->putChar(x + w - 2, y + h - 4, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
		
		for (int i = 1; i < w - 1; ++i) {
			c->putChar(x + i, y + h - 3, TCOD_CHAR_HLINE, TCOD_BKGND_SET);
		}
		
		c->putChar(x,         y + h - 3, TCOD_CHAR_TEEE, TCOD_BKGND_SET);
		c->putChar(x + w - 1, y + h - 3, TCOD_CHAR_TEEW, TCOD_BKGND_SET);
		
		c->print(x + 1, y + h - 2, "[In %d]: %s", console.inputID, console.input.c_str());
		
		c->flush();

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {
		    if (key.vk == TCODK_ESCAPE) {
			return;
		    } else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
			maxScroll = console.Eval();
		    } else if (key.vk == TCODK_BACKSPACE && console.input.size() > 0) {
			console.input.erase(console.input.end() - 1);
		    } else if (key.c >= ' ' && key.c <= '~') {
			console.input.push_back(key.c);
		    }
		}

		// if (event & TCOD_EVENT_MOUSE) {
		//     mouse = TCODMouse::getStatus();
		// }

		// if (mouse.lbutton) {
		// 	clicked = true;
		// }
		
		// if (clicked && !mouse.lbutton) {
		if (event & TCOD_EVENT_MOUSE_PRESS) {
		    if (mouse.lbutton) {
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
}
