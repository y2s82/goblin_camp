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


#include <boost/array.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <SDL.h>

#include "MapRenderer.hpp"
#include "Tile.hpp"
#include "NPC.hpp"
#include "NatureObject.hpp"
#include "tileRenderer/Sprite.hpp"
#include "tileRenderer/NPCSpriteSet.hpp"
#include "tileRenderer/NatureObjectSpriteSet.hpp"
#include "tileRenderer/ItemSpriteSet.hpp"
#include "tileRenderer/ConstructionSpriteSet.hpp"

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

	void DrawCursor(CursorType type, int cursorHint, bool placeable, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawMarkedOverlay(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawMarker(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawTerrain(TileType type, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawBlood(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawWater(int index, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMinor(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMajor(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawTerritoryOverlay(bool owned, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawNPC(boost::shared_ptr<NPC> npc, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawNatureObject(boost::shared_ptr<NatureObject> plant, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawItem(boost::shared_ptr<Item> item, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawConstruction(boost::shared_ptr<Construction> construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect);

	int GetGraphicsHintFor(const NPCPreset& npcPreset) const;
	int GetGraphicsHintFor(const NatureObjectPreset& plantPreset) const;
	int GetGraphicsHintFor(const ItemPreset& itemPreset) const;
	int GetGraphicsHintFor(const ConstructionPreset& constructionPreset) const;

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
	void SetCursorSprites(CursorType type, const Sprite& sprite);
	void SetCursorSprites(CursorType type, const Sprite& placeableSprite, const Sprite& nonplaceableSprite);
	void SetDefaultUnderConstructionSprite(const Sprite& sprite);
	void SetStatusSprite(StatusEffectType statusEffect, const Sprite& sprite);
	
	void AddNPCSpriteSet(std::string name, const NPCSpriteSet& set);
	void SetDefaultNPCSpriteSet(const NPCSpriteSet& set);
	void AddNatureObjectSpriteSet(std::string name, const NatureObjectSpriteSet& set);
	void SetDefaultNatureObjectSpriteSet(const NatureObjectSpriteSet& set);
	void AddItemSpriteSet(std::string name, const ItemSpriteSet& set);
	void SetDefaultItemSpriteSet(const ItemSpriteSet& set);
	void AddConstructionSpriteSet(std::string name, const ConstructionSpriteSet& set);
	void SetDefaultConstructionSpriteSet(const ConstructionSpriteSet& set);
	
private:
	typedef boost::array<Sprite, TILE_TYPE_COUNT> TileTypeSpriteArray;
	typedef boost::array<Sprite, Cursor_Simple_Mode_Count> CursorTypeSpriteArray;
	typedef boost::array<Sprite, STATUS_EFFECT_COUNT> StatusEffectSpriteArray;
	typedef boost::unordered_map< std::string, int, boost::hash<std::string> > LookupMap;

	class DrawConstructionVisitor;

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

	Sprite defaultUnderConstructionSprite;

	NPCSpriteSet defaultNPCSpriteSet;
	std::vector<NPCSpriteSet> npcSpriteSets;
	LookupMap npcSpriteLookup;

	NatureObjectSpriteSet defaultNatureObjectSpriteSet;
	std::vector<NatureObjectSpriteSet> natureObjectSpriteSets;
	LookupMap natureObjectSpriteLookup;

	ItemSpriteSet defaultItemSpriteSet;
	std::vector<ItemSpriteSet> itemSpriteSets;
	LookupMap itemSpriteLookup;

	ConstructionSpriteSet defaultConstructionSpriteSet;
	std::vector<ConstructionSpriteSet> constructionSpriteSets;
	LookupMap constructionSpriteLookup;

	CursorTypeSpriteArray placeableCursors;
	CursorTypeSpriteArray nonplaceableCursors;

	StatusEffectSpriteArray defaultStatusEffects;

	void DrawBaseConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawUnderConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawStockpileContents(Stockpile * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawOpenDoor(Door * door, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
};
