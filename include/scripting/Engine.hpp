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
#pragma once

#include <vector>
#include <string>

namespace boost { namespace python {
	class object;
}}

namespace Script {
	// Only mods with apiVersion property that equals to this will have their scripts loaded.
	extern const short version;
	
	// Initialises the engine.
	void Init(std::vector<std::string>&);
	
	// Shuts down the engine.
	void Shutdown();
	
	// Loads mod's __init__.py.
	void LoadScript(const std::string&, const std::string&);
	
	// Logs active exception (noop if no exception is active).
	void LogException(bool clear = true);
	
	// Extracts exception information into separate objects.
	void ExtractException(boost::python::object&, boost::python::object&, boost::python::object&);
}
