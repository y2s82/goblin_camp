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
#include "tileRenderer/NPCSpriteSet.hpp"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include "NPC.hpp"

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
	void DrawNPC(boost::shared_ptr<NPC> npc, SDL_Surface *dst, SDL_Rect * dstRect) const;

	int GetGraphicsHintFor(const NPCPreset& npcPreset) const;

	void SetAuthor(std::string auth);
	void SetDescription(std::string desc);
	void SetTerrain(TileType type, const Sprite& sprite);
	void AddWater(const Sprite& sprite);
	void SetFilthMinor(const Sprite& sprite);
	void SetFilthMajor(const Sprite& sprite);
	void SetMarker(const Sprite& sprite);
	void SetBlood(const Sprite& sprite);
	void SetNonTerritoryOverlay(const Sprite& sprite);
	void SetTerritoryOverlay(const Sprite& sprite);
	void SetMarkedOverlay(const Sprite& sprite);
	void AddNPCSpriteSet(std::string name, const NPCSpriteSet& set);
	void SetDefaultNPCSpriteSet(const NPCSpriteSet& set);

private:
	typedef boost::array<Sprite, TILE_TYPE_COUNT> TileTypeSpriteArray;
	typedef boost::unordered_map<std::string, int, boost::hash<std::string>> NPCLookupMap;
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

	NPCSpriteSet defaultNPCSpriteSet;
	std::vector<NPCSpriteSet> npcSpriteSets;
	NPCLookupMap npcSpriteLookup;
};