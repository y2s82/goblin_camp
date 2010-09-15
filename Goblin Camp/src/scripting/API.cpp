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

#include <cassert>
#include <cstdarg>
#include <list>
#include <boost/foreach.hpp>

namespace py = boost::python;

#include "Logger.hpp"
#include "scripting/API.hpp"
#include "scripting/Engine.hpp"
#include "scripting/_gcampapi/__init__.hpp"
#include "scripting/_gcampapi/LoggerStream.hpp"

namespace {
	namespace globals {
		std::list<py::object> listeners;
	}
	
	using namespace Script::API;
	
	PyMethodDef methods[] = {
		{"addListener", &AddListener, METH_VARARGS, ""},
		{"announce",    &Announce,    METH_VARARGS, ""},
		{NULL}
	};
	
	typedef void (*Init)(PyObject*);
	Init apiTypes[] = {
		&_InitLoggerStream
	};
}

namespace Script {
	void ExposeAPI() {
		PyObject *module = Py_InitModule("_gcampapi", methods);
		
		if (!module) {
			LOG("Could not initialise _gcampapi module.");
			Script::LogException();
			return;
		}
		
		for (unsigned idx = 0; idx < (sizeof(apiTypes) / sizeof(apiTypes[0])); ++idx) {
			apiTypes[idx](module);
		}
	}
	
	namespace API {
		void CreateType(PyTypeObject *type, Py_ssize_t basicSize) {
			memset(type, 0, sizeof(PyTypeObject));
			
			type->ob_refcnt    = 1;
			type->ob_type      = &PyType_Type;
			type->tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
			type->tp_basicsize = basicSize;
			type->tp_doc       = "";
			type->tp_new       = PyType_GenericNew;
		}
		
		void FinishType(PyTypeObject *type, const char *name, PyObject *module) {
			if (PyType_Ready(type) < 0) {
				LOG("Could not finalise API type " << name);
				return;
			}
			
			PyObject *typeObj = (PyObject*)type;
			Py_INCREF(typeObj);
			PyModule_AddObject(module, name, typeObj);
		}
	}
	
	void AppendListener(PyObject *listener) {
		assert(listener);
		py::handle<> hListener(py::borrowed(listener));
		
		py::object oListener(hListener);
		{
			py::object repr(py::handle<>(PyObject_Repr(listener)));
			LOG("New listener: " << py::extract<char*>(repr) << ".");
		}
		
		globals::listeners.push_back(oListener);
	}
	
	void InvokeListeners(char *method, PyObject *args) {
		BOOST_FOREACH(py::object listener, globals::listeners) {
			if (!PyObject_HasAttrString(listener.ptr(), method)) {
				continue;
			}
			
			py::object callable = listener.attr(method);
			try {
				py::handle<> result(
					PyObject_CallObject(callable.ptr(), args)
				);
			} catch (const py::error_already_set&) {
				LogException();
			}
		}
	}
	
	void InvokeListeners(char *method, char *format, ...) {
		va_list argList;
		va_start(argList, format);
		
		py::object args(py::handle<>(
			Py_VaBuildValue(format, argList)
		));
		
		va_end(argList);
		
		InvokeListeners(method, args.ptr());
	}
	
	void ReleaseListeners() {
		globals::listeners.clear();
	}
}
