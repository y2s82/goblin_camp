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

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "Logger.hpp"
#include "data/Paths.hpp"

/**
	\enum Paths::Path
		\see Paths::Get
		
	\var Paths::Path::Executable
		\see Globals::exec
		
	\var Paths::Path::GlobalData
		\see Globals::dataDir
	
	\var Paths::Path::Personal
		\see Globals::personalDir
		
	\var Paths::Path::Mods
		\see Globals::modsDir
		
	\var Paths::Path::Saves
		\see Globals::savesDir
		
	\var Paths::Path::Screenshots
		\see Globals::screensDir
		
	\var Paths::Path::Font
		\see Globals::font
		
	\var Paths::Path::Config
		\see Globals::config
		
	\var Paths::Path::ExecutableDir
		\see Globals::execDir

	\var Paths::Path::CoreTilesets
	    \see Globals::coreTilesetsDir

	\var Paths::Path::Tilesets
	    \see Globals::tilesetsDir
*/

/**
	Platform-specific APIs required to implement Paths module.
	
	These functions are defined in <tt>\<platform\>/DataImpl.cpp</tt>.
*/
namespace PathsImpl {
	/**
		Finds current user's personal directory.
		
		On Windows, this is <tt>~/Documents/My Games/Goblin Camp</tt>.
		On Linux, this is <tt>~/.goblincamp</tt>.
		On MacOSX, this is <tt>~/Library/Application Support/Goblin Camp</tt>.
		
		\param[out] personalDir Must be set to the correct path by the implementation.
	*/
	void FindPersonalDirectory(fs::path& personalDir);
	
	/**
		Finds Goblin %Camp executable and data directories.
		
		On Windows, executable and data directories are the same.
		On Linux, executable will be located in <tt>bin/</tt>, and data in <tt>share/goblin-camp/</tt>.
		On MacOSX, executable and data will be located in a bundle.
		
		\param[out] exec    Must be set to the correct path to the executable file by the implementation.
		\param[out] execDir Must be set to the correct path to the executable directory by the implementation.
		\param[out] dataDir Must be set to the correct path to the global data directory by the implementation.
	*/
	void FindExecutableDirectory(fs::path& exec, fs::path& execDir, fs::path& dataDir);
}

namespace Globals {
	/**
		\var personalDir
			Path to user's personal directory for Goblin %Camp.
			\see PathsImpl::FindPersonalDirectory
			
		\var exec
			Path to Goblin %Camp executable file.
			\see PathsImpl::FindExecutableDirectory
			
		\var execDir
			Path to Goblin %Camp executable directory.
			\see PathsImpl::FindExecutableDirectory
			
		\var dataDir
			Path to Goblin %Camp global data directory.
			\see PathsImpl::FindExecutableDirectory
			
		\var savesDir
			Path to user's save directory (subdir of personalDir).
			
		\var screensDir
			Path to user's screenshot directory (subdir of personalDir).
			
		\var modsDir
			Path to user's mods directory (subdir of personalDir).

		\var coreTilesetsDir
		    Path to core tilesets directory (subdir of dataDir).

		\var tilesetsDir
			Path to user's tilesets directory (subdir of personalDir).
			
		\var config
			Path to user's configuration file.
			
		\var font
			Path to user's bitmap font.
	*/
	fs::path personalDir, exec, execDir, dataDir, coreTilesetsDir;
	fs::path savesDir, screensDir, modsDir, tilesetsDir;
	fs::path config, font;
}

/**
	Interface to manage platform-specific paths in a platform-agnostic way.
	Basic building block of data handling code.
*/
namespace Paths {
	/**
		Finds platform-specific directories (see PathsImpl), creates personal directory
		if doesn't exist, and opens log file.
		
		This has to be called before anything else, especially anything
		that uses logging.
		
		\returns Whether portable mode has been activated through binDir/goblin-camp.portable
	*/
	inline bool Bootstrap() {
		using namespace Globals;
		
		PathsImpl::FindExecutableDirectory(exec, execDir, dataDir);
		
		bool portableMode = fs::exists(execDir / "goblin-camp.portable");
		
		if (!portableMode) {
			PathsImpl::FindPersonalDirectory(personalDir);
		} else {
			personalDir = dataDir / "user-data";
		}
		
		savesDir    = personalDir / "saves";
		screensDir  = personalDir / "screenshots";
		modsDir     = personalDir / "mods";
		tilesetsDir = personalDir / "tilesets";
		
		config      = personalDir / "config.py";
		font        = personalDir / "terminal.png";
		
		coreTilesetsDir = dataDir / "lib" / "tilesets_core";
		
		fs::create_directory(personalDir);
		Logger::OpenLogFile((personalDir / "goblin-camp.log").string());
		
		return portableMode;
	}
	
	/**
		Calls Paths::Bootstrap and then tries to create user directory structure.
	*/
	void Init() {
		if (Bootstrap()) {
			LOG("Portable mode active.");
		}
		
		LOG("Personal directory: " << Globals::personalDir);
		LOG("Saves directory: " << Globals::savesDir);
		LOG("Screenshots directory: " << Globals::screensDir);
		LOG("Mods directory: " << Globals::modsDir);
		LOG("CoreTilesets directory: " << Globals::coreTilesetsDir);
		LOG("Tilesets directory: " << Globals::tilesetsDir);
		LOG("Executable directory: " << Globals::execDir);
		LOG("Global data directory: " << Globals::dataDir);
		LOG("Executable: " << Globals::exec);
		LOG("Config: " << Globals::config);
		LOG("Font: " << Globals::font);
		
		// try to create personal directory structure
		// create_directory doesn't throw if directory already exist
		try {
			fs::create_directory(Globals::savesDir);
			fs::create_directory(Globals::screensDir);
			fs::create_directory(Globals::modsDir);
			fs::create_directory(Globals::tilesetsDir);
		} catch (const fs::filesystem_error& e) {
			LOG("filesystem_error while creating directories: " << e.what());
			exit(1);
		}
	}
	
#if defined(_MSC_VER)
	#pragma warning(push)
	// "warning C4715: 'Data::GetPath' : not all control paths return a value"
	#pragma warning(disable : 4715)
#endif
	
	/**
		Retrieves reference to a given path. Exists to hide implementation details of path storage.
		
		\param[in] what What to return, a member of Path enumeration.
		\returns        Constant reference to given path (a boost::filesystem::path object).
	*/
	const fs::path& Get(Path what) {
		switch (what) {
			case Executable:    return Globals::exec;
			case GlobalData:    return Globals::dataDir;
			case Personal:      return Globals::personalDir;
			case Mods:          return Globals::modsDir;
			case Saves:         return Globals::savesDir;
			case Screenshots:   return Globals::screensDir;
			case Font:          return Globals::font;
			case Config:        return Globals::config;
			case ExecutableDir: return Globals::execDir;
			case CoreTilesets:  return Globals::coreTilesetsDir;
			case Tilesets:      return Globals::tilesetsDir;
		}
		
		// If control reaches here, then someone added new value to the enum,
		// forgot to add it here, and missed the 'switch not checking
		// every value' warning. So, crash and burn.
		LOG("Impossible code path, crashing.");
		assert(false);
#if defined(__GNUC__)
		__builtin_unreachable(); // to silence the warning
#endif
	}
	
#if defined(_MSC_VER)
	#pragma warning(pop)
#endif
}
