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

#include "Farmplot.hpp"
#include "Stockpile.hpp"
#include "Door.hpp"
#include "SpawningPool.hpp"

TileSet::TileSet(std::string tileSetName, int tileW, int tileH) :
	name(tileSetName),
	tileWidth(tileW),
	tileHeight(tileH),
	author(""),
	description(""),
	version(""),
	waterTile(),
	minorFilth(),
	majorFilth(),
	nonTerritoryOverlay(),
	territoryOverlay(),
	markedOverlay(),
	corruptionTile(),
	marker(),
	blood(),
	defaultUnderConstructionSprite(),
	defaultNPCSpriteSet(),
	npcSpriteSets(),
	npcSpriteLookup(),
	defaultNatureObjectSpriteSet(),
	natureObjectSpriteSets(),
	natureObjectSpriteLookup(),
	itemSprites(),
	itemSpriteLookup(),
	defaultItemSprite(),
	constructionSpriteSets(),
	constructionSpriteLookup(),
	defaultConstructionSpriteSet(),
	defaultSpellSpriteSet(),
	spellSpriteLookup(),
	spellSpriteSets(),
	fireTile(),
	defaultTerrainTile() {
		for (int i = 0; i < terrainTiles.size(); ++i) {
			terrainTiles[i] = Sprite();
		}
		for (int i = 0; i < placeableCursors.size(); ++i) {
			placeableCursors[i] = Sprite();
			nonplaceableCursors[i] = Sprite();
		}
		for (int i = 0; i < defaultStatusEffects.size(); ++i) {
			defaultStatusEffects[i] = StatusEffectSprite();
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

std::string TileSet::GetVersion() const {
	return version;
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

void TileSet::DrawTerrain(TileType type, Sprite::ConnectedFunction connected, SDL_Surface *dst, SDL_Rect* dstRect) const {
	if (type == TILENONE || !terrainTiles.at(type).Exists()) { 
		defaultTerrainTile.Draw(dst, dstRect);
	} else {
		terrainTiles.at(type).Draw(connected, dst, dstRect);
	}
}

void TileSet::DrawCorruption(Sprite::ConnectedFunction connected, SDL_Surface *dst, SDL_Rect* dstRect) const {
	corruptionTile.Draw(connected, dst, dstRect);
}

void TileSet::DrawBlood(Sprite::ConnectedFunction connected, SDL_Surface *dst, SDL_Rect* dstRect) const {
	blood.Draw(connected, dst, dstRect);
}

void TileSet::DrawWater(Sprite::ConnectedFunction connected, SDL_Surface *dst, SDL_Rect* dstRect) const {
	waterTile.Draw(connected, dst, dstRect);
}

void TileSet::DrawFilthMinor(SDL_Surface *dst, SDL_Rect * dstRect) const {
	minorFilth.Draw(dst, dstRect);
}

void TileSet::DrawFilthMajor(Sprite::ConnectedFunction connected, SDL_Surface *dst, SDL_Rect * dstRect) const {
	majorFilth.Draw(connected, dst, dstRect);
}

void TileSet::DrawTerritoryOverlay(bool owned, SDL_Surface *dst, SDL_Rect * dstRect) const {
	if (owned) {
		territoryOverlay.Draw(dst, dstRect);
	} else {
		nonTerritoryOverlay.Draw(dst, dstRect);
	}
}

void TileSet::DrawNPC(boost::shared_ptr<NPC> npc, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = npc->GetGraphicsHint();
	if (hint == -1 || hint >= npcSpriteSets.size()) {
		defaultNPCSpriteSet.tile.Draw(dst, dstRect);
	} else {
		npcSpriteSets[hint].tile.Draw(dst, dstRect);
	}

	if ((TCODSystem::getElapsedMilli() % 1000 < 700)) {
		if (npc->HasEffect(CARRYING)) {
			if (boost::shared_ptr<Item> carriedItem = npc->Carrying().lock()) {
				DrawItem(carriedItem, dst, dstRect);
			}
		}
		else if (boost::shared_ptr<Item> wielded = npc->Wielding().lock())
		{
			int itemHint = wielded->GetGraphicsHint();
			if (itemHint != -1 && itemSprites[itemHint].renderWhenWielded)
			{
				DrawItem(wielded, dst, dstRect);
			}
		}
	}

	int numActiveEffects = 0;
	for (NPC::StatusEffectIterator iter = npc->StatusEffects()->begin(); iter != npc->StatusEffects()->end(); ++iter) {
		if (defaultStatusEffects.at(iter->type).IsAlwaysVisible()) {
			defaultStatusEffects.at(iter->type).Draw(false, dst, dstRect);
		} else if (defaultStatusEffects.at(iter->type).Exists()) {
			numActiveEffects++;
		}
	}
		
	if (numActiveEffects > 0) {
		int activeEffect = (TCODSystem::getElapsedMilli() / 250) % numActiveEffects;
		int count = 0;
		for (NPC::StatusEffectIterator iter = npc->StatusEffects()->begin(); iter != npc->StatusEffects()->end(); ++iter) {
			const StatusEffectSprite& sprite = defaultStatusEffects.at(iter->type);
			if (sprite.Exists() && !sprite.IsAlwaysVisible())
			{
				if (count == activeEffect)
				{
					sprite.Draw(numActiveEffects > 1, dst, dstRect);
					break;
				}
				++count;
			}
		}	
	} 
}

void TileSet::DrawNatureObject(boost::shared_ptr<NatureObject> plant, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = plant->GetGraphicsHint();
	if (hint == -1 || hint >= natureObjectSpriteSets.size()) {
		defaultNatureObjectSpriteSet.tile.Draw(dst, dstRect);
	} else {
		natureObjectSpriteSets[hint].tile.Draw(dst, dstRect);
	}
}

void TileSet::DrawItem(boost::shared_ptr<Item> item, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = item->GetGraphicsHint();
	if (hint == -1 || hint >= itemSprites.size()) {
		defaultItemSprite.tile.Draw(dst, dstRect);
	} else {
		itemSprites[hint].tile.Draw(dst, dstRect);
	}
}

void TileSet::DrawOpenDoor(Door * door, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = door->GetGraphicsHint();
	if (hint == -1 || hint >= constructionSpriteSets.size()) {
		defaultConstructionSpriteSet.DrawOpen(worldPos - door->Position(), dst, dstRect);
	} else {
		constructionSpriteSets[hint].DrawOpen(worldPos - door->Position(), dst, dstRect);
	}
}

namespace {
	bool ConstructionConnectTo(ConstructionType type, Coordinate origin, Direction dir) {
		Coordinate pos = origin + Coordinate::DirectionToCoordinate(dir);
		boost::weak_ptr<Construction> constructPtr = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(pos.X(), pos.Y()));
		if (boost::shared_ptr<Construction> otherConstruct = constructPtr.lock()) {
			return otherConstruct->Type() == type;
		}
		return false;
	}
}

void TileSet::DrawBaseConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = construction->GetGraphicsHint();
	const ConstructionSpriteSet& spriteSet((hint == -1 || hint >= constructionSpriteSets.size()) ? defaultConstructionSpriteSet : constructionSpriteSets[hint]);
	if (spriteSet.IsConnectionMap()) {
		ConstructionType type = construction->Type();
		spriteSet.Draw(boost::bind(&ConstructionConnectTo, type, worldPos, _1), dst, dstRect);
	} else {
		spriteSet.Draw(worldPos - construction->Position(), dst, dstRect);
	}
}

void TileSet::DrawUnderConstruction(Construction * construction, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = construction->GetGraphicsHint();
	const ConstructionSpriteSet& spriteSet((hint == -1 || hint >= constructionSpriteSets.size()) ? defaultConstructionSpriteSet : constructionSpriteSets[hint]);
	if (spriteSet.HasUnderConstructionSprites()) {
		if (spriteSet.IsConnectionMap()) {
			ConstructionType type = construction->Type();
			spriteSet.DrawUnderConstruction(boost::bind(&ConstructionConnectTo, type, worldPos, _1), dst, dstRect);
		} else {
			spriteSet.DrawUnderConstruction(worldPos - construction->Position(), dst, dstRect);
		}
	} else {
		defaultUnderConstructionSprite.Draw(dst, dstRect);
	}
}

void TileSet::DrawUnreadyTrap(Construction * trap, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const {
	int hint = trap->GetGraphicsHint();
	const ConstructionSpriteSet& spriteSet((hint == -1 || hint >= constructionSpriteSets.size()) ? defaultConstructionSpriteSet : constructionSpriteSets[hint]);
	if (spriteSet.IsConnectionMap()) {
		ConstructionType type = trap->Type();
		spriteSet.DrawUnreadyTrap(boost::bind(&ConstructionConnectTo, type, worldPos, _1), dst, dstRect);
	} else {
		spriteSet.DrawUnreadyTrap(worldPos - trap->Position(), dst, dstRect);
	}
}

void TileSet::DrawStockpileContents(Stockpile * stockpile, const Coordinate& worldPos, SDL_Surface *dst, SDL_Rect * dstRect) const {
	boost::shared_ptr<Container> storage = stockpile->Storage(worldPos).lock();
	if (storage && !storage->empty()) {
		if (boost::shared_ptr<Item> item = storage->GetFirstItem().lock()) {
			DrawItem(item, dst, dstRect);
		}
	}
}

void TileSet::DrawCursor(CursorType type, int cursorHint, bool placeable, SDL_Surface *dst, SDL_Rect * dstRect) const {
	if (type == Cursor_Item_Mode)
	{
		if (cursorHint == -1 || cursorHint >= itemSprites.size()) {
			defaultItemSprite.tile.Draw(dst, dstRect);
		} else {
			itemSprites[cursorHint].tile.Draw(dst, dstRect);
		}
	} else if (type == Cursor_NPC_Mode) {
		if (cursorHint == -1 || cursorHint >= npcSpriteSets.size()) {
			defaultNPCSpriteSet.tile.Draw(dst, dstRect);
		} else {
			npcSpriteSets[cursorHint].tile.Draw(dst, dstRect);
		}
	} else {
		if (placeable) {
			placeableCursors.at(type).Draw(dst, dstRect);
		} else {
			nonplaceableCursors.at(type).Draw(dst, dstRect);
		}
	}
}

void TileSet::DrawSpell(boost::shared_ptr<Spell> spell, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (spell->GetGraphicsHint() == -1) {
		defaultSpellSpriteSet.Draw(dst, dstRect);
	} else {
		spellSpriteSets[spell->GetGraphicsHint()].Draw(dst, dstRect);
	}
}

void TileSet::DrawFire(boost::shared_ptr<FireNode> fire, SDL_Surface * dst, SDL_Rect * dstRect) const {
	fireTile.Draw(dst, dstRect);
}

int TileSet::GetGraphicsHintFor(const NPCPreset& npcPreset) const {
	LookupMap::const_iterator set;

	set = npcSpriteLookup.find(npcPreset.typeName);
	if (set != npcSpriteLookup.end()) {
		return set->second;
	}

	set = npcSpriteLookup.find(npcPreset.fallbackGraphicsSet);
	if (set != npcSpriteLookup.end()) {
		return set->second;
	}	

	return -1;
}

int TileSet::GetGraphicsHintFor(const NatureObjectPreset& natureObjectPreset) const {
	LookupMap::const_iterator set;
	
	set = natureObjectSpriteLookup.find(natureObjectPreset.name);
	if (set != natureObjectSpriteLookup.end()) {
		return set->second;
	}

	set = natureObjectSpriteLookup.find(natureObjectPreset.fallbackGraphicsSet);
	if (set != natureObjectSpriteLookup.end()) {
		return set->second;
	}

	return -1;
}

int TileSet::GetGraphicsHintFor(const ItemPreset& itemPreset) const {
	LookupMap::const_iterator set;
	
	set = itemSpriteLookup.find(itemPreset.name);
	if (set != itemSpriteLookup.end()) {
		return set->second;
	}

	set = itemSpriteLookup.find(itemPreset.fallbackGraphicsSet);
	if (set != itemSpriteLookup.end()) {
		return set->second;
	}

	for (std::set<ItemCategory>::const_iterator cati = itemPreset.specificCategories.begin(); cati != itemPreset.specificCategories.end(); ++cati) {
		ItemCategory catId = *cati;
		while (catId != -1) {
			ItemCat& cat = Item::Categories.at(catId);
			set = itemSpriteLookup.find(cat.GetName());
			if (set != itemSpriteLookup.end())
			{
				return set->second;
			}
			catId = cat.parent;
		}
	}

	return -1;
}

int TileSet::GetGraphicsHintFor(const ConstructionPreset& constructPreset) const {
	LookupMap::const_iterator set;
	
	set = constructionSpriteLookup.find(constructPreset.name);
	if (set != constructionSpriteLookup.end()) {
		return set->second;
	}

	set = constructionSpriteLookup.find(constructPreset.fallbackGraphicsSet);
	if (set != constructionSpriteLookup.end()) {
		return set->second;
	}

	return -1;
}

int TileSet::GetGraphicsHintFor(const SpellPreset& spellPreset) const {
	LookupMap::const_iterator set;
	
	set = spellSpriteLookup.find(spellPreset.name);
	if (set != spellSpriteLookup.end()) {
		return set->second;
	}

	set = spellSpriteLookup.find(spellPreset.fallbackGraphicsSet);
	if (set != spellSpriteLookup.end()) {
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

void TileSet::SetVersion(std::string ver) {
	version = ver;
}

void TileSet::SetTerrain(TileType type, const Sprite& sprite) {
	if (type < 0 || type >= TILE_TYPE_COUNT) 
		return;

	if (type == TILENONE) {
		defaultTerrainTile = sprite;
	} else {
		terrainTiles[type] = sprite;
	}
}

void TileSet::SetWater(const Sprite& sprite) {
	waterTile = sprite;
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

void TileSet::SetCorruption(const Sprite& sprite) {
	corruptionTile = sprite;
}

void TileSet::SetCursorSprites(CursorType type, const Sprite& sprite) {
	placeableCursors[type] = sprite;
	nonplaceableCursors[type] = sprite;
	if (type == Cursor_None) {
		for (CursorTypeSpriteArray::size_type i = 0; i < placeableCursors.size(); ++i) {
			if (!placeableCursors[i].Exists()) {
				placeableCursors[i] = sprite;
				nonplaceableCursors[i] = sprite;
			}
		}
	}
}

void TileSet::SetCursorSprites(CursorType type, const Sprite& placeableSprite, const Sprite& nonplaceableSprite) {
	placeableCursors[type] = placeableSprite;
	nonplaceableCursors[type] = nonplaceableSprite;
	if (type == Cursor_None) {
		for (CursorTypeSpriteArray::size_type i = 0; i < placeableCursors.size(); ++i) {
			if (!placeableCursors[i].Exists()) {
				placeableCursors[i] = placeableSprite;
				nonplaceableCursors[i] = nonplaceableSprite;
			}
		}
	}
}

void TileSet::SetStatusSprite(StatusEffectType statusEffect, const StatusEffectSprite& sprite) {
	defaultStatusEffects[statusEffect] = sprite;
}

void TileSet::SetDefaultUnderConstructionSprite(const Sprite& sprite) {
	defaultUnderConstructionSprite = sprite;
}

void TileSet::SetFireSprite(const Sprite& sprite) {
	fireTile = sprite;
}

void TileSet::AddNPCSpriteSet(std::string name, const NPCSpriteSet& set) {
	int index = npcSpriteSets.size();
	npcSpriteSets.push_back(set);
	npcSpriteLookup[name] = index;
}

void TileSet::SetDefaultNPCSpriteSet(const NPCSpriteSet& set) {
	defaultNPCSpriteSet = set;
}

void TileSet::AddNatureObjectSpriteSet(std::string name, const NatureObjectSpriteSet& set) {
	int index = natureObjectSpriteSets.size();
	natureObjectSpriteSets.push_back(set);
	natureObjectSpriteLookup[name] = index;
}

void TileSet::SetDefaultNatureObjectSpriteSet(const NatureObjectSpriteSet& set) {
	defaultNatureObjectSpriteSet = set;
}

void TileSet::AddItemSprite(std::string name, const ItemSprite& set) {
	int index = itemSprites.size();
	itemSprites.push_back(set);
	itemSpriteLookup[name] = index;
}

void TileSet::SetDefaultItemSprite(const ItemSprite& set) {
	defaultItemSprite = set;
}

void TileSet::AddConstructionSpriteSet(std::string name, const ConstructionSpriteSet& set) {
	int index = constructionSpriteSets.size();
	constructionSpriteSets.push_back(set);
	constructionSpriteLookup[name] = index;
}

void TileSet::SetDefaultConstructionSpriteSet(const ConstructionSpriteSet& set) {
	defaultConstructionSpriteSet = set;
}

void TileSet::AddSpellSpriteSet(std::string name, const SpellSpriteSet& set) {
	int index = spellSpriteSets.size();
	spellSpriteSets.push_back(set);
	spellSpriteLookup[name] = index;
}

void TileSet::SetDefaultSpellSpriteSet(const SpellSpriteSet& set) {
	defaultSpellSpriteSet = set;
}
