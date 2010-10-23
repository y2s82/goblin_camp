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

#include <string>
#include <boost/assert.hpp>
#include <libtcod.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#include "Logger.hpp"
#include "data/Mods.hpp"
#include "data/Paths.hpp"
#include "Construction.hpp"
#include "Item.hpp"
#include "NatureObject.hpp"
#include "NPC.hpp"
#include "scripting/Engine.hpp"

namespace {
	namespace globals {
		std::list<Mods::Metadata> loadedMods;
	}
	
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
	
	void LoadFile(std::string filename, const fs::path& dir, void (*loadFunc)(std::string), bool required = false) {
		filename += ".dat";
		
		LOG_FUNC("Trying to load " << filename << " from " << dir.string(), "LoadFile");
		
		if (!fs::exists(dir / filename)) {
			LOG_FUNC("Doesn't exist.", "LoadFile");
			
			if (required) {
				exit(1);
			} else {
				return;
			}
		}
		
		loadFunc((dir / filename).string());
	}
	
	void LoadMetadata(Mods::Metadata& metadata, const fs::path& metadataFile) {
		TCODParser parser;
		TCODParserStruct *type = parser.newStructure("mod");
		type->addProperty("name",       TCOD_TYPE_STRING, true);
		type->addProperty("author",     TCOD_TYPE_STRING, true);
		type->addProperty("version",    TCOD_TYPE_STRING, true);
		type->addProperty("apiVersion", TCOD_TYPE_INT, false);
		
		ModListener *listener = new ModListener(&metadata);
		parser.run(metadataFile.string().c_str(), listener);
		delete listener;
	}
	
	void LoadMod(const fs::path& dir, bool required = false) {
		std::string mod = dir.filename().string();
		
		LOG_FUNC("Trying to load mod '" << mod << "' from " << dir.string(), "LoadMod");
		
		LoadFile("items",         dir, Item::LoadPresets, required);
		LoadFile("constructions", dir, Construction::LoadPresets, required);
		LoadFile("wildplants",    dir, NatureObject::LoadPresets, required);
		LoadFile("names",         dir, LoadNames, required);
		LoadFile("creatures",     dir, NPC::LoadPresets, required);
		
		Mods::Metadata metadata(mod, mod, "", "1.0", -1);
		if (fs::exists(dir / "mod.dat")) {
			LOG_FUNC("Loading metadata.", "LoadMod");
			LoadMetadata(metadata, (dir / "mod.dat"));
		}
		
		if (metadata.apiVersion != -1) {
			// NB: 9999 is a magic value reserved for data shipped with Goblin Camp itself.
			if (metadata.apiVersion != Script::version && metadata.apiVersion != 9999) {
				LOG_FUNC("WARNING: Ignoring mod scripts because of incorrect API version.", "LoadMod");
			} else {
				Script::LoadScript(mod, dir.string());
			}
		}
		
		globals::loadedMods.push_back(metadata);
	}
}

namespace Mods {
	Metadata::Metadata(const std::string& mod, const std::string& name, const std::string& author, const std::string& version, short apiVersion)
		: mod(mod), name(name), author(author), version(version), apiVersion(apiVersion) {
	}
	
	const std::list<Metadata>& GetLoaded() {
		return globals::loadedMods;
	}
	
	void Load() {
		// load core data
		LoadMod(Paths::Get(Paths::GlobalData) / "lib" / "gcamp_core", true);
		globals::loadedMods.begin()->mod = "Goblin Camp";
		
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