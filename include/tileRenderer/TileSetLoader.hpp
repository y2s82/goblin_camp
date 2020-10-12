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
#include<memory>


#include <filesystem>
#include <libtcod.hpp>
#include "tileRenderer/TileSetRenderer.hpp"
#include "tileRenderer/TileSet.hpp"

struct TileSetMetadata
{
	std::filesystem::path path;
	std::string name, author, version, description;
	int width, height;
	bool valid;
	
	explicit TileSetMetadata();
	explicit TileSetMetadata(std::filesystem::path tilesetPath);
};

struct TilesetModMetadata {
	std::filesystem::path location;
	int width, height;
	
	explicit TilesetModMetadata(std::filesystem::path loc);
};

namespace TileSetLoader
{
	std::shared_ptr<TileSet> LoadTileSet(std::shared_ptr<TilesetRenderer> spriteFactory, std::string name);
	std::shared_ptr<TileSet> LoadTileSet(std::shared_ptr<TilesetRenderer> spriteFactory, std::filesystem::path path);
	TileSetMetadata LoadTileSetMetadata(std::filesystem::path path);
	std::list<TilesetModMetadata> LoadTilesetModMetadata(std::filesystem::path path);
}
