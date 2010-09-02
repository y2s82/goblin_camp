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

#include <Python.h>

#include <vector>
#include <string>
#include <list>
#include <cstdlib>

#include <boost/foreach.hpp>

#include "Data.hpp"
#include "Engine.hpp"
#include "Logger.hpp"

namespace {
	namespace globals {
		//lua_State *lua;
	}
}

namespace Script {
	const short version = 0;
	
	void Init(std::vector<std::string>& args) {
		Logger::Inst()->output << "[Script] Initialising engine.\n";
		
		Py_InitializeEx(0);
		Py_SetProgramName(const_cast<char*>(args[0].c_str()));
		char *pyargv[] = { "" };
		PySys_SetArgvEx(1, pyargv, 0);
		
		Logger::Inst()->output << "[Script] Python " << Py_GetVersion() << "\n";
	}
	
	void Shutdown() {
		Logger::Inst()->output << "[Script] Shutting down engine.\n";
		
		Py_Finalize();
	}
	
	//void Load
}
