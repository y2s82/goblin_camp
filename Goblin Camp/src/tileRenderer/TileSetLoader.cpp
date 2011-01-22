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
#include "Logger.hpp"

const char* TileSetLoader::uninitialisedTilesetError = "tileset_data must be defined and tileWidth & tileHeight must be provided first";

TileSetLoader::TileSetLoader() :
	tileSet(),
	success(false),
	parser(),
	tileSetName(),
	tileWidth(-1),
	tileHeight(-1),
	currentTexture(),
	currentSpriteSet(SS_NONE),
	tileSetPath(),
	npcSpriteSet(),
	natureObjectSpriteSet(),
	itemSpriteSet(),
	constructionSpriteSet()
{
	TCODParserStruct* creatureSpriteStruct = parser.newStructure("creature_sprite_data");
	creatureSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

	TCODParserStruct* natureObjectSpriteStruct = parser.newStructure("plant_sprite_data");
	natureObjectSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

	TCODParserStruct* itemSpriteStruct = parser.newStructure("item_sprite_data");
	itemSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

	TCODParserStruct* constructionSpriteStruct = parser.newStructure("construction_sprite_data");
	constructionSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
	constructionSpriteStruct->addListProperty("underconstruction_sprites", TCOD_TYPE_INT, false);
	constructionSpriteStruct->addProperty("openSprite", TCOD_TYPE_INT, false);
	constructionSpriteStruct->addProperty("width", TCOD_TYPE_INT, false);
	
	TCODParserStruct* tileTextureStruct = parser.newStructure("tile_texture_data");

	// Terrain tile types
	tileTextureStruct->addProperty("unknown_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("grass_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("ditch_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("riverbed_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("bog_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("rock_terrain", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("mud_terrain", TCOD_TYPE_INT, false);

	// Terrain modifiers
	tileTextureStruct->addListProperty("water_levels", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("minor_filth", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("major_filth", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("marker", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("blood", TCOD_TYPE_INT, false);

	// Overlays
	tileTextureStruct->addProperty("non_territory", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("territory", TCOD_TYPE_INT, false);
	tileTextureStruct->addProperty("marked", TCOD_TYPE_INT, false);

	tileTextureStruct->addProperty("default_underconstruction", TCOD_TYPE_INT, false);

	// Cursors
	tileTextureStruct->addListProperty("default_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("construction_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("stockpile_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("treeFelling_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("harvest_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("order_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("tree_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("dismantle_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("undesignate_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("bog_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("dig_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("add_territory_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("remove_territory_cursor", TCOD_TYPE_INT, false);
	tileTextureStruct->addListProperty("gather_cursor", TCOD_TYPE_INT, false);	
	
	// Sprite Sets
	tileTextureStruct->addStructure(creatureSpriteStruct);
	tileTextureStruct->addStructure(natureObjectSpriteStruct);
	tileTextureStruct->addStructure(itemSpriteStruct);
	tileTextureStruct->addStructure(constructionSpriteStruct);

	TCODParserStruct* tilesetStruct = parser.newStructure("tileset_data");
	tilesetStruct->addProperty("tileWidth", TCOD_TYPE_INT, true);
	tilesetStruct->addProperty("tileHeight", TCOD_TYPE_INT, true);
	tilesetStruct->addProperty("author", TCOD_TYPE_STRING, false);
	tilesetStruct->addProperty("description", TCOD_TYPE_STRING, false);
	tilesetStruct->addStructure(tileTextureStruct);
}

TileSetLoader::~TileSetLoader() {
}

bool TileSetLoader::LoadTileSet(boost::filesystem::path path) {
	success = true;
	tileSetPath = path.branch_path();
	tileSet = boost::shared_ptr<TileSet>();

	parser.run(path.string().c_str(), this);

	if (tileSet.get() == NULL) {
		success = false;
	}
	Reset();
	return success;
}

bool TileSetLoader::Success() const {
	return success;
}

boost::shared_ptr<TileSet> TileSetLoader::LoadedTileSet() const {
	if (success) {
		return tileSet;
	}
	return boost::shared_ptr<TileSet>();
}

void TileSetLoader::Reset() {
	tileSetName = "";
	tileWidth = -1;
	tileHeight = -1;
	currentTexture = boost::shared_ptr<TileSetTexture>();
}

bool TileSetLoader::parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
	// TileSet data (should be first and root)	
	if (boost::iequals(str->getName(), "tileset_data"))
	{
		tileSetName = name;
		if (tileSet.get() != NULL) {
			parser->error("Multiple tile set declarations in one file not supported");
		}
		return success;
	}
	
	if (tileSet.get() == NULL) {
		parser->error(uninitialisedTilesetError);
		return false;
	}

	// Texture structure
	if (boost::iequals(str->getName(), "tile_texture_data")) {
		currentTexture = boost::shared_ptr<TileSetTexture>(new TileSetTexture(tileSetPath / name, tileWidth, tileHeight));
		if (currentTexture->Count() == 0) {
			parser->error("Failed to load texture %s", name); 
		}
		return success;
	}
	
	if (currentTexture.get() == 0) {
		parser->error(uninitialisedTilesetError);
		return false;
	}
	
	// Sprite Sets
	if (boost::iequals(str->getName(), "creature_sprite_data")) {
		npcSpriteSet = NPCSpriteSet();
		currentSpriteSet = SS_NPC;
	} else if (boost::iequals(str->getName(), "plant_sprite_data")) {
		natureObjectSpriteSet = NatureObjectSpriteSet();
		currentSpriteSet = SS_NATURE;
	} else if (boost::iequals(str->getName(), "item_sprite_data")) {
		itemSpriteSet = ItemSpriteSet();
		currentSpriteSet = SS_ITEM;
	} else if (boost::iequals(str->getName(), "construction_sprite_data")) {
		constructionSpriteSet = ConstructionSpriteSet();
		currentSpriteSet = SS_CONSTRUCTION;
	}

	return success;
}

bool TileSetLoader::parserFlag(TCODParser *parser,const char *name) {
	return success;
}

bool TileSetLoader::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
	// Tile Texture Properties
	if (currentTexture.get() != NULL) {
		switch (currentSpriteSet) {
		case SS_NONE:
			if (boost::iequals(name, "unknown_terrain")) {
				tileSet->SetTerrain(TILENONE, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "grass_terrain")) {
				tileSet->SetTerrain(TILEGRASS, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "ditch_terrain")) {
				tileSet->SetTerrain(TILEDITCH, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "riverbed_terrain")) {
				tileSet->SetTerrain(TILERIVERBED, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "bog_terrain")) {
				tileSet->SetTerrain(TILEBOG, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "rock_terrain")) {
				tileSet->SetTerrain(TILEROCK, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "mud_terrain")) {
				tileSet->SetTerrain(TILEMUD, Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "water_levels")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tileSet->AddWater(Sprite((intptr_t)TCOD_list_get(value.list, i), currentTexture));
			} else if (boost::iequals(name, "minor_filth")) {
				tileSet->SetFilthMinor(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "major_filth")) {
				tileSet->SetFilthMajor(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "marker")) {
				tileSet->SetMarker(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "blood")) {
				tileSet->SetBlood(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "non_territory")) {
				tileSet->SetNonTerritoryOverlay(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "territory")) {
				tileSet->SetTerritoryOverlay(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "marked")) {
				tileSet->SetMarkedOverlay(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "default_cursor")) {
				SetCursorSprites(Cursor_None, value.list);
			} else if (boost::iequals(name, "construction_cursor")) {
				SetCursorSprites(Cursor_Construct, value.list);
			} else if (boost::iequals(name, "stockpile_cursor")) {
				SetCursorSprites(Cursor_Stockpile, value.list);
			} else if (boost::iequals(name, "treeFelling_cursor")) {
				SetCursorSprites(Cursor_TreeFelling, value.list);
			} else if (boost::iequals(name, "harvest_cursor")) {
				SetCursorSprites(Cursor_Harvest, value.list);
			} else if (boost::iequals(name, "order_cursor")) {
				SetCursorSprites(Cursor_Order, value.list);
			} else if (boost::iequals(name, "tree_cursor")) {
				SetCursorSprites(Cursor_Tree, value.list);
			} else if (boost::iequals(name, "dismantle_cursor")) {
				SetCursorSprites(Cursor_Dismantle, value.list);
			} else if (boost::iequals(name, "undesignate_cursor")) {
				SetCursorSprites(Cursor_Undesignate, value.list);
			} else if (boost::iequals(name, "bog_cursor")) {
				SetCursorSprites(Cursor_Bog, value.list);
			} else if (boost::iequals(name, "dig_cursor")) {
				SetCursorSprites(Cursor_Dig, value.list);
			} else if (boost::iequals(name, "add_territory_cursor")) {
				SetCursorSprites(Cursor_AddTerritory, value.list);
			} else if (boost::iequals(name, "remove_territory_cursor")) {
				SetCursorSprites(Cursor_RemoveTerritory, value.list);
			} else if (boost::iequals(name, "gather_cursor")) {
				SetCursorSprites(Cursor_Gather, value.list);
			} else if (boost::iequals(name, "default_underconstruction")) {
				tileSet->SetDefaultUnderConstructionSprite(Sprite(value.i, currentTexture));
			}
			break;
		case SS_NPC:
			if (boost::iequals(name, "sprite")) {
				npcSpriteSet.tile = Sprite(value.i, currentTexture);
			}
			break;
		case SS_ITEM:
			if (boost::iequals(name, "sprite")) {
				itemSpriteSet.tile = Sprite(value.i, currentTexture);
			}
			break;
		case SS_NATURE:
			if (boost::iequals(name, "sprite")) {
				natureObjectSpriteSet.tile = Sprite(value.i, currentTexture);
			}
			break;
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionSpriteSet.AddSprite(Sprite((intptr_t)TCOD_list_get(value.list, i), currentTexture));
			} else if (boost::iequals(name, "underconstruction_sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionSpriteSet.AddUnderConstructionSprite(Sprite((intptr_t)TCOD_list_get(value.list, i), currentTexture));
			} else if (boost::iequals(name, "openSprite")) {
				constructionSpriteSet.SetOpenSprite(Sprite(value.i, currentTexture));
			} else if (boost::iequals(name, "width")) {
				constructionSpriteSet.SetWidth(value.i);
			}
			break;
		}	
		return success;
	}

	// Mandatory tile set properties
	if (boost::iequals(name, "tileWidth")) {
		tileWidth = value.i;
		if (tileWidth != -1 && tileHeight != -1)  {
			tileSet = boost::shared_ptr<TileSet>(new TileSet(tileSetName, tileWidth, tileHeight));
		}
		return success;
	} else if (boost::iequals(name, "tileHeight")) {
		tileHeight = value.i;
		if (tileWidth != -1 && tileHeight != -1)  {
			tileSet = boost::shared_ptr<TileSet>(new TileSet(tileSetName, tileWidth, tileHeight));
		}
		return success;
	}

	if (tileSet.get() == NULL) { 
		parser->error(uninitialisedTilesetError); 
		return false; 
	}

	// Optional tile set properties
	if (boost::iequals(name, "description")) {
		tileSet->SetDescription(value.s);
	} else if (boost::iequals(name, "author")) {
		tileSet->SetAuthor(value.s);
	} 

	return success;
}

bool TileSetLoader::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	if (boost::iequals(str->getName(), "tile_texture_data")) {
		currentTexture = boost::shared_ptr<TileSetTexture>();
	} else if (boost::iequals(str->getName(), "creature_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultNPCSpriteSet(npcSpriteSet);
		} else {
			tileSet->AddNPCSpriteSet(std::string(name), npcSpriteSet);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "plant_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultNatureObjectSpriteSet(natureObjectSpriteSet);
		} else {
			tileSet->AddNatureObjectSpriteSet(std::string(name), natureObjectSpriteSet);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "item_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultItemSpriteSet(itemSpriteSet);
		} else {
			tileSet->AddItemSpriteSet(std::string(name), itemSpriteSet);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "construction_sprite_data")) {
		if (constructionSpriteSet.IsValid()) {
			if (name == 0) {
				tileSet->SetDefaultConstructionSpriteSet(constructionSpriteSet);
			} else {
				tileSet->AddConstructionSpriteSet(std::string(name), constructionSpriteSet);
			}
		} else {
			if (name == 0) {
				LOG("Skipping invalid construction sprite data: default");
			} else {
				LOG("Skipping invalid construction sprite data: " << std::string(name));
			}
		}
		currentSpriteSet = SS_NONE;
	}
	
	
	return success;
}

void TileSetLoader::error(const char *msg) {
	LOG(msg);
	success = false;
}

void TileSetLoader::SetCursorSprites(CursorType type, TCOD_list_t cursors) {
	int size = TCOD_list_size(cursors);
	if (size == 1) {
		tileSet->SetCursorSprites(type, Sprite((intptr_t)TCOD_list_get(cursors, 0), currentTexture));
	} else if (size > 1) {
		tileSet->SetCursorSprites(type, Sprite((intptr_t)TCOD_list_get(cursors, 0), currentTexture), Sprite((intptr_t)TCOD_list_get(cursors, 1), currentTexture));
	}
}