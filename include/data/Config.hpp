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

#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>

// Data refactoring: game configuration.

namespace Config {
	typedef boost::unordered_map<std::string, std::string> CVarMap;
	typedef boost::unordered_map<std::string, char> KeyMap;
	
	void Init();
	void Save();
	
	// config variables
	void SetStringCVar(const std::string&, const std::string&);
	std::string GetStringCVar(const std::string&);
	
	template <typename T>
	inline T GetCVar(const std::string& name) {
		return boost::lexical_cast<T>(GetStringCVar(name));
	}
	
	template <typename T>
	inline void SetCVar(const std::string& name, const T& value) {
		SetStringCVar(name, boost::lexical_cast<std::string>(value));
	}
	
	const CVarMap& GetCVarMap();
	
	// key bindings
	char GetKey(const std::string&);
	void SetKey(const std::string&, const char);
	KeyMap& GetKeyMap();
}
