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

#include "tileRenderer/Sprite.hpp"
#include <boost/array.hpp>
#include "Tile.hpp"
#include <SDL.h>
#include "Tile.hpp"

//#include <boost/unordered_map.hpp>

class TileSet : private boost::noncopyable
{
public:
	TileSet(std::string tileSetName, int tileW, int tileH);
	~TileSet();

	int TileWidth() const;
	int TileHeight() const;
	std::string GetName() const;
	std::string GetAuthor() const;
	std::string GetDescription() const;

	void DrawMarkedOverlay(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawMarker(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawTerrain(TileType type, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawBlood(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawWater(int index, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMinor(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMajor(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawTerritoryOverlay(bool owned, SDL_Surface *dst, SDL_Rect * dstRect) const;

	void SetAuthor(std::string auth);
	void SetDescription(std::string desc);
	void SetTerrain(TileType type, Sprite sprite);
	void AddWater(Sprite sprite);
	void SetFilthMinor(Sprite sprite);
	void SetFilthMajor(Sprite sprite);
	void SetMarker(Sprite sprite);
	void SetBlood(Sprite sprite);
	void SetNonTerritoryOverlay(Sprite sprite);
	void SetTerritoryOverlay(Sprite sprite);
	void SetMarkedOverlay(Sprite sprite);

private:
	typedef boost::array<Sprite, TILE_TYPE_COUNT> TileTypeSpriteArray;
	int tileWidth;
	int tileHeight;
	std::string name;
	std::string author;
	std::string description;

	TileTypeSpriteArray terrainTiles;
	std::vector<Sprite> waterTiles;
	Sprite minorFilth;
	Sprite majorFilth;
	
	Sprite nonTerritoryOverlay;
	Sprite territoryOverlay;
	Sprite markedOverlay;

	Sprite marker;
	Sprite blood;

	//boost::unordered_map<std::string, ItemSpriteSet>
};