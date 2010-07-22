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

#if defined(WINDOWS)
#	define NOMINMAX
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <shlobj.h>
#elif defined(LINUX)
#	include <sys/types.h>
#	include <unistd.h>
#elif defined(MACOSX)
#	include <CoreFoundation/CoreFoundation.h>
#else
#	error WINDOWS, LINUX and MACOSX are the only targets supported, ensure correct macros are defined.
#endif

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

namespace {
	namespace globals {
		fs::path personalDir, exec, execDir, dataDir;
		fs::path savesDir, screensDir, modsDir;
		fs::path config, font;
	}
	
	// Finds path to 'personal dir' and subdirs.
	void FindPersonalDirectory() {
		std::string dir;
	#if defined(WINDOWS)
		char myGames[MAX_PATH];
		// gotta love WinAPI
		SHGetFolderPathAndSubDirA(
			NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, "My Games", myGames
		);
		dir = myGames;
		dir += "\\Goblin Camp";
	#elif defined(LINUX)
		dir = getenv("HOME");
		dir += "/.goblincamp";
	#elif defined(MACOSX)
		char buffer[1024];
		CFStringRef username, path;
		username = CSCopyUserName(true);
		path     = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Users/%@/Application Support/Goblin Camp"), username);
		
		CFStringGetCString(path, buffer, sizeof(buffer), kCFStringEncodingUTF8);
		dir = buffer;
		
		CFRelease(username);
		CFRelease(path);
	#endif
		
		globals::personalDir = fs::path(dir);
		globals::savesDir    = globals::personalDir / "saves";
		globals::screensDir  = globals::personalDir / "screenshots";
		globals::config      = globals::personalDir / "config.ini";
		globals::font        = globals::personalDir / "terminal.png";
		globals::modsDir     = globals::personalDir / "mods";
	}
	
	// Finds 'executable', 'bin-dir' and 'global data'.
	void FindExecutableDirectory() {
		std::string exec;
		
	#if defined(WINDOWS)
		char *cmdLine = GetCommandLineA();
		char *ptr     = strchr(cmdLine, ' ');
		
		if (ptr != NULL) *ptr = '\0';
		
		// strip quotes if they're present
		if (cmdLine[0] == '"') {
			char *end = strchr(cmdLine, '\0') - 1;
			if (end != NULL && *end == '"') *end = '\0';
			++cmdLine;
		}
		
		exec = cmdLine;
	#elif defined(LINUX)
		char buffer[1024];
		ssize_t pos = readlink("/proc/self/exe", buffer, 1023);
		buffer[pos] = '\0';
		exec = buffer;
	#elif defined(MACOSX)
		CFBundleRef bundle;
		CFURLRef    execURL, resURL;
		CFStringRef execStr, resStr;
		char execPath[1024], resPath[1024];
		
		bundle  = CFBundleGetMainBundle();
		execURL = CFBundleCopyExecutableURL(bundle);
		resURL  = CFBundleCopyResourcesDirectoryURL(bundle);
		execStr = CFURLCopyFileSystemPath(execURL, kCFURLPOSIXPathStyle);
		resStr  = CFURLCopyFileSystemPath(resURL, kCFURLPOSIXPathStyle);
		
		CFStringGetCString(execStr, execPath, sizeof(execPath), kCFStringEncodingUTF8);
		CFStringGetCString(resStr, resPath, sizeof(resPath), kCFStringEncodingUTF8);
		exec = execPath;
		
		CFRelease(execStr);
		CFRelease(resStr);
		CFRelease(execURL);
		CFRelease(resURL);
		CFRelease(bundle);
	#endif
		
		globals::exec    = fs::path(exec);
		globals::execDir = globals::exec.parent_path();
	#if defined(WINDOWS)
		globals::dataDir = globals::execDir;
	#elif defined(LINUX)
		globals::dataDir = fs::path(globals::execDir.parent_path()) / "share/goblin-camp/";
	#elif defined(MACOSX)
		globals::dataDir = fs::path(std::string(resPath) + "/");
	#endif
	}
	
	void LoadGlobalMod() {
		Logger::Inst()->output << "[Data] Loading global data files.\n";
		
		#define GC_GLOBAL_DATA_FILE(dat) \
			if (fs::exists(globals::dataDir / #dat ".dat")) { \
				dat = (globals::dataDir / #dat ".dat").string(); \
			} else if (fs::exists(globals::execDir / #dat ".dat")) { \
				dat = (globals::execDir / #dat ".dat").string(); \
			}
		
		std::string items, constructions, wildplants, names, creatures;
		
		GC_GLOBAL_DATA_FILE(items)
		GC_GLOBAL_DATA_FILE(constructions)
		GC_GLOBAL_DATA_FILE(wildplants)
		GC_GLOBAL_DATA_FILE(names)
		GC_GLOBAL_DATA_FILE(creatures)
		
		#undef GC_GLOBAL_DATA_FILE
		
		Logger::Inst()->output <<
			"[Data] items.dat: " << items << "\n" <<
			"[Data] constructions.dat: " << constructions << "\n" <<
			"[Data] wildplants.dat: " << wildplants << "\n" <<
			"[Data] names.dat: " << names << "\n" <<
			"[Data] creatures.dat: " << creatures << "\n"
		;
		
		// Item presets _must_ be loaded first because constructions refer to items by name
		Item::LoadPresets(items);
		Construction::LoadPresets(constructions);
		NatureObject::LoadPresets(wildplants);
		TCODNamegen::parse(names.c_str());
		NPC::LoadPresets(creatures);
	}
	
	void LoadLocalMods() {
		fs::directory_iterator end;
		for (fs::directory_iterator it(globals::modsDir); it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			
			fs::path mod = it->path();
			Logger::Inst()->output << "[Data] Loading mod: " << mod.filename() << "\n";
			
			#define GC_MOD_DATA_FILE(dat, cls) \
				if (fs::exists(mod / #dat ".dat")) { \
					Logger::Inst()->output << "[Data] Loading: " << (mod / #dat ".dat") << "\n"; \
					cls::LoadPresets((mod / #dat ".dat").string()); \
				}
			
			GC_MOD_DATA_FILE(items, Item)
			GC_MOD_DATA_FILE(constructions, Construction)
			GC_MOD_DATA_FILE(wildplants, NatureObject)
			
			// of course, it's a special case
			if (fs::exists(mod / "names.dat")) {
				Logger::Inst()->output << "[Data] Loading: " << (mod / "names.dat") << "\n";
				TCODNamegen::parse((mod / "names.dat").string().c_str());
			}
			
			GC_MOD_DATA_FILE(creatures, NPC)
			
			#undef GC_MOD_DATA_FILE
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
