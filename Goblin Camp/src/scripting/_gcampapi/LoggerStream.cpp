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

#include "scripting/API.hpp"
#include "scripting/_gcampapi/LoggerStream.hpp"
#include "Logger.hpp"

namespace {
	struct Object {
		PyObject_HEAD
	};
	
	PyTypeObject type;
	
	PyObject* Write(PyObject *self, PyObject *args) {
		const char *str;
		
		if (!PyArg_ParseTuple(args, "s", &str)) {
			PyErr_SetString(PyExc_TypeError, "write() expects a string without embedded NULLs");
			return NULL;
		}
		
		Logger::log << str;
		
		Py_RETURN_NONE;
	}
	
	PyMethodDef methods[] = {
		{"write", &Write, METH_VARARGS, ""},
		{NULL}
	};
}

namespace Script { namespace API {
	PyObject* LoggerStream() {
		return PyType_GenericNew(&type, NULL, NULL);
	}
	
	void _InitLoggerStream(PyObject *module) {
		CreateType(&type, sizeof(Object));
		{
			type.tp_name    = "_gcampapi.LoggerStream";
			type.tp_methods = methods;
		}
		FinishType(&type, "LoggerStream", module);
	}
}}
