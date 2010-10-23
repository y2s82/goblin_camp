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

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "Logger.hpp"
#include "data/Paths.hpp"

// These functions are platform-specific, and are defined in <platform>/DataImpl.cpp.
namespace PathsImpl {
	void FindPersonalDirectory(fs::path&);
	void FindExecutableDirectory(fs::path&, fs::path&, fs::path&);
}

namespace {
	namespace globals {
		fs::path personalDir, exec, execDir, dataDir;
		fs::path savesDir, screensDir, modsDir;
		fs::path config, keys, font;
	}
	
	// Bootstrap to get logging up and running.
	inline void Bootstrap() {
		using namespace globals;
		
		PathsImpl::FindPersonalDirectory(personalDir);
		
		savesDir    = personalDir / "saves";
		screensDir  = personalDir / "screenshots";
		modsDir     = personalDir / "mods";
		
		config      = personalDir / "config.py";
		keys        = personalDir / "keys.ini";
		font        = personalDir / "terminal.png";
		
		PathsImpl::FindExecutableDirectory(exec, execDir, dataDir);
		
		fs::create_directory(personalDir);
		Logger::OpenLogFile((personalDir / "goblin-camp.log").string());
	}
}

namespace Paths {
	void Init() {
		Bootstrap();
		
		LOG("Personal directory: " << globals::personalDir);
		LOG("Saves directory: " << globals::savesDir);
		LOG("Screenshots directory: " << globals::screensDir);
		LOG("Mods directory: " << globals::modsDir);
		LOG("Executable directory: " << globals::execDir);
		LOG("Global data directory: " << globals::dataDir);
		LOG("--------");
		LOG("Executable: " << globals::exec);
		LOG("Config: " << globals::config);
		LOG("Font: " << globals::font);
		
		// try to create personal directory structure
		// create_directory doesn't throw if directory already exist
		try {
			fs::create_directory(globals::savesDir);
			fs::create_directory(globals::screensDir);
			fs::create_directory(globals::modsDir);
		} catch (const fs::filesystem_error& e) {
			LOG("filesystem_error while creating directories: " << e.what());
			exit(1);
		}
	}
	
	#pragma warning(push)
	// "warning C4715: 'Data::GetPath' : not all control paths return a value"
	#pragma warning(disable : 4715)
	const fs::path& Get(Path what) {
		switch (what) {
			case Executable:    return globals::exec;
			case GlobalData:    return globals::dataDir;
			case Personal:      return globals::personalDir;
			case Mods:          return globals::modsDir;
			case Saves:         return globals::savesDir;
			case Screenshots:   return globals::screensDir;
			case Font:          return globals::font;
			case Config:        return globals::config;
			case ExecutableDir: return globals::execDir;
		}
		
		// If control reaches here, then someone added new value to the enum,
		// forgot to add it here, and missed the 'switch not checking
		// every value' warning. So, crash and burn.
		LOG("Impossible code path, crashing.");
		assert(false);
	}
	#pragma warning(pop)
}
