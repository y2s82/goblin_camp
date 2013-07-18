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

#include <string>
#include <boost/assert.hpp>
#include <libtcod.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace fs = boost::filesystem;

#include "Game.hpp"
#include "Logger.hpp"
#include "data/Mods.hpp"
#include "data/Paths.hpp"
#include "Spell.hpp"
#include "Construction.hpp"
#include "Item.hpp"
#include "NatureObject.hpp"
#include "NPC.hpp"
#include "scripting/Engine.hpp"
#include "Faction.hpp"

namespace Globals {
	/**
		List of loaded mods. NB: removing an entry from this list does not unload the mod.
	*/
	std::list<Mods::Metadata> loadedMods;
	std::list<TilesetModMetadata> availableTilesetMods;
}

namespace {
	struct ModListener : public ITCODParserListener {
		Mods::Metadata *ptr;
		
		ModListener(Mods::Metadata *ptr) : ptr(ptr) {
			BOOST_ASSERT(ptr != NULL);
		}
		
		bool parserProperty(TCODParser*, const char *name, TCOD_value_type_t, TCOD_value_t value) {
			if (boost::iequals(name, "name")) {
				ptr->name = value.s;
			} else if (boost::iequals(name, "author")) {
				ptr->author = value.s;
			} else if (boost::iequals(name, "version")) {
				ptr->version = value.s;
			} else if (boost::iequals(name, "apiversion")) {
				ptr->apiVersion = value.i;
			}
			
			return true;
		}
		
		void error(const char *err) {
			LOG_FUNC("ModListener: " << err, "ModListener::error");
		}
		
		// unused
		bool parserNewStruct(TCODParser*, const TCODParserStruct*, const char*) { return true; }
		bool parserFlag(TCODParser*, const char*) { return true; }
		bool parserEndStruct(TCODParser*, const TCODParserStruct*, const char*) { return true; }
	};
	
	void LoadNames(std::string fn) {
		TCODNamegen::parse(fn.c_str());
	}
	
	/**
		Loads given data file with given function.
		
		\param[in] filename Name of the file, without .dat extension.
		\param[in] dir      Directory containing the file.
		\param[in] loadFunc Function that loads the data file.
		\param[in] required If true, and file doesn't exist, the game will exit completely.
	*/
	void LoadFile(std::string filename, const fs::path& dir, void (*loadFunc)(std::string), bool required = false) {
		filename += ".dat";
		
		LOG_FUNC("Trying to load " << filename << " from " << dir.string(), "LoadFile");
		
		if (!fs::exists(dir / filename)) {
			LOG_FUNC("Doesn't exist.", "LoadFile");
			
			if (required) {
				throw std::runtime_error("Doesn't exist.");
			} else {
				return;
			}
		}
		
		loadFunc((dir / filename).string());
	}
	
	/**
		Loads mod metadata from given file. Doesn't modify loadedMods.
		
		\param[out] metadata     Mods::Metadata structure to fill.
		\param[in]  metadataFile Full path to the metadata file.
	*/
	void LoadMetadata(Mods::Metadata& metadata, const fs::path& metadataFile) {
		TCODParser parser;
		TCODParserStruct *type = parser.newStructure("mod");
		type->addProperty("name",       TCOD_TYPE_STRING, true);
		type->addProperty("author",     TCOD_TYPE_STRING, true);
		type->addProperty("version",    TCOD_TYPE_STRING, true);
		type->addProperty("apiVersion", TCOD_TYPE_INT,    false);
		
		ModListener *listener = new ModListener(&metadata);
		parser.run(metadataFile.string().c_str(), listener);
		delete listener;
	}
	
	/**
		Loads given mod and inserts it's metadata into loadedMods.
		NB: Currently there is no way to unload a mod.
		
		\param[in] dir      Mod's directory.
		\param[in] required Passed down to \ref LoadFile for every data file of the mod.
	*/
	void LoadMod(const fs::path& dir, bool required = false) {
		std::string mod = dir.filename().string();
		
		LOG_FUNC("Trying to load mod '" << mod << "' from " << dir.string(), "LoadMod");
		
		// Removed magic apiVersion in favour of reusing already-hardcoded 'required' code path.
		Mods::Metadata metadata(mod, mod, "", "1.0", (required ? Script::version : -1));
		
		if (fs::exists(dir / "mod.dat")) {
			LOG_FUNC("Loading metadata.", "LoadMod");
			LoadMetadata(metadata, (dir / "mod.dat"));
		}
		
		try {
			LoadFile("spells",        dir, Spell::LoadPresets, required);
			LoadFile("items",         dir, Item::LoadPresets, required);
			LoadFile("constructions", dir, Construction::LoadPresets, required);
			LoadFile("wildplants",    dir, NatureObject::LoadPresets, required);
			LoadFile("names",         dir, LoadNames, required);
			LoadFile("creatures",     dir, NPC::LoadPresets, required);
			LoadFile("factions",      dir, Faction::LoadPresets, required);
		} catch (const std::runtime_error& e) {
			LOG_FUNC("Failed to load mod due to std::runtime_error: " << e.what(), "LoadMod");
			if (required) Game::Inst()->ErrorScreen();  // FIXME: hangs
		}
		std::list<TilesetModMetadata> tilesetMods = TileSetLoader::LoadTilesetModMetadata(dir);
		for (std::list<TilesetModMetadata>::iterator iter = tilesetMods.begin(); iter != tilesetMods.end(); ++iter) {
			Globals::availableTilesetMods.push_back(*iter);
		}
		
		if (metadata.apiVersion != -1) {
			if (metadata.apiVersion != Script::version) {
				LOG_FUNC("WARNING: Ignoring mod scripts because of an incorrect API version.", "LoadMod");
			} else {
				Script::LoadScript(mod, dir.string());
			}
		}
		
		Globals::loadedMods.push_back(metadata);
	}
}

/**
	Interface to load and query mods.
*/
namespace Mods {
	Metadata::Metadata(const std::string& mod, const std::string& name, const std::string& author, const std::string& version, short apiVersion)
		: mod(mod), name(name), author(author), version(version), apiVersion(apiVersion) {
	}
	
	/**
		Retrieves the list of loaded mods.
		
		\returns Constant reference to list of mods metadata.
	*/
	const std::list<Metadata>& GetLoaded() {
		return Globals::loadedMods;
	}
	

	const std::list<TilesetModMetadata>& GetAvailableTilesetMods() {
		return Globals::availableTilesetMods;
	}
	
	/**
		Loads global mod and then tries to load user mods.
	*/
	void Load() {
		// load core data
		LoadMod(Paths::Get(Paths::GlobalData) / "lib" / "gcamp_core", true);
		Globals::loadedMods.begin()->mod = "Goblin Camp";
		
		// load user mods
		for (fs::directory_iterator it(Paths::Get(Paths::Mods)), end; it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			
			LoadMod(it->path());
		}
		
		// now resolve containers and products
		Item::ResolveContainers();
		Construction::ResolveProducts();
	}
}
