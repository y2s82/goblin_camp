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
			
	/**
		Reads in the metadata for the given tileset.
		\param[in] dir      Tileset's directory.
		\param[in] metadataList The metadata list to populate on a successful read
	*/
	void LoadMetadata(const fs::path& dir, std::vector<TileSetMetadata>& metadataList) {
		LOG_FUNC("Trying to load tileset metadata from " << dir.string(), "Tilesets::LoadMetadata");
		
		TileSetMetadata metadata = TileSetLoader::LoadTileSetMetadata(dir);
		if (metadata.valid && metadata.width > 0 && metadata.height > 0)
		{
			metadataList.push_back(metadata);
		}
	}
}

namespace Tilesets {
	/**
		Loads default tileset and then tries to load user tilesets.
	*/
	std::vector<TileSetMetadata> LoadTilesetMetadata() {
		std::vector<TileSetMetadata> metadata;

		// load core tilesets
		for (fs::directory_iterator it(Paths::Get(Paths::CoreTilesets)), end; it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			LoadMetadata(it->path(), metadata);
		}
				
		// load user tilesets
		for (fs::directory_iterator it(Paths::Get(Paths::Tilesets)), end; it != end; ++it) {
			if (!fs::is_directory(it->status())) continue;
			
			LoadMetadata(it->path(), metadata);
		}
		return metadata;
	}
}
