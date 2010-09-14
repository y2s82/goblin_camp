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

#include "scripting/Engine.hpp"
#include "scripting/API.hpp"

#include "Announce.hpp"
#include "Logger.hpp"

namespace Script { namespace API {
	PyObject* AddListener(PyObject *self, PyObject *args) {
		PyObject *listener;
		
		if (!PyArg_ParseTuple(args, "O", &listener)) {
			return NULL;
		}
		
		Script::AppendListener(listener);
		Py_RETURN_NONE;
	}
	
	PyObject* Announce(PyObject *self, PyObject *args) {
		const char *message;
		
		if (!PyArg_ParseTuple(args, "s", &message)) {
			return NULL;
		}
		
		Announce::Inst()->AddMsg(message);
		Py_RETURN_NONE;
	}
}}
