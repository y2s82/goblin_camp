/* Copyright 2011 Ilkka Halila
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

#include "tileRenderer/TileSetLoader.hpp"
#include "tileRenderer/TilesetParserV1.hpp"
#include "tileRenderer/TilesetParserV2.hpp"
#include "Logger.hpp"
#include "data/Mods.hpp"
#include "data/Paths.hpp"

TileSetMetadata::TileSetMetadata()
	: path(),
	  name(),
	  author(),
	  version(),
	  description(),
	  width(0),
	  height(0),
	  valid(false)
{
}

TileSetMetadata::TileSetMetadata(boost::filesystem::path tilesetPath)
	: path(tilesetPath),
	  name(),
	  author(),
	  version(),
	  description(),
	  width(0),
	  height(0),
	  valid(false)
{
}

TilesetModMetadata::TilesetModMetadata(boost::filesystem::path loc)
	: location(loc),
	  width(0),
	  height(0)
{
}

boost::shared_ptr<TileSet> TileSetLoader::LoadTileSet(boost::shared_ptr<TilesetRenderer> spriteFactory, std::string tilesetName) {
	// Resolve path
	boost::filesystem::path tilesetPath(Paths::Get(Paths::CoreTilesets) / tilesetName);
	if (!boost::filesystem::is_directory(tilesetPath)) {
		tilesetPath = Paths::Get(Paths::Tilesets) / tilesetName;
	}
	return LoadTileSet(spriteFactory, tilesetPath);
}

boost::shared_ptr<TileSet> TileSetLoader::LoadTileSet(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	fs::path tileSetV1Path(path / "tileset.dat");
	fs::path tileSetV2Path(path / "tilesetV2.dat");

	boost::shared_ptr<TileSet> tileset;

	if (fs::exists(tileSetV2Path)) {
		TileSetParserV2 parser(spriteFactory);
		tileset = parser.Run(tileSetV2Path);
	} else if (fs::exists(tileSetV1Path)) {
		TileSetParserV1 parser(spriteFactory);
		tileset = parser.Run(tileSetV1Path);
	} else {
		return boost::shared_ptr<TileSet>();
	}

	if (tileset)
	{
		for (std::list<TilesetModMetadata>::const_iterator iter = Mods::GetAvailableTilesetMods().begin(); iter != Mods::GetAvailableTilesetMods().end(); ++iter) {
			if (iter->height == tileset->TileHeight() && iter->width == tileset->TileWidth())
			{
				fs::path tileSetModV2Path(iter->location / "tilesetModV2.dat");
				if (fs::exists(tileSetModV2Path)) {
					TileSetParserV2 parser(spriteFactory);
					parser.Modify(tileset, tileSetModV2Path);
				}
			}
		}
	}
	return tileset; 
}

TileSetMetadata TileSetLoader::LoadTileSetMetadata(boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	fs::path tileSetV1Path(path / "tileset.dat");
	fs::path tileSetV2Path(path / "tilesetV2.dat");
		
	if (fs::exists(tileSetV2Path)) {
		TileSetMetadataParserV2 parser = TileSetMetadataParserV2();
		return parser.Run(tileSetV2Path);
	} else if (fs::exists(tileSetV1Path)) {
		TileSetMetadataParserV1 parser = TileSetMetadataParserV1();
		return parser.Run(tileSetV1Path);
	}
	return TileSetMetadata();
}

std::list<TilesetModMetadata> TileSetLoader::LoadTilesetModMetadata(boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	fs::path tileSetV2Path(path / "tilesetModV2.dat");
	if (fs::exists(tileSetV2Path)) {
		TileSetModMetadataParserV2 parser = TileSetModMetadataParserV2();
		return parser.Run(tileSetV2Path);
	}
	return std::list<TilesetModMetadata>();
}


