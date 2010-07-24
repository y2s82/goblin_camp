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

#include <libtcod.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>

namespace fs = boost::filesystem;

#include "Data.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "Item.hpp"
#include "NatureObject.hpp"
#include "NPC.hpp"
#include "Construction.hpp"

// These functions are platform-specific, and are defined in <platform>/DataImpl.cpp.
void _ImplFindPersonalDirectory(std::string&);
void _ImplFindExecutableDirectory(fs::path&, fs::path&, fs::path&);

namespace {
	namespace globals {
		fs::path personalDir, exec, execDir, dataDir;
		fs::path savesDir, screensDir, modsDir;
		fs::path config, font;
	}
	
	// Finds path to 'personal dir' and subdirs.
	inline void FindPersonalDirectory() {
		std::string dir;
		
		_ImplFindPersonalDirectory(dir);
		
		globals::personalDir = fs::path(dir);
		globals::savesDir    = globals::personalDir / "saves";
		globals::screensDir  = globals::personalDir / "screenshots";
		globals::config      = globals::personalDir / "config.ini";
		globals::font        = globals::personalDir / "terminal.png";
		globals::modsDir     = globals::personalDir / "mods";
	}
	
	// Finds 'executable', 'bin-dir' and 'global data'.
	inline void FindExecutableDirectory() {
		_ImplFindExecutableDirectory(globals::exec, globals::execDir, globals::dataDir);
	}
	
	void _LoadNames(std::string fn) {
		TCODNamegen::parse(fn.c_str());
	}
	
	void _TryLoadDataFile(
		bool global, const fs::path& dir1, const fs::path& dir2,
		const std::string& filename, void (*load)(std::string)
	) {
		std::string fn = filename + ".dat";
		fs::path dir;
		
		Logger::Inst()->output << "[Data] Searching for: " << fn << "\n";
		
		if (fs::exists(dir1 / fn)) {
			dir = dir1;
		} else if (global) {
			if (fs::exists(dir2 / fn)) {
				dir = dir2;
			} else {
				Logger::Inst()->output << "[Data] Cannot find global data file -- abort.\n";
				Logger::Inst()->output.flush();
				exit(5);
			}
		} else {
			Logger::Inst()->output << "[Data] Not found, skipping.\n";
			return;
		}
		
		Logger::Inst()->output << "[Data] Loading: " << (dir / fn).string() << "\n";
		load((dir / fn).string());
	}
	
	inline void TryLoadGlobalDataFile(const std::string& filename, void (*load)(std::string)) {
		_TryLoadDataFile(true, globals::dataDir, globals::execDir, filename, load);
	}
	
	inline void TryLoadLocalDataFile(const fs::path& dir, const std::string& filename, void (*load)(std::string)) {
		_TryLoadDataFile(false, dir, dir, filename, load);
	}
	
	void LoadGlobalMod() {
		Logger::Inst()->output << "[Data] Loading global data files.\n";
		
		// Item presets _must_ be loaded first because constructions refer to items by name
		TryLoadGlobalDataFile("items",         Item::LoadPresets);
		TryLoadGlobalDataFile("constructions", Construction::LoadPresets);
		TryLoadGlobalDataFile("wildplants",    NatureObject::LoadPresets);
		TryLoadGlobalDataFile("names",         _LoadNames);
		TryLoadGlobalDataFile("creatures",     NPC::LoadPresets);
	}
	
	void LoadLocalMods() {
		fs::directory_iterator end;
		for (fs::directory_iterator it(globals::modsDir); it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			
			fs::path mod = it->path();
			Logger::Inst()->output << "[Data] Loading mod: " << mod.filename() << "\n";
			
			TryLoadLocalDataFile(mod, "items",         Item::LoadPresets);
			TryLoadLocalDataFile(mod, "constructions", Construction::LoadPresets);
			TryLoadLocalDataFile(mod, "wildplants",    NatureObject::LoadPresets);
			TryLoadLocalDataFile(mod, "names",         _LoadNames);
			TryLoadLocalDataFile(mod, "creatures",     NPC::LoadPresets);
		}
	}
}

