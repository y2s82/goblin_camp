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
#include "Spell.hpp"
#include "Fire.hpp"
#include "tileRenderer/Sprite.hpp"
#include "tileRenderer/StatusEffectSprite.hpp"
#include "tileRenderer/NPCSprite.hpp"
#include "tileRenderer/NatureObjectSpriteSet.hpp"
#include "tileRenderer/ItemSprite.hpp"
#include "tileRenderer/ConstructionSprite.hpp"
#include "tileRenderer/SpellSpriteSet.hpp"
#include "tileRenderer/TerrainSprite.hpp"

class TileSet : private boost::noncopyable
{
public:
	explicit TileSet(std::string tileSetName, int tileW, int tileH);
	~TileSet();

	int TileWidth() const;
	int TileHeight() const;
	std::string GetName() const;
	std::string GetAuthor() const;
	std::string GetVersion() const;
	std::string GetDescription() const;

	bool IsIceSupported() const;

	void DrawCursor(CursorType type, int cursorHint, bool placeable, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawMarkedOverlay(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawMarkedOverlay(Sprite::ConnectedFunction, SDL_Surface *dst, SDL_Rect* dstRect) const;
	void DrawMarker(SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawBlood(Sprite::ConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawWater(Sprite::LayeredConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawIce(Sprite::LayeredConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMinor(Sprite::LayeredConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawFilthMajor(Sprite::LayeredConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawTerritoryOverlay(bool owned, Sprite::ConnectedFunction, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawNPC(boost::shared_ptr<NPC> npc, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawNatureObject(boost::shared_ptr<NatureObject> plant, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawItem(boost::shared_ptr<Item> item, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawSpell(boost::shared_ptr<Spell> spell, SDL_Surface * dst, SDL_Rect * dstRect) const;
	void DrawFire(boost::shared_ptr<FireNode> fire, SDL_Surface * dst, SDL_Rect * dstRect) const;
	void DrawBaseConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawUnderConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawUnreadyTrap(Construction * trap, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRecT) const;
	void DrawStockpileContents(Stockpile * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;
	void DrawOpenDoor(Door * door, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const;

	const TerrainSprite& GetTerrainSprite(TileType type) const;
	TerrainSprite GetTerrainSprite(TileType type);

	int GetGraphicsHintFor(const NPCPreset& npcPreset) const;
	int GetGraphicsHintFor(const NatureObjectPreset& plantPreset) const;
	int GetGraphicsHintFor(const ItemPreset& itemPreset) const;
	int GetGraphicsHintFor(const ConstructionPreset& constructionPreset) const;
	int GetGraphicsHintFor(const SpellPreset& spellPreset) const;

	void SetAuthor(std::string auth);
	void SetVersion(std::string ver);
	void SetDescription(std::string desc);
	void SetTerrain(TileType type, const TerrainSprite& sprite);
	void SetWaterAndIce(const Sprite& sprite);
	void SetIce(const Sprite& sprite);
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
	void SetFireSprite(const Sprite& sprite);

	void SetStatusSprite(StatusEffectType statusEffect, const StatusEffectSprite& sprite);
		
	void AddNPCSprite(std::string name, const NPCSprite& sprite);
	void AddNatureObjectSpriteSet(std::string name, const NatureObjectSpriteSet& sprite);
	void AddItemSprite(std::string name, const ItemSprite& sprite);
	void AddConstructionSprite(std::string name, const ConstructionSprite& sprite);
	void AddSpellSpriteSet(std::string name, const SpellSpriteSet& sprite);
		
	void SetDefaultNPCSprite(const NPCSprite& sprite);
	void SetDefaultNatureObjectSpriteSet(const NatureObjectSpriteSet& sprite);
	void SetDefaultItemSprite(const ItemSprite& sprite);
	void SetDefaultConstructionSprite(const ConstructionSprite& sprite);
	void SetDefaultSpellSpriteSet(const SpellSpriteSet& sprite);
	
private:
	typedef boost::array<TerrainSprite, TILE_TYPE_COUNT> TileTypeSpriteArray;
	typedef boost::array<Sprite, Cursor_Simple_Mode_Count> CursorTypeSpriteArray;
	typedef boost::array<StatusEffectSprite, STATUS_EFFECT_COUNT> StatusEffectSpriteArray;
	typedef boost::unordered_map< std::string, int, boost::hash<std::string> > LookupMap;
	
	int tileWidth;
	int tileHeight;
	std::string name;
	std::string author;
	std::string version;
	std::string description;

	TerrainSprite defaultTerrainTile;
	TileTypeSpriteArray terrainTiles;
	Sprite waterTile;
	Sprite iceTile;
	Sprite minorFilth;
	Sprite majorFilth;
	
	Sprite nonTerritoryOverlay;
	Sprite territoryOverlay;
	Sprite markedOverlay;

	Sprite marker;
	Sprite blood;

	Sprite defaultUnderConstructionSprite;
	Sprite fireTile;
	
	NPCSprite defaultNPCSprite;
	std::vector<NPCSprite> npcSprites;
	LookupMap npcSpriteLookup;

	NatureObjectSpriteSet defaultNatureObjectSpriteSet;
	std::vector<NatureObjectSpriteSet> natureObjectSpriteSets;
	LookupMap natureObjectSpriteLookup;

	ItemSprite defaultItemSprite;
	std::vector<ItemSprite> itemSprites;
	LookupMap itemSpriteLookup;

	ConstructionSprite defaultConstructionSprite;
	std::vector<ConstructionSprite> constructionSprites;
	LookupMap constructionSpriteLookup;

	SpellSpriteSet defaultSpellSpriteSet;
	std::vector<SpellSpriteSet> spellSpriteSets;
	LookupMap spellSpriteLookup;

	CursorTypeSpriteArray placeableCursors;
	CursorTypeSpriteArray nonplaceableCursors;

	StatusEffectSpriteArray defaultStatusEffects;
		
};
