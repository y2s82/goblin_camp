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

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <libtcod.hpp>
#include "tileRenderer/TileSet.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/NPCSpriteSet.hpp"
#include "tileRenderer/ItemSpriteSet.hpp"
#include "tileRenderer/ConstructionSpriteSet.hpp"
#include "tileRenderer/SpellSpriteSet.hpp"

struct TileSetMetadata
{
	boost::filesystem::path path;
	std::string name, author, version, description;
	int width, height;
	bool valid;
	
	explicit TileSetMetadata();
	explicit TileSetMetadata(boost::filesystem::path tilesetPath);
};

namespace TileSetLoader
{
	boost::shared_ptr<TileSet> LoadTileSet(boost::filesystem::path path);
	TileSetMetadata LoadTileSetMetadata(boost::filesystem::path path);
};
