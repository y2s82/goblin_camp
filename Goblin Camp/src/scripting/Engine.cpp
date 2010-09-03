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

#include <vector>
#include <string>
#include <list>
#include <cstdlib>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
namespace py = boost::python;

#include "Data.hpp"
#include "scripting/Engine.hpp"
//#include "scripting/API.hpp"
#include "Logger.hpp"

namespace {
	namespace globals {
		
	}
}

namespace Script {
	const short version = 0;
	
	void Init(std::vector<std::string>& args) {
		Logger::Inst()->output << "[Script] Initialising engine.\n";
		
		Py_NoSiteFlag = 1;
		Py_InitializeEx(0);
		Py_SetProgramName(const_cast<char*>(args[0].c_str()));
		
		// Requires at least Python 2.6.6.
		char *pyargv[] = { "" };
		PySys_SetArgvEx(1, pyargv, 0);
		
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
			Logger::Inst()->output.flush();
			PyErr_Print();
			exit(20);
		}
		
		//ExposeAPI();
		Logger::Inst()->output.flush();
	}
	
	void Shutdown() {
		Logger::Inst()->output << "[Script] Shutting down engine.\n";
		
		Py_Finalize();
	}
	
	void LoadScript(const std::string& mod, const std::string& filename) {
		Logger::Inst()->output << "[Script] Loading " << filename << " into mods." << mod << "\n";
		
	}
}
