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

#include "data/Tilesets.hpp"
#include "tileRenderer/TileSetLoader.hpp"

#include <string>
#include <boost/assert.hpp>
#include <libtcod.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#include "Game.hpp"
#include "Logger.hpp"
#include "data/Paths.hpp"

namespace {
	struct TilesetsMetadataListener : public ITCODParserListener {
		Tilesets::Metadata *ptr;
		
		TilesetsMetadataListener(Tilesets::Metadata *ptr) : ptr(ptr) {
			BOOST_ASSERT(ptr != NULL);
		}
		
		bool parserProperty(TCODParser*, const char *name, TCOD_value_type_t, TCOD_value_t value) {
			if (boost::iequals(name, "author")) {
				ptr->author = value.s;
			} else if (boost::iequals(name, "version")) {
				ptr->version = value.s;
			} else if (boost::iequals(name, "description")) {
				ptr->description = value.s;
			} else if (boost::iequals(name, "tileWidth")) {
				ptr->width = value.i;
			} else if (boost::iequals(name, "tileHeight")) {
				ptr->height = value.i;
			}
			
			return true;
		}
		
		void error(const char *err) {
			LOG_FUNC("TilesetsMetadataListener: " << err, "TilesetsMetadataListener::error");
			ptr->valid = false;
		}
		
		bool parserNewStruct(TCODParser*, const TCODParserStruct* str, const char* val) 
		{ 
			if (boost::iequals(str->getName(), "tileset_data")) {
				ptr->name = val;
			}
			return true; 
		}

		bool parserFlag(TCODParser*, const char*) { return true; }
		bool parserEndStruct(TCODParser*, const TCODParserStruct*, const char*) { return true; }
	};
	
	/**
		Reads in tileset metadata
		
		\return                  Whether the metadata was read successfully
		\param[out] metadata     Tilesets::Metadata structure to fill.
		\param[in]  metadataFile Full path to the tileset file.
	*/
	bool ReadMetadata(Tilesets::Metadata& metadata, const fs::path& metadataFile) {
		TCODParser parser;
		TileSetLoader::SetupTilesetParser(parser);
		
		boost::shared_ptr<TilesetsMetadataListener> listener(new TilesetsMetadataListener(&metadata));
		parser.run(metadataFile.string().c_str(), listener.get());
		return metadata.valid && metadata.width > 0 && metadata.height > 0;
	}
	
	/**
		Reads in the metadata for the given tileset.
		
		\param[in] dir      Tileset's directory.
		\param[in] metadataList The metadata list top populate on a successful read
	*/
	void LoadMetadata(const fs::path& dir, std::vector<Tilesets::Metadata>& metadataList, bool isDefault=false) {
		boost::filesystem::path tilesetDat = dir / "tileset.dat";

		LOG_FUNC("Trying to load tileset data from " << tilesetDat.string(), "Tilesets::LoadMetadata");
		
		Tilesets::Metadata metadata(dir);
		metadata.defaultTileset = isDefault;
		
		if (fs::exists(tilesetDat)) {
			if (ReadMetadata(metadata, tilesetDat)) {
				metadataList.push_back(metadata);
			}
		}
	}
}

namespace Tilesets {
	Metadata::Metadata(const boost::filesystem::path& path)
		: basePath(path), name(""), author(""), version(""), description(""), width(0), height(0), valid(true) {
	}
	
	/**
		Loads default tileset and then tries to load user tilesets.
	*/
	std::vector<Metadata> LoadTilesetMetadata() {
		std::vector<Metadata> metadata;

		// load core tileset
		LoadMetadata(Paths::Get(Paths::GlobalData) / "tiles", metadata, true);
		
		// load user tilesets
		for (fs::directory_iterator it(Paths::Get(Paths::Tilesets)), end; it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			
			LoadMetadata(it->path(), metadata);
		}
		return metadata;
	}
}
