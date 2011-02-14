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

boost::shared_ptr<TileSet> TileSetLoader::LoadTileSet(boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	fs::path tileSetV1Path(path / "tileset.dat");
	fs::path tileSetV2Path(path / "tilesetV2.dat");

	if (fs::exists(tileSetV2Path)) {
		TileSetParserV2 parser = TileSetParserV2();
		return parser.Run(tileSetV2Path);
	} else if (fs::exists(tileSetV1Path)) {
		TileSetParserV1 parser = TileSetParserV1();
		return parser.Run(tileSetV1Path);
	}
	return boost::shared_ptr<TileSet>();
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


