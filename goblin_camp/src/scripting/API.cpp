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

#include <cassert>
#include <cstdarg>
#include <list>
#include <boost/foreach.hpp>

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
namespace py = boost::python;

#include "data/Config.hpp"
#include "scripting/API.hpp"
#include "scripting/Engine.hpp"
#include "scripting/_gcampapi/LoggerStream.hpp"
#include "scripting/_gcampapi/Functions.hpp"
#include "scripting/_gcampapi/PyItem.hpp"
#include "scripting/_gcampapi/PyConstruction.hpp"
#include "Logger.hpp"

namespace Globals {
	std::list<py::object> listeners;
}

namespace Script { namespace API {
	BOOST_PYTHON_MODULE(_gcampapi) {
		typedef void (*ExposeFunc)(void);
		ExposeFunc expose[] = {
			&ExposeLoggerStream, &ExposeFunctions, &PyItem::Expose, &PyConstruction::Expose
		};
		
		for (unsigned idx = 0; idx < sizeof(expose) / sizeof(expose[0]); ++idx) {
			expose[idx]();
		}
	}
	
	BOOST_PYTHON_MODULE(_gcampconfig) {
		py::def("setCVar", &Config::SetStringCVar);
		py::def("getCVar", &Config::GetStringCVar);
		py::def("bindKey", &Config::SetKey);
		py::def("getKey",  &Config::GetKey);
	}
}}

namespace Script {
	void ExposeAPI() {
		API::init_gcampapi();
		API::init_gcampconfig();
	}
	
	void AppendListener(PyObject *listener) {
		assert(listener);
		py::handle<> hListener(py::borrowed(listener));
		
		py::object oListener(hListener);
		{
			py::object repr(py::handle<>(PyObject_Repr(listener)));
			LOG("New listener: " << py::extract<char*>(repr) << ".");
		}
		
		Globals::listeners.push_back(oListener);
	}
	
	void InvokeListeners(const char *method, PyObject *args) {
		BOOST_FOREACH(py::object listener, Globals::listeners) {
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
	
	void InvokeListeners(const char *method, const char *format, ...) {
		va_list argList;
		va_start(argList, format);
		
		py::object args(py::handle<>(
			Py_VaBuildValue(format, argList)
		));
		
		va_end(argList);
		
		InvokeListeners(method, args.ptr());
	}
	
	void ReleaseListeners() {
		Globals::listeners.clear();
	}
}
