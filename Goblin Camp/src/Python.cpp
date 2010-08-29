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

#ifdef GC_PYTHON

#include <cassert>

#include <boost/python.hpp>
namespace py = boost::python;

#include "Logger.hpp"
#include "Python.hpp"

namespace {
	unsigned int getAPIVersion() { return Python::apiVersion; }
	
	BOOST_PYTHON_MODULE(gcamp) {
		py::def("getAPIVersion", getAPIVersion);
	}
}

namespace Python {
	const unsigned int apiVersion = 0;
	
	void Init() {
		Py_SetProgramName("goblin-camp");
		Py_InitializeEx(0);
		
		Logger::Inst()->output << "[Python] Init: " << Py_GetVersion() << "\n";
		PyImport_AppendInittab("gcamp", initgcamp);
		
		try {
			unsigned int v = py::extract<unsigned int>(
				py::eval("__import__('gcamp').getAPIVersion()", py::import("__builtin__").attr("__dict__"))
			);
			assert(v == apiVersion);
			
			Logger::Inst()->output << "[Python] gcamp.getAPIVersion() = " << v << "\n";
		} catch (const py::error_already_set&) {
			PyErr_Print();
			Logger::Inst()->output << "[Python] getAPIVersion could not be called.\n";
		}
	}
}

#endif // GC_PYTHON
