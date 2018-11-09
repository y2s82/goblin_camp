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

	void DrawCursor(int screenX, int screenY, CursorType type, int cursorHint, bool placeable) const;
	void DrawMarkedOverlay(int screenX, int screenY) const;
	void DrawMarkedOverlay(int screenX, int screenY, Sprite::ConnectedFunction) const;
	void DrawMarker(int screenX, int screenY) const;
	void DrawBlood(int screenX, int screenY, Sprite::ConnectedFunction) const;
	void DrawWater(int screenX, int screenY, Sprite::LayeredConnectedFunction) const;
	void DrawIce(int screenX, int screenY, Sprite::LayeredConnectedFunction) const;
	void DrawFilthMinor(int screenX, int screenY, Sprite::LayeredConnectedFunction) const;
	void DrawFilthMajor(int screenX, int screenY, Sprite::LayeredConnectedFunction) const;
	void DrawTerritoryOverlay(int screenX, int screenY, bool owned, Sprite::ConnectedFunction) const;
	void DrawNPC(int screenX, int screenY, boost::shared_ptr<NPC> npc) const;
	void DrawNatureObject(int screenX, int screenY, boost::shared_ptr<NatureObject> plant) const;
	void DrawItem(int screenX, int screenY, boost::shared_ptr<Item> item) const;
	void DrawSpell(int screenX, int screenY, boost::shared_ptr<Spell> spell) const;
	void DrawFire(int screenX, int screenY, boost::shared_ptr<FireNode> fire) const;
	void DrawBaseConstruction(int screenX, int screenY, Construction * construction, const Coordinate& worldPos) const;
	void DrawUnderConstruction(int screenX, int screenY, Construction * construction, const Coordinate& worldPos) const;
	void DrawUnreadyTrap(int screenX, int screenY, Construction * trap, const Coordinate& worldPos) const;
	void DrawStockpileContents(int screenX, int screenY, Stockpile * construction, const Coordinate& worldPos) const;
	void DrawOpenDoor(int screenX, int screenY, Door * door, const Coordinate& worldPos) const;

	const TerrainSprite& GetTerrainSprite(TileType type) const;
	TerrainSprite& GetTerrainSprite(TileType type);

	int GetGraphicsHintFor(const NPCPreset& npcPreset) const;
	int GetGraphicsHintFor(const NatureObjectPreset& plantPreset) const;
	int GetGraphicsHintFor(const ItemPreset& itemPreset) const;
	int GetGraphicsHintFor(const ConstructionPreset& constructionPreset) const;
	int GetGraphicsHintFor(const SpellPreset& spellPreset) const;

	void SetAuthor(std::string auth);
	void SetVersion(std::string ver);
	void SetDescription(std::string desc);
	void SetTerrain(TileType type, const TerrainSprite& sprite);
	void SetWaterAndIce(Sprite_ptr sprite);
	void SetIce(Sprite_ptr sprite);
	void SetFilthMinor(Sprite_ptr sprite);
	void SetFilthMajor(Sprite_ptr sprite);
	void SetMarker(Sprite_ptr sprite);
	void SetBlood(Sprite_ptr sprite);
	void SetNonTerritoryOverlay(Sprite_ptr sprite);
	void SetTerritoryOverlay(Sprite_ptr sprite);
	void SetMarkedOverlay(Sprite_ptr sprite);
	void SetCursorSprites(CursorType type, Sprite_ptr sprite);
	void SetCursorSprites(CursorType type, Sprite_ptr placeableSprite, Sprite_ptr nonplaceableSprite);
	void SetDefaultUnderConstructionSprite(Sprite_ptr sprite);
	void SetFireSprite(Sprite_ptr sprite);

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
	typedef boost::array<Sprite_ptr, Cursor_Simple_Mode_Count> CursorTypeSpriteArray;
	typedef boost::array<StatusEffectSprite, STATUS_EFFECT_COUNT> StatusEffectSpriteArray;
	typedef boost::unordered_map< std::string, int, boost::hash<std::string> > LookupMap;
	
	int tileWidth;
	int tileHeight;
	std::string name;
	std::string author;
	std::string version;
	std::string description;

	Sprite_ptr waterTile;
	Sprite_ptr iceTile;
	Sprite_ptr fireTile;
	Sprite_ptr minorFilth;
	Sprite_ptr majorFilth;
	
	Sprite_ptr nonTerritoryOverlay;
	Sprite_ptr territoryOverlay;
	Sprite_ptr markedOverlay;

	Sprite_ptr marker;
	Sprite_ptr blood;

	Sprite_ptr defaultUnderConstructionSprite;
	
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

	TerrainSprite defaultTerrainTile;
	TileTypeSpriteArray terrainTiles;

	CursorTypeSpriteArray placeableCursors;
	CursorTypeSpriteArray nonplaceableCursors;

	StatusEffectSpriteArray defaultStatusEffects;
		
};
