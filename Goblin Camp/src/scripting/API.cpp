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

namespace py = boost::python;

#include "Announce.hpp"
#include "Logger.hpp"
#include "scripting/API.hpp"
#include "scripting/Engine.hpp"

namespace {
	namespace globals {
		std::list<PyObject*> listeners;
	}
}

namespace Script { namespace API {
	LoggerStream::LoggerStream() : logger(Logger::Inst()) { }
	void LoggerStream::close() { }
	
	void LoggerStream::write(const char *str) {
		logger->output << str;
	}
	
	void LoggerStream::flush() {
		logger->output.flush();
	}
	
	void announce(const char *str) {
		Announce::Inst()->AddMsg(str);
	}
	
	py::object pyLoggerStream;
	
	BOOST_PYTHON_MODULE(_gcampapi) {
		pyLoggerStream = py::class_<LoggerStream>("LoggerStream")
			.def("close", &LoggerStream::close)
			.def("write", &LoggerStream::write)
			.def("flush", &LoggerStream::flush)
		;
		
		py::def("announce", &announce);
		py::def("appendListener", &Script::AppendListener);
	}
}}

namespace Script {
	void ExposeAPI() {
		Logger::Inst()->output << "[Script:API] Exposing API.\n";
		API::init_gcampapi();
	}
	
	void AppendListener(PyObject *listener) {
		assert(listener);
		Py_INCREF(listener);
		
		PyObject *repr = PyObject_Repr(listener);
		assert(repr);
		Logger::Inst()->output << "[Script:API] New listener: " << PyString_AsString(repr) << ".\n";
		Py_DECREF(repr);
		
		globals::listeners.push_back(listener);
	}
	
	void InvokeListeners(char *method, PyObject *args) {
		BOOST_FOREACH(PyObject *listener, globals::listeners) {
			if (!PyObject_HasAttrString(listener, method)) {
				continue;
			}
			
			PyObject *callable = PyObject_GetAttrString(listener, method);
			Py_DECREF(PyObject_CallObject(callable, args));
			Py_DECREF(callable);
			
			LogException();
		}
		// Caller is expected to do InvokeListeners("foo", Py_BuildValue("...")).
		// That's why we DECREF the args tuple here.
		Py_XDECREF(args);
	}
	
	void ReleaseListeners() {
		typedef std::list<PyObject*>::iterator Iterator;
		
		// Removing items, so can't use foreach.
		for (Iterator it = globals::listeners.begin(); it != globals::listeners.end();) {
			Py_DECREF(*it);
			
			it = globals::listeners.erase(it);
		}
	}
}
