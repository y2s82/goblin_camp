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

#include "tileRenderer/TileSet.hpp"
#include <boost/numeric/conversion/cast.hpp> 

TileSet::TileSet(std::string tileSetName, int tileW, int tileH) :
	name(tileSetName),
	tileWidth(tileW),
	tileHeight(tileH),
	author(""),
	description(""),
	waterTiles(),
	minorFilth(),
	majorFilth(),
	nonTerritoryOverlay(),
	territoryOverlay(),
	markedOverlay(),
	marker(),
	blood(),
	npcSpriteSets(),
	npcSpriteLookup() {
		for (int i = 0; i < terrainTiles.size(); ++i) {
			terrainTiles[i] = Sprite();
		}
}

TileSet::~TileSet() {}

int TileSet::TileWidth() const {
	return tileWidth;
}

int TileSet::TileHeight() const {
	return tileHeight;
}

std::string TileSet::GetName() const {
	return name;
}

std::string TileSet::GetAuthor() const {
	return author;
}

std::string TileSet::GetDescription() const {
	return description;
}

void TileSet::DrawMarkedOverlay(SDL_Surface *dst, SDL_Rect* dstRect) const {
	markedOverlay.Draw(dst, dstRect);
}

void TileSet::DrawMarker(SDL_Surface *dst, SDL_Rect* dstRect) const {
	marker.Draw(dst, dstRect);
}

void TileSet::DrawTerrain(TileType type, SDL_Surface *dst, SDL_Rect* dstRect) const {
	terrainTiles.at(type).Draw(dst, dstRect);
}

void TileSet::DrawBlood(SDL_Surface *dst, SDL_Rect* dstRect) const {
	blood.Draw(dst, dstRect);
}

void TileSet::DrawWater(int index, SDL_Surface *dst, SDL_Rect* dstRect) const {
	int i = std::min(index, boost::numeric_cast<int>(waterTiles.size() - 1));
	if (index > -1)
	{
		waterTiles.at(index).Draw(dst, dstRect);
	}
}

void TileSet::DrawFilthMinor(SDL_Surface *dst, SDL_Rect * dstRect) const {
	minorFilth.Draw(dst, dstRect);
}

void TileSet::DrawFilthMajor(SDL_Surface *dst, SDL_Rect * dstRect) const {
	majorFilth.Draw(dst, dstRect);
}

void TileSet::DrawTerritoryOverlay(bool owned, SDL_Surface *dst, SDL_Rect * dstRect) const {
	if (owned) {
		territoryOverlay.Draw(dst, dstRect);
	} else {
		nonTerritoryOverlay.Draw(dst, dstRect);
	}
}

void TileSet::DrawNPC(boost::shared_ptr<NPC> npc, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = npc->GraphicsHint();
	if (hint == -1 || hint >= npcSpriteSets.size())
	{
		defaultNPCSpriteSet.tile.Draw(dst, dstRect);
	}
	else
	{
		npcSpriteSets[hint].tile.Draw(dst, dstRect);
	}
}

int TileSet::GetGraphicsHintFor(const NPCPreset& npcPreset) const {
	NPCLookupMap::const_iterator set;

	set = npcSpriteLookup.find(npcPreset.graphicsSet);
	if (set != npcSpriteLookup.end()) {
		return set->second;
	}

	set = npcSpriteLookup.find(npcPreset.fallbackGraphicsSet);
	if (set != npcSpriteLookup.end()) {
		return set->second;
	}

	set = npcSpriteLookup.find(npcPreset.typeName);
	if (set != npcSpriteLookup.end()) {
		return set->second;
	}

	return -1;
}


void TileSet::SetDescription(std::string desc) {
	description = desc;
}

void TileSet::SetAuthor(std::string auth) {
	author = auth;
}

void TileSet::SetTerrain(TileType type, const Sprite& sprite) {
	if (type < 0 || type >= TILE_TYPE_COUNT) 
		return;

	if (type == TILENONE) {
		for (int i = 0; i < terrainTiles.size(); ++i) {
			if (!terrainTiles[i].Exists()) terrainTiles[i] = sprite;
		}
	} else {
		terrainTiles[type] = sprite;
	}
}

void TileSet::AddWater(const Sprite& sprite) {
	waterTiles.push_back(sprite);
}

void TileSet::SetFilthMinor(const Sprite& sprite) {
	minorFilth = sprite;
}

void TileSet::SetFilthMajor(const Sprite& sprite) {
	majorFilth = sprite;
}

void TileSet::SetMarker(const Sprite& sprite) {
	marker = sprite;
}

void TileSet::SetBlood(const Sprite& sprite) {
	blood = sprite;
}

void TileSet::SetNonTerritoryOverlay(const Sprite& sprite) {
	nonTerritoryOverlay = sprite;
}

void TileSet::SetTerritoryOverlay(const Sprite& sprite) {
	territoryOverlay = sprite;
}

void TileSet::SetMarkedOverlay(const Sprite& sprite) {
	markedOverlay = sprite;
}

void TileSet::AddNPCSpriteSet(std::string name, const NPCSpriteSet& set) {
	int index = npcSpriteSets.size();
	npcSpriteSets.push_back(set);
	npcSpriteLookup[name] = index;
}

void TileSet::SetDefaultNPCSpriteSet(const NPCSpriteSet& set) {
	defaultNPCSpriteSet = set;
}
