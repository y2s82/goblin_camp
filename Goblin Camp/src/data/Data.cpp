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

#include "scripting/_python.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <boost/format.hpp>
#include <libtcod.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

namespace fs = boost::filesystem;

#include "data/Data.hpp"
#include "data/Paths.hpp"
#include "Logger.hpp"
#include "UI/YesNoDialog.hpp"
#include "Game.hpp"
#include "scripting/Event.hpp"
#include "scripting/Engine.hpp"

namespace {
	void FormatTimestamp(const time_t& timestamp, std::string& dest) {
		char buffer[21] = { "0000-00-00, 00:00:00" }; 
		
		size_t size = 0;
		struct tm *date;
		
		date = localtime(&timestamp);
		size = strftime(buffer, 21, "%Y-%m-%d, %H:%M:%S", date);
		buffer[size] = '\0';
		
		dest = buffer;
	}
	
	void FormatFileSize(const boost::uintmax_t& filesize, std::string& dest) {
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
	
	void DoSave(std::string file) {
		LOG_FUNC("Saving game to " << file, "DoSave");
		
		Game::Inst()->SaveGame(file);
		Script::Event::GameSaved(file);
	}
	
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
}

namespace Data {
	Save::Save(const std::string& filename, boost::uintmax_t size, time_t timestamp) : filename(filename), timestamp(timestamp) {
		FormatFileSize(size, this->size);
		FormatTimestamp(timestamp, this->date);
	}
	
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
	
	unsigned CountSavedGames() {
		std::vector<Save> saves;
		GetSavedGames(saves);
		return saves.size();
	}
	
	void LoadGame(const std::string& save) {
		std::string file = (Paths::Get(Paths::Saves) / save).string() + ".sav";
		LOG("Loading game from " << file);
		
		Game::Inst()->LoadGame(file);
		Script::Event::GameLoaded(file);
	}
	
	void SaveGame(const std::string& save) {
		std::string file = (Paths::Get(Paths::Saves) / save).string() + ".sav";
		
		if (!fs::exists(file)) {
			DoSave(file);
		} else {
			YesNoDialog::ShowYesNoDialog("Save game exists, overwrite?", boost::bind(DoSave, file), NULL);
		}
	}
	
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
		}
	}
	
	void LoadFont() {
		static bool again = false;
		
		LOG("Loading the font " << (again ? "(again)" : ""));
		const fs::path& font = Paths::Get(Paths::Font);
		
		CopyDefault(font);
		TCODConsole::setCustomFont(font.string().c_str());
		again = true;
	}
	
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
