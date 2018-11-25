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

#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <boost/format.hpp>
#include <libtcod.hpp>
// http://www.ridgesolutions.ie/index.php/2013/05/30/boost-link-error-undefined-reference-to-boostfilesystemdetailcopy_file/
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>

namespace py = boost::python;
namespace fs = boost::filesystem;

#include "data/Config.hpp"
#include "data/Data.hpp"
#include "data/Paths.hpp"
#include "Logger.hpp"
#include "UI/MessageBox.hpp"
#include "Game.hpp"
#include "scripting/Event.hpp"
#include "scripting/Engine.hpp"

namespace {
	/**
		Converts an UNIX timestamp to ISO8601 date string (YYYY-MM-DD, HH:MM:SS).
		
		\param[in]  timestamp A source UNIX timestamp.
		\param[out] dest      A string buffer to receive formatted date.
	*/
	void FormatTimestamp(const time_t& timestamp, std::string& dest) {
		char buffer[21] = { "0000-00-00, 00:00:00" }; 
		
		size_t size = 0;
		struct tm *date;
		
		date = localtime(&timestamp);
		size = strftime(buffer, 21, "%Y-%m-%d, %H:%M:%S", date);
		buffer[size] = '\0';
		
		dest = buffer;
	}
	
	/**
		Converts a file size in bytes into more human-readable larger units
		(NB: uses kB/MB/GB as 1024-based units).
		
		\param[in]  filesize File size (in bytes).
		\param[out] dest     A string buffer to receive formatted file size.
	*/
	void FormatFileSize(const std::uintmax_t& filesize, std::string& dest) {
		static const char* sizes[] = { "%10.0f b", "%10.2f kB", "%10.2f MB", "%10.2f GB" };
		static unsigned maxSize = sizeof(sizes) / sizeof(sizes[0]);
		
		long double size = static_cast<long double>(filesize);
		
		unsigned idx = 0;
		while (size > 1024.L && idx < maxSize) {
			size /= 1024.L;
			++idx;
		}
		
		dest = (boost::format(sizes[idx]) % size).str();
	}
	
	/**
		Removes invalid (<tt>\\/:*?"\<\>|</tt>) characters from the
		filename (removes characters that are rejected by Windows,
		but allowed by *nixes for consistency).
		
		\param[in] filename Filename as supplied by the user.
		\returns            Sanitized filename.
	*/
	std::string SanitizeFilename(const std::string& filename) {
		std::string sanitized;
		std::string invalid = "\\/:*?\"<>|";
		
		return filename;
/* TODO: Check why the Mac side of things apparently needed a non-sanitizing filename-sanitizer
		std::remove_copy_if(
			filename.begin(), filename.end(),
			std::back_inserter(sanitized),

			[&invalid](char x) -> bool {
				return invalid.find(x) != std::string::npos;
			}
		);
		
		return sanitized;
*/
	}
	
	/**
		Saves current game to a given file. Emits onGameSaved scripting event.
		
		\param[in]  file   Full path to the save.
		\param[out] result Boolean indicating success or failure.
	*/
	void DoSave(std::string file, bool& result) {
		LOG_FUNC("Saving game to " << file, "DoSave");
		
		if ((result = Game::Inst()->SaveGame(file))) {
			Script::Event::GameSaved(file);
		}
	}
	
	/**
		Checks whether given file exists in the user's personal directory, and if not,
		tries to copy it from the global data directory.
		
		\see Paths
		\param[in] target File to check for (full path).
	*/
	void CopyDefault(const fs::path& target) {
		if (fs::exists(target)) return;
		
		fs::path file   = target.filename();
		fs::path source = Paths::Get(Paths::GlobalData) / file;
		
		LOG_FUNC("User's " << file.string() << " does not exist -- trying to copy " << source.string(), "CopyDefault");
		
		if (!fs::exists(source)) {
			LOG_FUNC("Global data file doesn't exist!", "CopyDefault");
			exit(1);
		}
		
		try {
			fs::copy_file(source, target);
		} catch (const fs::filesystem_error& e) {
			LOG_FUNC("Error while copying: " << e.what(), "CopyDefault");
			exit(2);
		}
	}
	
	/**
		Checks whether given file exists in the user's personal directory, and if not,
		tries to create a new one.
		
		\see Paths
		\param[in] target File to check for (full path).
		\param[in] source Default content to use for the new file.
	*/
	void CreateDefault(const fs::path& target, const std::string& source) {
		if (fs::exists(target)) return;
		
		LOG_FUNC("Creating default " << target.filename().string(), "CreateDefault");
		
		try {
			std::ofstream file(target.string().c_str());
			file << source;
			file.close();
		} catch (const std::exception& e) {
			LOG_FUNC("Error while writing to file: " << e.what(), "CreateDefault");
		}
	}
	
