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
#include <vector>
#include <string>
#include <list>
#include <cstdlib>
#include <ostream> // std::flush

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
namespace py = boost::python;

#include "Data.hpp"
#include "scripting/Engine.hpp"
#include "scripting/API.hpp"
#include "Logger.hpp"

namespace {
	namespace globals {
		PyObject *loadPackageFunc, *printExcFunc;
		py::object logger;
	}
}

namespace Script {
	const short version = 0;
	
	void Init(std::vector<std::string>& args) {
		Logger::Inst()->output << "[Script] Initialising engine.\n";
		
		Py_NoSiteFlag = 1;
		Py_InitializeEx(0);
		Py_SetProgramName(const_cast<char*>(args[0].c_str()));
		
		Logger::Inst()->output << "[Script] Python " << Py_GetVersion() << "\n";
		
		// Don't use default search path.
		{
		#ifdef WINDOWS
			char pathsep = ';';
		#else
			char pathsep = ':';
		#endif
			std::string path = (Data::GetPath(Data::Path::GlobalData) / "lib").string();
			path += pathsep;
			path += (Data::GetPath(Data::Path::GlobalData) / "lib" / "stdlib.zip").string();
			
			PySys_SetPath(const_cast<char*>(path.c_str()));
		}
		
		try {
			// This cannot possibly fail. Unless the universe has blown up.
			py::object res = py::eval(
				"repr(__import__('sys').path)",
				py::import("__builtin__").attr("__dict__")
			);
			Logger::Inst()->output << "[Script] sys.path = " << py::extract<char*>(res) << "\n";
		} catch (const py::error_already_set&) {
			Logger::Inst()->output << "[Script] Bootstrap failed.\n";
			LogException();
			Logger::Inst()->output.flush();
			exit(20);
		}
		
		// Get utility functions.
		Logger::Inst()->output << "[Script] import imp.\n" << std::flush;
		PyObject *modImp = PyImport_ImportModule("imp");
		Logger::Inst()->output << "[Script] import traceback.\n" << std::flush;
		PyObject *modTraceback = PyImport_ImportModule("traceback");
		
		assert(modImp);
		assert(modTraceback);
		
		Logger::Inst()->output << "[Script] printExcFunc = traceback.print_exception.\n" << std::flush;
		globals::printExcFunc = PyObject_GetAttrString(modTraceback, "print_exception");
		Logger::Inst()->output << "[Script] loadPackageFunc = imp.load_package.\n" << std::flush;
		globals::loadPackageFunc = PyObject_GetAttrString(modImp, "load_package");
		
		assert(globals::loadPackageFunc);
		assert(globals::printExcFunc);
		
		Py_DECREF(modImp);
		Py_DECREF(modTraceback);
		
		ExposeAPI();
		PyImport_AddModule("gcmods");
		
		globals::logger = API::pyLoggerStream();
		Logger::Inst()->output.flush();
	}
	
	void Shutdown() {
		Logger::Inst()->output << "[Script] Shutting down engine.\n";
		
		Py_DECREF(globals::loadPackageFunc);
		
		ReleaseListeners();
		Py_Finalize();
	}
	
	void LoadScript(const std::string& mod, const std::string& directory) {
		Logger::Inst()->output << "[Script] Loading '" << directory << "' into 'gcmods." << mod << "'.\n" << std::flush;
		
		try {
			py::call<void>(globals::loadPackageFunc, "gcmods." + mod, directory);
		} catch (const py::error_already_set&) {
			LogException();
		}
		//PyObject_CallFunction(globals::loadPackageFunc, "ss", mod.c_str(), directory.c_str());
	}
	
	void LogException(bool clear) {
		if (PyErr_Occurred() == NULL) return;
		
		PyObject *excType, *excVal, *excTB;
		PyErr_Fetch(&excType, &excVal, &excTB);
		PyErr_Clear();
		
		// Boost.Python's call cannot use raw PyObject*.
		// And its documentation sucks.
		Logger::Inst()->output << "**** Python exception occurred ****\n";
		Py_DECREF(PyObject_CallFunction(
			globals::printExcFunc, "OOOOO", excType, excVal, excTB, Py_None, globals::logger.ptr()
		));
		Logger::Inst()->output << "***********************************\n" << std::flush;
		
		if (PyErr_Occurred() != NULL) {
			Logger::Inst()->output << "[Script] INTERNAL ERROR IN LOGEXCEPTION, SEE STDERR.\n";
			PyErr_Print();
			PyErr_Clear();
		}
		
		if (!clear) {
			PyErr_Restore(excType, excVal, excTB);
		}
		
		Py_DECREF(excType);
		Py_DECREF(excVal);
		Py_DECREF(excTB);
	}
}
