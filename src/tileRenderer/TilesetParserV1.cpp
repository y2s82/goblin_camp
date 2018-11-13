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

#include <boost/algorithm/string.hpp>

#include "tileRenderer/TilesetParserV1.hpp"
#include "tileRenderer/StatusEffectSprite.hpp"
#include "Logger.hpp"

const char* TileSetParserV1::uninitialisedTilesetError = "tileset_data must be defined and tileWidth & tileHeight must be provided first";

namespace {
	void SetupTilesetParser(TCODParser& parser) {
		TCODParserStruct* creatureSpriteStruct = parser.newStructure("creature_sprite_data");
		creatureSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

		TCODParserStruct* natureObjectSpriteStruct = parser.newStructure("plant_sprite_data");
		natureObjectSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

		TCODParserStruct* itemSpriteStruct = parser.newStructure("item_sprite_data");
		itemSpriteStruct->addProperty("sprite", TCOD_TYPE_INT, true);

		TCODParserStruct* spellSpriteStruct = parser.newStructure("spell_sprite_data");
		spellSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		spellSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);

		TCODParserStruct* constructionSpriteStruct = parser.newStructure("construction_sprite_data");
		constructionSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		constructionSpriteStruct->addListProperty("underconstruction_sprites", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addProperty("openSprite", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addProperty("width", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addFlag("connection_map");
	
		TCODParserStruct* tileTextureStruct = parser.newStructure("tile_texture_data");

		// Terrain tile types
		tileTextureStruct->addProperty("unknown_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("grass_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("ditch_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("riverbed_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("bog_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("rock_terrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("mud_terrain", TCOD_TYPE_INT, false);

		// Terrain modifiers
		tileTextureStruct->addListProperty("water", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("minor_filth", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("major_filth", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("marker", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("blood", TCOD_TYPE_INT, false);

		// Overlays
		tileTextureStruct->addProperty("non_territory", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("territory", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("marked", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("corruption", TCOD_TYPE_INT, false);

		// Status Effects
		tileTextureStruct->addProperty("default_hungry", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_thirsty", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_panic", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_concussion", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_drowsy", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_asleep", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_poison", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_bleeding", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_sluggish", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_rage", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_eating", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_drinking", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_swimming", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("default_burning", TCOD_TYPE_INT, false);

		tileTextureStruct->addProperty("default_underconstruction", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("fire", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("fireFPS", TCOD_TYPE_INT, false);

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
		tileTextureStruct->addStructure(spellSpriteStruct);

		TCODParserStruct* tilesetStruct = parser.newStructure("tileset_data");
		tilesetStruct->addProperty("tileWidth", TCOD_TYPE_INT, true);
		tilesetStruct->addProperty("tileHeight", TCOD_TYPE_INT, true);
		tilesetStruct->addProperty("author", TCOD_TYPE_STRING, false);
		tilesetStruct->addProperty("version", TCOD_TYPE_STRING, false);
		tilesetStruct->addProperty("description", TCOD_TYPE_STRING, false);
		tilesetStruct->addStructure(tileTextureStruct);
	}
}

TileSetParserV1::TileSetParserV1(boost::shared_ptr<TilesetRenderer> spriteFactory) :
	parser(),
	spriteFactory(spriteFactory),

	tileSet(),
	success(true),

	tileSetPath(),

	tileSetName(),
	tileWidth(-1),
	tileHeight(-1),
	currentTexture(),

	fireSprites(),
	fireFPS(15),

	tempConstruction(spriteFactory),

	tempSpell(spriteFactory),

	currentSpriteSet(SS_NONE),
	npcSprite(),
	natureObjectSpriteSet(),
	itemSprite(),
	corruptionOverride()
{
	SetupTilesetParser(parser);
}

TileSetParserV1::~TileSetParserV1() {
	
}

boost::shared_ptr<TileSet> TileSetParserV1::Run(boost::filesystem::path dataFilePath) {
	tileSetName = "";
	tileWidth = -1;
	tileHeight = -1;
	currentTexture = boost::shared_ptr<TileSetTexture>();
	tileSetPath = dataFilePath.parent_path();
	success = true;

	parser.run(dataFilePath.string().c_str(), this);

	boost::shared_ptr<TileSet> result = tileSet;
	tileSet.reset();
	if (success)
		return result;
	return boost::shared_ptr<TileSet>();
}

void TileSetParserV1::SetCursorSprites(CursorType type, TCOD_list_t cursors) {
	int size = TCOD_list_size(cursors);
	if (size == 1) {
		tileSet->SetCursorSprites(type, spriteFactory->CreateSprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)));
	} else if (size > 1) {
		tileSet->SetCursorSprites(type, spriteFactory->CreateSprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)), spriteFactory->CreateSprite( currentTexture, (intptr_t)TCOD_list_get(cursors, 1)));
	}
}

bool TileSetParserV1::parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
	// TileSet data (should be first and root)	
	if (boost::iequals(str->getName(), "tileset_data"))
	{
		tileSetName = name;
		if (tileSet) {
			parser->error("Multiple tile set declarations in one file not supported");
		}
		return success;
	}
	
	if (!tileSet) {
		parser->error(uninitialisedTilesetError);
		return false;
	}

	// Texture structure
	if (boost::iequals(str->getName(), "tile_texture_data")) {
		currentTexture = boost::shared_ptr<TileSetTexture>(new TileSetTexture(tileSetPath / name, tileWidth, tileHeight));
		fireSprites.clear();
		fireFPS = 15;
		if (currentTexture->Count() == 0) {
			parser->error("Failed to load texture %s", name); 
		}
		return success;
	}
	
	if (!currentTexture) {
		parser->error(uninitialisedTilesetError);
		return false;
	}
	
	// Sprite Sets
	if (boost::iequals(str->getName(), "creature_sprite_data")) {
		npcSprite = NPCSprite();
		currentSpriteSet = SS_NPC;
	} else if (boost::iequals(str->getName(), "plant_sprite_data")) {
		natureObjectSpriteSet = NatureObjectSpriteSet();
		currentSpriteSet = SS_NATURE;
	} else if (boost::iequals(str->getName(), "item_sprite_data")) {
		itemSprite = ItemSprite();
		currentSpriteSet = SS_ITEM;
	} else if (boost::iequals(str->getName(), "construction_sprite_data")) {
		currentSpriteSet = SS_CONSTRUCTION;
		tempConstruction = TempConstruction(spriteFactory);
	} else if (boost::iequals(str->getName(), "spell_sprite_data")) {
		tempSpell = TempSpell(spriteFactory);
		currentSpriteSet = SS_SPELL;
	}

	return success;
}

bool TileSetParserV1::parserFlag(TCODParser *parser,const char *name) {
	if (currentTexture) {
		switch (currentSpriteSet) {
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "connection_map")) {
				tempConstruction.connectionMapped = true;
			}
			break;
		case SS_NONE:
		case SS_NPC:
		case SS_ITEM:
		case SS_NATURE:
		case SS_SPELL:
			parser->error("unsupported flag '%s'", name);
			success = false;
			break;
		}
	}
	return success;
}

Sprite_ptr TileSetParserV1::ReadConnectionMap(TCOD_list_t tiles) {
	int size = TCOD_list_size(tiles);
	if (size == 47 || size == 16) {
		std::vector<int> orderedTiles;
		for (int i = 12; i < 16; ++i)
			orderedTiles.push_back((intptr_t)TCOD_list_get(tiles, i));
		for (int i = 0; i < 12; ++i)
			orderedTiles.push_back((intptr_t)TCOD_list_get(tiles, i));
		for (int i = 16; i < size; ++i)
			orderedTiles.push_back((intptr_t)TCOD_list_get(tiles, i));
		return spriteFactory->CreateSprite(currentTexture, orderedTiles, true);
	} else {
		return TilesetRenderer::CreateSprite(spriteFactory, currentTexture, (intptr_t*)TCOD_list_begin(tiles), (intptr_t*)TCOD_list_end(tiles), true);
	}
}

bool TileSetParserV1::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
	// Tile Texture Properties
	if (currentTexture) {
		switch (currentSpriteSet) {
		case SS_NONE:
			// Terrain
			if (boost::iequals(name, "unknown_terrain")) {
				tileSet->SetTerrain(TILENONE, TerrainSprite(spriteFactory->CreateSprite(currentTexture, value.i)));
			} else if (boost::iequals(name, "grass_terrain")) {
				tileSet->SetTerrain(TILEGRASS, TerrainSprite(ReadConnectionMap(value.list)));
			} else if (boost::iequals(name, "ditch_terrain")) {
				tileSet->SetTerrain(TILEDITCH, TerrainSprite(ReadConnectionMap(value.list)));
			} else if (boost::iequals(name, "riverbed_terrain")) {
				tileSet->SetTerrain(TILERIVERBED, TerrainSprite(ReadConnectionMap(value.list)));
			} else if (boost::iequals(name, "bog_terrain")) {
				tileSet->SetTerrain(TILEBOG, TerrainSprite(ReadConnectionMap(value.list)));
			} else if (boost::iequals(name, "rock_terrain")) {
				tileSet->SetTerrain(TILEROCK, TerrainSprite(ReadConnectionMap(value.list)));
			} else if (boost::iequals(name, "mud_terrain")) {
				tileSet->SetTerrain(TILEMUD, TerrainSprite(ReadConnectionMap(value.list)));
			} 
			
			// Terrain Modifiers
			else if (boost::iequals(name, "water")) {
				tileSet->SetWaterAndIce(ReadConnectionMap(value.list));
			} else if (boost::iequals(name, "minor_filth")) {
				tileSet->SetFilthMinor(spriteFactory->CreateSprite(currentTexture, value.i));
			} else if (boost::iequals(name, "major_filth")) {
				tileSet->SetFilthMajor(ReadConnectionMap(value.list));
			} else if (boost::iequals(name, "marker")) {
				tileSet->SetMarker(spriteFactory->CreateSprite(currentTexture, value.i));
			} else if (boost::iequals(name, "blood")) {
				tileSet->SetBlood(ReadConnectionMap(value.list));
			} 
			
			// Overlays
			else if (boost::iequals(name, "non_territory")) {
				tileSet->SetNonTerritoryOverlay(spriteFactory->CreateSprite(currentTexture, value.i));
			} else if (boost::iequals(name, "territory")) {
				tileSet->SetTerritoryOverlay(spriteFactory->CreateSprite(currentTexture, value.i));
			} else if (boost::iequals(name, "marked")) {
				tileSet->SetMarkedOverlay(spriteFactory->CreateSprite(currentTexture, value.i));
			} else if (boost::iequals(name, "corruption")) {
				corruptionOverride = ReadConnectionMap(value.list);
			}

			// Cursors
			else if (boost::iequals(name, "default_cursor")) {
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
			} 
			
			// Status Effects
			else if (boost::iequals(name, "default_hungry")) {
				tileSet->SetStatusSprite(HUNGER, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_thirsty")) {
				tileSet->SetStatusSprite(THIRST, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_panic")) {
				tileSet->SetStatusSprite(PANIC, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_concussion")) {
				tileSet->SetStatusSprite(CONCUSSION, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_drowsy")) {
				tileSet->SetStatusSprite(DROWSY, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_asleep")) {
				tileSet->SetStatusSprite(SLEEPING, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_poison")) {
				tileSet->SetStatusSprite(POISON, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_bleeding")) {
				tileSet->SetStatusSprite(BLEEDING, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_sluggish")) {
				tileSet->SetStatusSprite(BADSLEEP, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_rage")) {
				tileSet->SetStatusSprite(RAGE, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_eating")) {
				tileSet->SetStatusSprite(EATING, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_drinking")) {
				tileSet->SetStatusSprite(DRINKING, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 1, false));
			} else if (boost::iequals(name, "default_swimming")) {
				tileSet->SetStatusSprite(SWIM, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 0, true));
			} else if (boost::iequals(name, "default_burning")) {
				tileSet->SetStatusSprite(BURNING, StatusEffectSprite(spriteFactory->CreateSprite(currentTexture, value.i), 0, true));
			}
			
			else if (boost::iequals(name, "default_underconstruction")) {
				tileSet->SetDefaultUnderConstructionSprite(spriteFactory->CreateSprite(currentTexture, value.i));
			}

			else if (boost::iequals(name, "fireFPS")) {
				fireFPS = value.i;
			}
			else if (boost::iequals(name, "fire")) {
				for (intptr_t * iter = (intptr_t*)TCOD_list_begin(value.list); iter != (intptr_t*)TCOD_list_end(value.list); ++iter) {
					fireSprites.push_back(*iter);
				}
			}
			break;
		case SS_NPC:
			if (boost::iequals(name, "sprite")) {
				npcSprite = NPCSprite(spriteFactory->CreateSprite(currentTexture, value.i));
			}
			break;
		case SS_ITEM:
			if (boost::iequals(name, "sprite")) {
				itemSprite.tile = spriteFactory->CreateSprite(currentTexture, value.i);
			}
			break;
		case SS_NATURE:
			if (boost::iequals(name, "sprite")) {
				natureObjectSpriteSet.tile = spriteFactory->CreateSprite(currentTexture, value.i);
			}
			break;
		case SS_CONSTRUCTION:
			// Record sprites, under construction sprites for later assembly once width/connection map flag is known
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tempConstruction.mainSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "underconstruction_sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tempConstruction.underConstructionSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} 
			else if (boost::iequals(name, "openSprite")) {
				tempConstruction.openDoor = spriteFactory->CreateSprite(currentTexture, value.i);
			} else if (boost::iequals(name, "width")) {
				tempConstruction.width = value.i;
			}
			break;
		case SS_SPELL:
			// Record sprites for later construction once both sprites and framerate is determined
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tempSpell.sprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "fps")) {
				tempSpell.fps = value.i;
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

	if (!tileSet) { 
		parser->error(uninitialisedTilesetError); 
		return false; 
	}

	// Optional tile set properties
	if (boost::iequals(name, "description")) {
		tileSet->SetDescription(value.s);
	} else if (boost::iequals(name, "author")) {
		tileSet->SetAuthor(value.s);
	} else if (boost::iequals(name, "version")) {
		tileSet->SetVersion(value.s);
	}

	return success;
}

bool TileSetParserV1::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	if (boost::iequals(str->getName(), "tileset_data")) {
		if (corruptionOverride.Exists()) {
			for (TileType type = TILENONE; type < TILE_TYPE_COUNT; type = static_cast<TileType>(static_cast<unsigned int>(type) + 1)) {
				TerrainSprite sprite = tileSet->GetTerrainSprite(type);
				sprite.SetCorruption(corruptionOverride);
				tileSet->SetTerrain(type, sprite);
			}
			corruptionOverride = Sprite_ptr();
		}
	}
	else if (boost::iequals(str->getName(), "tile_texture_data")) {
		if (fireSprites.size() > 0) {
			tileSet->SetFireSprite(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, fireSprites.begin(), fireSprites.end(), false, fireFPS));
		}
		currentTexture = boost::shared_ptr<TileSetTexture>();
	} else if (boost::iequals(str->getName(), "creature_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultNPCSprite(npcSprite);
		} else {
			tileSet->AddNPCSprite(std::string(name), npcSprite);
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
			tileSet->SetDefaultItemSprite(itemSprite);
		} else {
			tileSet->AddItemSprite(std::string(name), itemSprite);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "construction_sprite_data")) {
		ConstructionSprite constructionSprite = tempConstruction.Build(currentTexture);
		if (constructionSprite.IsValid()) {
			if (name == 0) {
				tileSet->SetDefaultConstructionSprite(constructionSprite);
			} else {
				tileSet->AddConstructionSprite(std::string(name), constructionSprite);
			}
		} else {
			if (name == 0) {
				LOG("Skipping invalid construction sprite data: default");
			} else {
				LOG("Skipping invalid construction sprite data: " << std::string(name));
			}
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "spell_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultSpellSpriteSet(tempSpell.Build(currentTexture));
		} else {
			tileSet->AddSpellSpriteSet(std::string(name), tempSpell.Build(currentTexture));
		}
		currentSpriteSet = SS_NONE;
	}
	
	
	return success;
}

void TileSetParserV1::error(const char *msg) {
	LOG(msg);
	success = false;
}

ConstructionSprite TileSetParserV1::TempConstruction::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	ConstructionSprite spriteSet = ConstructionSprite();
	if (connectionMapped) {
		if (mainSprites.size() > 0) {
			if (mainSprites.size() == 16 || mainSprites.size() == 47) {
				std::vector<int> orderedTiles;
				for (size_t i = 12; i < 16; ++i)
					orderedTiles.push_back(mainSprites.at(i));
				for (size_t i = 0; i < 12; ++i)
					orderedTiles.push_back(mainSprites.at(i));
				for (size_t i = 16; i < mainSprites.size(); ++i)
					orderedTiles.push_back(mainSprites.at(i));
				spriteSet.AddSprite(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, orderedTiles.begin(), orderedTiles.end(), true));
			} else {
				spriteSet.AddSprite(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, mainSprites.begin(), mainSprites.end(), true));
			}
		}
		if (underConstructionSprites.size() > 0) {
			if (underConstructionSprites.size() == 16 || underConstructionSprites.size() == 47) {
				std::vector<int> orderedTiles;
				for (size_t i = 12; i < 16; ++i)
					orderedTiles.push_back(underConstructionSprites.at(i));
				for (size_t i = 0; i < 12; ++i)
					orderedTiles.push_back(underConstructionSprites.at(i));
				for (size_t i = 16; i < underConstructionSprites.size(); ++i)
					orderedTiles.push_back(underConstructionSprites.at(i));
				spriteSet.AddUnderConstructionSprite(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, orderedTiles.begin(), orderedTiles.end(), true));
			} else {
				spriteSet.AddUnderConstructionSprite(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, underConstructionSprites.begin(), underConstructionSprites.end(), true));
			}
		}
	} else {
		for (std::vector<int>::iterator iter = mainSprites.begin(); iter != mainSprites.end(); ++iter) {
			spriteSet.AddSprite(spriteFactory->CreateSprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = underConstructionSprites.begin(); iter != underConstructionSprites.end(); ++iter) {
			spriteSet.AddUnderConstructionSprite(spriteFactory->CreateSprite(currentTexture, *iter));
		}
		spriteSet.SetWidth(width);
	}
	spriteSet.SetOpenSprite(openDoor);
	return spriteSet;
}

SpellSpriteSet TileSetParserV1::TempSpell::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	return SpellSpriteSet(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, sprites.begin(), sprites.end(), false, fps));
}


TileSetMetadataParserV1::TileSetMetadataParserV1()
	: parser(), metadata()
{
	SetupTilesetParser(parser);
	
}

TileSetMetadata TileSetMetadataParserV1::Run(boost::filesystem::path dataFilePath) {
	metadata = TileSetMetadata(dataFilePath.parent_path());
	metadata.valid = true;

	parser.run(dataFilePath.string().c_str(), this);

	return metadata;
}

bool TileSetMetadataParserV1::parserProperty(TCODParser*, const char *name, TCOD_value_type_t, TCOD_value_t value) {
	if (boost::iequals(name, "author")) {
		metadata.author = value.s;
	} else if (boost::iequals(name, "version")) {
		metadata.version = value.s;
	} else if (boost::iequals(name, "description")) {
		metadata.description = value.s;
	} else if (boost::iequals(name, "tileWidth")) {
		metadata.width = value.i;
	} else if (boost::iequals(name, "tileHeight")) {
		metadata.height = value.i;
	}
			
	return true;
}
		
void TileSetMetadataParserV1::error(const char *err) {
	LOG_FUNC("TilesetsMetadataListener: " << err, "TilesetsMetadataListener::error");
	metadata.valid = false;
}
		
bool TileSetMetadataParserV1::parserNewStruct(TCODParser*, const TCODParserStruct* str, const char* val) 
{ 
	if (boost::iequals(str->getName(), "tileset_data")) {
		metadata.name = val;
	}
	return true; 
}

bool TileSetMetadataParserV1::parserFlag(TCODParser*, const char*) { return true; }
bool TileSetMetadataParserV1::parserEndStruct(TCODParser*, const TCODParserStruct*, const char*) { return true; }