	/**
		Ensures that \ref Config::Save won't throw at exit.
	*/
	void SaveConfig() {
		try {
			Config::Save();
		} catch (...) {
			// pass
		}
	}
}

namespace Data {
	Save::Save(const std::string& filename, std::uintmax_t size, time_t timestamp) : filename(filename), timestamp(timestamp) {
		FormatFileSize(size, this->size);
		FormatTimestamp(timestamp, this->date);
	}
	
	/**
		Retrieves a list of saved games.
		
		\param[out] list Storage for the list.
	*/
	void GetSavedGames(std::vector<Save>& list) {
		for (fs::directory_iterator it(Paths::Get(Paths::Saves)), end; it != end; ++it) {
			fs::path save = it->path();
			if (!boost::iequals(save.extension().string(), ".sav")) continue;
			
			save.replace_extension();
			
			list.push_back(Save(
				save.filename().string(),
				fs::file_size(it->path()),
				fs::last_write_time(it->path())
			));
		}
	}
	
	/**
		Retrieves a count of saved games.
		
		\returns Number of saved games found.
	*/
	unsigned CountSavedGames() {
		std::vector<Save> saves;
		GetSavedGames(saves);
		return saves.size();
	}
	
	/**
		Loads the game from given file.
		
		\param[in] save Save filename.
		\returns        Boolean indicating success or failure.
	*/
	bool LoadGame(const std::string& save) {
		std::string file = (Paths::Get(Paths::Saves) / save).string() + ".sav";
		LOG("Loading game from " << file);
		
		if (!Game::Inst()->LoadGame(file)) return false;
		Script::Event::GameLoaded(file);
		
		return true;
	}
	
	/**
		Saves the game to given file. If it exists, prompts the user whether to override.
		
		\see DoSave
		\bug If sanitized filename is empty, will use @c _ instead. Should tell the user.
		
		\param[in] save    Save filename.
		\param[in] confirm Boolean indicating whether to confirm overwriting an existing save
		\returns           Boolean indicating success or failure.
	*/
	bool SaveGame(const std::string& save, bool confirm) {
		std::string file = SanitizeFilename(save);
		
		if (file.size() == 0) {
			file = "_";
		}
		
		file = (Paths::Get(Paths::Saves) / file).string() + ".sav";
		
		bool result = false;
		
		if (!fs::exists(file) || !confirm) {
			DoSave(file, result);
		} else {
			MessageBox::ShowMessageBox(
				"Save game exists, overwrite?", boost::bind(DoSave, file, boost::ref(result)), "Yes",
				NULL, "No");
		}
		
		return result;
	}
	
	/**
		Executes the user's configuration file.
	*/
	void LoadConfig() {
		LOG("Loading user config.");
		const fs::path& config = Paths::Get(Paths::Config);
		CreateDefault(config, "## Goblin Camp default empty configuration file");
		
		py::object globals = py::import("_gcampconfig").attr("__dict__");
		py::object locals  = py::import("__gcuserconfig__").attr("__dict__");
		try {
			py::exec_file(config.string().c_str(), globals, locals);
		} catch (const py::error_already_set&) {
			LOG("Cannot load user config.");
			Script::LogException();
			return;
		}
		
		atexit(SaveConfig);
	}
	
	/**
		Loads the user's bitmap font.
	*/
	void LoadFont() {
		static bool again = false;
		
		LOG("Loading the font " << (again ? "(again)" : ""));
		const fs::path& font = Paths::Get(Paths::Font);
		
		CopyDefault(font);
		TCODConsole::setCustomFont(font.string().c_str());
		again = true;
	}
	
	/**
		Saves a screenshot of the game. Takes care of automatic numbering.
	*/
	void SaveScreenshot() {
		// sadly, libtcod supports autonumbering only when saving to current dir
		unsigned int largest = 0;
		
		for (fs::directory_iterator it(Paths::Get(Paths::Screenshots)), end; it != end; ++it) {
			fs::path png = it->path();
			if (!boost::iequals(png.extension().string(), ".png")) continue;
			
			png.replace_extension();
			
			std::string file = png.filename().string();
			try {
				// screens are saved as screenXXXXXX.png
				largest = std::max(largest, boost::lexical_cast<unsigned int>(file.substr(6)));
			} catch (const std::exception& e) {
				// not worth terminating the game for
				(void)e; // variable not referenced warning
			}
		}
		
		std::string png = (
			Paths::Get(Paths::Screenshots) / ((boost::format("screen%|06|.png") % (largest + 1)).str())
		).string();
		
		LOG("Saving screenshot to " << png);
		TCODSystem::saveScreenshot(png.c_str());
	}
}
