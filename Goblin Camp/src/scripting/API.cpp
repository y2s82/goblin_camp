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

namespace py = boost::python;

#include "Coordinate.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Item.hpp"
#include "scripting/API.hpp"
#include "scripting/Engine.hpp"

namespace {
	namespace globals {
		std::list<py::object> listeners;
	}
}

namespace Script { namespace API {
	// LoggerStream
	LoggerStream::LoggerStream() { }
	void LoggerStream::close() { }
	void LoggerStream::flush() { }
	
	void LoggerStream::write(const char *str) {
		Logger::log << str;
	}
	
	void announce(const char *str) {
		Announce::Inst()->AddMsg(str);
	}
	
	// ItemProxy
	ItemProxy::ItemProxy(Item *item) : item(item) { }
	
	std::string ItemProxy::getName() {
		return Item::ItemTypeToString(item->Type());
	}
	
	py::object pyLoggerStream;
	py::object pyCoordinate;
	py::object pyItem;
	py::object pyItemCat;
	
	BOOST_PYTHON_MODULE(_gcampapi) {
		pyLoggerStream = py::class_<LoggerStream>("LoggerStream")
			.def("close", &LoggerStream::close)
			.def("write", &LoggerStream::write)
			.def("flush", &LoggerStream::flush)
		;
		
		// Coordinate is simple enough to be exposed directly
		// (but with read-only X and Y properties).
		pyCoordinate = py::class_<Coordinate>("Coordinate", py::init<int, int>())
			.add_property("x", (int (Coordinate::*)() const)&Coordinate::X)
			.add_property("y", (int (Coordinate::*)() const)&Coordinate::Y)
			.def(py::self < py::self)
			.def(py::self == py::self)
			.def(py::self != py::self)
			.def(py::self + py::self)
			.def(py::self + py::other<int>())
			.def(py::self - py::other<int>())
		;
		
		pyItem = py::class_<ItemProxy>("ItemProxy", py::init<Item*>())
			.add_property("name", &ItemProxy::getName)
		;
		
		py::def("announce", &announce);
		py::def("appendListener", &Script::AppendListener);
	}
}}

namespace Script {
	void ExposeAPI() {
		LOG("Exposing API.");
		API::init_gcampapi();
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