namespace Data {
	void Init() {
		std::string personalDir, exec;
		fs::path execDir;
		
		// bootstrap to get logging up and running
		FindPersonalDirectory();
		fs::create_directory(globals::personalDir);
		Logger::Create((globals::personalDir / "goblin-camp.log").string());
		
		Logger::Inst()->output << "[Data] Data::Init()\n";
		FindExecutableDirectory();
		
		Logger::Inst()->output <<
			"[Data] Personal directory: " << globals::personalDir << "\n" <<
			"[Data] Saves directory: " << globals::savesDir << "\n" <<
			"[Data] Screenshots directory: " << globals::screensDir << "\n" <<
			"[Data] Mods directory: " << globals::modsDir << "\n" <<
			"[Data] Executable Directory: " << globals::execDir << "\n" <<
			"[Data] Global Data Directory: " << globals::dataDir << "\n" <<
			"[Data] --------\n" <<
			"[Data] Executable: " << globals::exec << "\n" <<
			"[Data] Config: " << globals::config << "\n" <<
			"[Data] Font: " << globals::font << "\n"
		;
		
		// try to create personal directory structure
		// create_directory doesn't throw if directory already exist
		try {
			fs::create_directory(globals::savesDir);
			fs::create_directory(globals::screensDir);
			fs::create_directory(globals::modsDir);
		} catch (const fs::filesystem_error& e) {
			Logger::Inst()->output << "[Data] filesystem_error while creating directories: " << e.what() << "\n";
			Logger::Inst()->output.flush();
			exit(1);
		}
		
		// now check whether user has config.ini
		//   if not, check whether default one exist (data-dir/config.ini)
		//     if so, copy it to user config.ini
		//     if not, write hardcoded defaults
		if (!fs::exists(globals::config)) {
			Logger::Inst()->output << "[Data] User's config.ini does not exist.\n";
			
			fs::path defaultConfig = globals::dataDir / "config.ini";
			if (!fs::exists(defaultConfig)) {
				defaultConfig = globals::execDir / "config.ini";
			}
			
			if (fs::exists(defaultConfig)) {
				Logger::Inst()->output << "[Data] Copying default config.ini to user directory.\n";
				
				try {
					fs::copy_file(defaultConfig, globals::config);
				} catch (const fs::filesystem_error& e) {
					Logger::Inst()->output << "[Data] filesystem_error while copying config: " << e.what() << "\n";
					Logger::Inst()->output.flush();
					exit(2);
				}
			} else {
				Logger::Inst()->output << "[Data] Creating default config.ini.\n";
				
				try {
					std::ofstream configStream(globals::config.string().c_str());
					configStream << "config {\n\twidth = 800\n\theight = 600\n\trenderer = \"SDL\"\n}";
					configStream.close();
				} catch (const std::exception &e) {
					Logger::Inst()->output << "[Data] std::exception while creating config: " << e.what() << "\n";
					Logger::Inst()->output.flush();
					exit(3);
				}
			}
		}
		
		// check whether custom font exists
		// if not, copy default one
		if (!fs::exists(globals::font)) {
			Logger::Inst()->output << "[Data] User's terminal.png does not exist, copying default.\n";
			
			fs::path defaultFont = globals::dataDir / "terminal.png";
			if (!fs::exists(defaultFont)) {
				defaultFont = globals::execDir / "terminal.png";
			}
			
			try {
				fs::copy_file(defaultFont, globals::font);
			} catch (const fs::filesystem_error& e) {
				Logger::Inst()->output << "[Data] filesystem_error while copying font: " << e.what() << "\n";
				Logger::Inst()->output.flush();
				exit(4);
			}
		}
		
		Logger::Inst()->output << "[Data] Loading config.ini.\n";
		Game::Inst()->LoadConfig(globals::config.string());
		
		Logger::Inst()->output << "[Data] Loading terminal.png.\n";
		TCODConsole::setCustomFont(globals::font.string().c_str());
		
		Logger::Inst()->output << "[Data] Data::Init() finished.\n";
		Logger::Inst()->output.flush();
	}
	
	void Load() {
		Logger::Inst()->output << "[Data] Data::Load()\n";
		
		// load global data files
		LoadGlobalMod();
		
		// and then user mods
		// each mod is a directory with data files, e.g.
		//   + mod1
		//   |- items.dat
		//   |- constructions.dat
		//   |- names.dat
		//   |- wildplants.dat
		//   |- creatures.dat
		//   + mod2
		//   |- ...
		LoadLocalMods();
		
		#ifdef MACOSX
		// workaround
		Logger::Inst()->output << "[Data] Loading terminal.png again.\n";
		TCODConsole::setCustomFont(globals::font.string().c_str());
		#endif
		
		Logger::Inst()->output << "[Data] Data::Load() finished.\n";
		Logger::Inst()->output.flush();
	}
	
	void GetSavedGames(TCODList<std::string>& list) {
		fs::directory_iterator end;
		for (fs::directory_iterator it(globals::savesDir); it != end; ++it) {
			fs::path save = it->path();
			if (!boost::iequals(save.extension(), ".sav")) continue;
			
			save.replace_extension();
			list.push(save.filename());
		}
	}
	
	void LoadGame(const std::string& save) {
		std::string file = (globals::savesDir / save).string() + ".sav";
		Logger::Inst()->output << "[Data] Loading game from " << file << "\n";
		Game::Inst()->LoadGame(file);
		
		Logger::Inst()->output.flush();
	}
	
	void SaveGame(const std::string& save) {
		std::string file = (globals::savesDir / save).string() + ".sav";
		Logger::Inst()->output << "[Data] Saving game to " << file << "\n";
		Game::Inst()->SaveGame(file);
		
		Logger::Inst()->output.flush();
	}
	
	void SaveScreenshot() {
		// sadly, libtcod supports autonumbering only when saving to current dir
		fs::directory_iterator end;
		unsigned int largest = 0;
		
		for (fs::directory_iterator it(globals::screensDir); it != end; ++it) {
			fs::path png = it->path();
			if (!boost::iequals(png.extension(), ".png")) continue;
			
			png.replace_extension();
			
			std::string file = png.filename();
			try {
				// screens are saved as screenXXXXXX.png
				largest = std::max(largest, boost::lexical_cast<unsigned int>(file.substr(6)));
			} catch (const std::exception& e) {
				// not worth terminating the game for
				(void)e; // variable not referenced warning
			}
		}
		
		std::string png = (
			globals::screensDir / ((boost::format("screen%|06|.png") % (largest + 1)).str())
		).string();
		
		Logger::Inst()->output << "[Data] Saving screenshot to " << png << "\n";
		TCODSystem::saveScreenshot(png.c_str());
		
		Logger::Inst()->output.flush();
	}
}
