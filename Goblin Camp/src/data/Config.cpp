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

#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/assign/list_inserter.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/foreach.hpp>

#include "data/Config.hpp"
#include "data/Paths.hpp"
#include "Logger.hpp"

namespace {
	namespace globals {
		Config::CVarMap cvars;
		Config::KeyMap  keys;
	}
	
	void _SaveNoThrow() {
		try {
			Config::Save();
		} catch (...) {
			// pass
		}
	}
}

namespace Config {
	void Save() {
		std::ofstream config(Paths::Get(Paths::Config).string().c_str());
		config << "##\n";
		config << "## Config automatically saved on " << boost::posix_time::second_clock::local_time() << '\n';
		config << "##\n";
		
		// dump cvars
		BOOST_FOREACH(CVarMap::value_type pair, globals::cvars) {
			config << "setCVar('" << pair.first << "', '" << pair.second << "')\n";
		}
		
		// dump keys
		BOOST_FOREACH(KeyMap::value_type pair, globals::keys) {
			config << "bindKey('" << pair.first << "', '" << pair.second << "')\n";
		}
		
		config.close();
	}
	
	// Set config defaults.
	void Init() {
		using boost::assign::insert;
		
		insert(globals::cvars)
			("resolutionX", "800")
			("resolutionY", "600")
			("fullscreen",  "0")
			("renderer",    "0")
		;
		
		insert(globals::keys)
			("Exit",          'q')
			("Basics",        'b')
			("Workshops",     'w')
			("Orders",        'o')
			("Furniture",     'f')
			("StockManager",  's')
			("Squads",        'm')
			("Announcements", 'a')
			("Center",        'c')
			("Help",          'h')
			("Pause",         ' ')
			("Jobs",          'j')
		;
		
		atexit(_SaveNoThrow);
	}
	
	void SetStringCVar(const std::string& n, const std::string& v) {
		LOG("Setting " << n << " to " << v);
		globals::cvars[n] = v;
	}
	
	std::string GetStringCVar(const std::string& n) {
		return globals::cvars[n];
	}
	
	const CVarMap& GetCVarMap() {
		return globals::cvars;
	}
	
	char GetKey(const std::string& n) {
		return globals::keys[n];
	}
	
	void SetKey(const std::string& n, const char v) {
		LOG("Setting " << n << " to " << v);
		if (globals::keys.find(n) == globals::keys.end()) {
			LOG("WARNING: Key " << n << " was not specified in the defaults -- it could mean the name has changed between releases.");
		}
		globals::keys[n] = v;
	}
	
	KeyMap& GetKeyMap() {
		return globals::keys;
	}
}
