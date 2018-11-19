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

#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/assign/list_inserter.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/foreach.hpp>

#include "data/Config.hpp"
#include "data/Paths.hpp"
#include "Logger.hpp"

namespace Globals {
	/**
		\var cvars
		A map of configuration variables.
		
		\var keys
		A map of key bindings.
	*/
	Config::CVarMap cvars;
	Config::KeyMap  keys;
}

/**
	Interface to manage game's configuration.
*/
namespace Config {
	/**
		Saves current configuration to user's configuration file.
		
		\see Paths
		\throws std::exception Refer to std::ofstream documentation.
	*/
	void Save() {
		std::ofstream config(Paths::Get(Paths::Config).string().c_str());
		config << "##\n";
		config << "## Config automatically saved on " << boost::posix_time::second_clock::local_time() << '\n';
		config << "##\n";
		
		// dump cvars
		BOOST_FOREACH(CVarMap::value_type pair, Globals::cvars) {
			config << "setCVar('" << pair.first << "', '" << pair.second << "')\n";
		}
		
		// dump keys
		BOOST_FOREACH(KeyMap::value_type pair, Globals::keys) {
			config << "bindKey('" << pair.first << "', '" << pair.second << "')\n";
		}
		
		config.close();
	}
	
	/**
		Creates configuration variables, and default key bindings.
	*/
	void Init() {
		using boost::assign::insert;
		
		insert(Globals::cvars)
			("resolutionX",  "800")
			("resolutionY",  "600")
			("fullscreen",   "0")
			("renderer",     "0")
			("useTileset",   "0")
			("tileset",      "")
			("tutorial",     "1")
			("riverWidth",   "30")
			("riverDepth",   "5")
			("halfRendering","0")
			("compressSaves","0")
			("translucentUI","0")
			("autosave","1")
			("pauseOnDanger","0")
		;
		
		insert(Globals::keys)
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
			("DevConsole",    '`')
			("TerrainOverlay",'t')
			("Permanent",     'p')
		;
	}
	
	/**
		Changes value of a configuration variable.
		
		\param[in] name  Name of the variable.
		\param[in] value New value for the variable.
	*/
	void SetStringCVar(const std::string& name, const std::string& value) {
		LOG("Setting " << name << " to " << value);
		Globals::cvars[name] = value;
	}
	
	/**
		Retrieves value of a configuration variable. If the variable doesn't exist,
		it will be created, set to empty string and then returned.
		
		\param[in] name Name of the variable.
		\returns        Value of the variable.
	*/
	std::string GetStringCVar(const std::string& name) {
		if (Globals::cvars.find(name) == Globals::cvars.end()) {
			LOG("WARNING: CVar "<< name << " doesn't exist.");
			return std::string("");
		}
		return Globals::cvars[name];
	}
	
	/**
		Retrieves all defined configuration variables.
		
		\returns A constant reference to the configuration variables map.
	*/
	const CVarMap& GetCVarMap() {
		return Globals::cvars;
	}
	
	/**
		Retrieves keycode bound to a named key. If the key doesn't exist,
		NULL keycode (\c 0) will be returned and warning will be logged.
		
		\param[in] name Name of the key.
		\returns        Currently bound keycode, or 0.
	*/
	char GetKey(const std::string& name) {
		if (Globals::keys.find(name) == Globals::keys.end()) {
			LOG("WARNING: Key " << name << " doesn't exist -- this may indicate outdated code.");
			return '\0';
		}
		
		return Globals::keys[name];
	}
	
	/**
		Changes keycode bound to a named key. If the key doesn't exist,
		a warning will be logged (but the binding will be saved).
		
		\param[in] name  Name of the key.
		\param[in] value New keycode for the key.
	*/
	void SetKey(const std::string& name, const char value) {
		LOG("Setting " << name << " to '" << value << "'");
		
		if (Globals::keys.find(name) == Globals::keys.end()) {
			LOG("WARNING: Key " << name << " was not specified in the defaults -- it could mean the name has changed between releases.");
		}
		
		Globals::keys[name] = value;
	}
	
	/**
		Retrieves all key bindings.
		
		\returns Non-constant reference to the key bindings map. Callers should be careful not to introduce new keys this way.
	*/
	KeyMap& GetKeyMap() {
		return Globals::keys;
	}
}
