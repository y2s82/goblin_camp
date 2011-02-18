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

#include "tileRenderer/TilesetParserV2.hpp"
#include "Logger.hpp"

const char* TileSetParserV2::uninitialisedTilesetError = "tileset_data must be defined and tileWidth & tileHeight must be provided first";

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
		constructionSpriteStruct->addListProperty("unreadytrap_sprites", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addProperty("width", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addFlag("connection_map");
	
		TCODParserStruct* statusEffectSpriteStruct = parser.newStructure("status_effect_sprite");
		statusEffectSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		statusEffectSpriteStruct->addProperty("flashRate", TCOD_TYPE_INT, false);
		statusEffectSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);
		statusEffectSpriteStruct->addFlag("alwaysOn");

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
		tileTextureStruct->addStructure(statusEffectSpriteStruct);

		TCODParserStruct* tilesetExtensionStruct = parser.newStructure("tileset_extension");
		tilesetExtensionStruct->addProperty("tileWidth", TCOD_TYPE_INT, true);
		tilesetExtensionStruct->addProperty("tileHeight", TCOD_TYPE_INT, true);
		tilesetExtensionStruct->addStructure(tileTextureStruct);

		TCODParserStruct* tilesetStruct = parser.newStructure("tileset_data");
		tilesetStruct->addProperty("tileWidth", TCOD_TYPE_INT, true);
		tilesetStruct->addProperty("tileHeight", TCOD_TYPE_INT, true);
		tilesetStruct->addProperty("author", TCOD_TYPE_STRING, false);
		tilesetStruct->addProperty("version", TCOD_TYPE_STRING, false);
		tilesetStruct->addProperty("description", TCOD_TYPE_STRING, false);
		tilesetStruct->addStructure(tileTextureStruct);
	}
}

TileSetParserV2::TileSetParserV2() :
	parser(),
	readTexture(false),
	extendingExisting(false),
	tileSet(),
	success(true),
	currentSpriteSet(SS_NONE),
	tileSetPath(),
	currentTexture(),
	tileSetName(),
	tileWidth(-1),
	tileHeight(-1),
	fireSprites(),
	fireFPS(15),
	constructionFactory(),
	spellFactory(),
	statusEffectFactory(),
	npcSpriteSet(),
	natureObjectSpriteSet(),
	itemSpriteSet()
{
	SetupTilesetParser(parser);
}

TileSetParserV2::~TileSetParserV2() {
	
}

boost::shared_ptr<TileSet> TileSetParserV2::Run(boost::filesystem::path dataFilePath) {
	tileSetName = "";
	tileWidth = -1;
	tileHeight = -1;
	currentTexture = boost::shared_ptr<TileSetTexture>();
	tileSetPath = dataFilePath.branch_path();
	statusEffectFactory.Reset();
	success = true;
	readTexture = false;

	parser.run(dataFilePath.string().c_str(), this);

	boost::shared_ptr<TileSet> result = tileSet;
	tileSet.reset();
	if (success)
		return result;
	return boost::shared_ptr<TileSet>();
}

void TileSetParserV2::Modify(boost::shared_ptr<TileSet> target, boost::filesystem::path dataFilePath) {
	tileWidth = target->TileWidth();
	tileHeight = target->TileHeight();
	currentTexture = boost::shared_ptr<TileSetTexture>();
	tileSetPath = dataFilePath.branch_path();
	statusEffectFactory.Reset();
	success = true;
	readTexture = false;
	tileSet = target;
	extendingExisting = true;

	parser.run(dataFilePath.string().c_str(), this);

	extendingExisting = false;
	tileSet.reset();
}

void TileSetParserV2::SetCursorSprites(CursorType type, TCOD_list_t cursors) {
	int size = TCOD_list_size(cursors);
	if (size == 1) {
		tileSet->SetCursorSprites(type, Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)));
	} else if (size > 1) {
		tileSet->SetCursorSprites(type, Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)), Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 1)));
	}
}

bool TileSetParserV2::parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
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

	// Tileset extension
	if (boost::iequals(str->getName(), "tileset_extension"))
	{
		readTexture = true;
		return success;
	}

	if (!readTexture)
		return success;

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
		npcSpriteSet = NPCSpriteSet();
		currentSpriteSet = SS_NPC;
	} else if (boost::iequals(str->getName(), "plant_sprite_data")) {
		natureObjectSpriteSet = NatureObjectSpriteSet();
		currentSpriteSet = SS_NATURE;
	} else if (boost::iequals(str->getName(), "item_sprite_data")) {
		itemSpriteSet = ItemSpriteSet();
		currentSpriteSet = SS_ITEM;
	} else if (boost::iequals(str->getName(), "construction_sprite_data")) {
		currentSpriteSet = SS_CONSTRUCTION;
		constructionFactory = ConstructionSpriteFactory();
	} else if (boost::iequals(str->getName(), "spell_sprite_data")) {
		spellFactory = SpellSpriteFactory();
		currentSpriteSet = SS_SPELL;
	} else if (boost::iequals(str->getName(), "status_effect_sprite")) {
		statusEffectFactory.Reset();
		currentSpriteSet = SS_STATUS_EFFECT;
	}

	return success;
}

bool TileSetParserV2::parserFlag(TCODParser *parser,const char *name) {
	if (currentTexture) {
		switch (currentSpriteSet) {
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "connection_map")) {
				constructionFactory.connectionMapped = true;
			}
			break;
		case SS_STATUS_EFFECT:
			if (boost::iequals(name, "alwaysOn")) {
				statusEffectFactory.SetAlwaysOn(true);
			}
			break;
		}
	}
	return success;
}

bool TileSetParserV2::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
	// Tile Texture Properties
	if (currentTexture) {
		switch (currentSpriteSet) {
		case SS_NONE:
			// Terrain
			if (boost::iequals(name, "unknown_terrain")) {
				tileSet->SetTerrain(TILENONE, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "grass_terrain")) {
				tileSet->SetTerrain(TILEGRASS, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "ditch_terrain")) {
				tileSet->SetTerrain(TILEDITCH, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "riverbed_terrain")) {
				tileSet->SetTerrain(TILERIVERBED, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "bog_terrain")) {
				tileSet->SetTerrain(TILEBOG, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "rock_terrain")) {
				tileSet->SetTerrain(TILEROCK, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "mud_terrain")) {
				tileSet->SetTerrain(TILEMUD, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} 
			
			// Terrain Modifiers
			else if (boost::iequals(name, "water")) {
				tileSet->SetWater(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "minor_filth")) {
				tileSet->SetFilthMinor(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "major_filth")) {
				tileSet->SetFilthMajor(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "marker")) {
				tileSet->SetMarker(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "blood")) {
				tileSet->SetBlood(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} 
			
			// Overlays
			else if (boost::iequals(name, "non_territory")) {
				tileSet->SetNonTerritoryOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "territory")) {
				tileSet->SetTerritoryOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "marked")) {
				tileSet->SetMarkedOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "corruption")) {
				tileSet->SetCorruption(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
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
						
			else if (boost::iequals(name, "default_underconstruction")) {
				tileSet->SetDefaultUnderConstructionSprite(Sprite(currentTexture, value.i));
			}

			// TODO: Capture fps/sprite set and build fire from this later
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
				npcSpriteSet.tile = Sprite(currentTexture, value.i);
			}
			break;
		case SS_ITEM:
			if (boost::iequals(name, "sprite")) {
				itemSpriteSet.tile = Sprite(currentTexture, value.i);
			}
			break;
		case SS_NATURE:
			if (boost::iequals(name, "sprite")) {
				natureObjectSpriteSet.tile = Sprite(currentTexture, value.i);
			}
			break;
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.mainSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "underconstruction_sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.underConstructionSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "unreadytrap_sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.unreadyTrapSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "openSprite")) {
				constructionFactory.openDoor = Sprite(currentTexture, value.i);
			} else if (boost::iequals(name, "width")) {
				constructionFactory.width = value.i;
			}
			break;
		case SS_SPELL:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					spellFactory.sprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "fps")) {
				spellFactory.fps = value.i;
			}
			break;
		case SS_STATUS_EFFECT:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					statusEffectFactory.AddSpriteFrame((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "fps")) {
				statusEffectFactory.SetFPS(value.i);
			} else if (boost::iequals(name, "flashRate")) {
				statusEffectFactory.SetFlashRate(value.i);
			}
			break;
		}	
		return success;
	}

	// Mandatory tile set properties
	if (boost::iequals(name, "tileWidth")) {
		if (extendingExisting) {
			if (tileWidth != value.i) {
				readTexture = false;
			}
		} else {
			tileWidth = value.i;
			if (tileWidth != -1 && tileHeight != -1)  {
				tileSet = boost::shared_ptr<TileSet>(new TileSet(tileSetName, tileWidth, tileHeight));
				readTexture = true;
			}
		}
		return success;
	} else if (boost::iequals(name, "tileHeight")) {
		if (extendingExisting) {
			if (tileHeight != value.i) {
				readTexture = false;
			}
		} else {
			tileHeight = value.i;
			if (tileWidth != -1 && tileHeight != -1)  {
				tileSet = boost::shared_ptr<TileSet>(new TileSet(tileSetName, tileWidth, tileHeight));
				readTexture = true;
			}
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

bool TileSetParserV2::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	if (boost::iequals(str->getName(), "tileset_extension")) {
		readTexture = false;
		return success;
	} else if (boost::iequals(str->getName(), "tile_texture_data")) {
		if (fireSprites.size() > 0) {
			tileSet->SetFireSprite(Sprite(currentTexture, fireSprites.begin(), fireSprites.end(), fireFPS));
		}
		currentTexture = boost::shared_ptr<TileSetTexture>();
		return success;
	} 
	
	if (!readTexture)
		return success;
	
	if (boost::iequals(str->getName(), "creature_sprite_data")) {
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
		ConstructionSpriteSet constructionSpriteSet = constructionFactory.Build(currentTexture);
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
	} else if (boost::iequals(str->getName(), "spell_sprite_data")) {
		if (name == 0) {
			tileSet->SetDefaultSpellSpriteSet(spellFactory.Build(currentTexture));
		} else {
			tileSet->AddSpellSpriteSet(std::string(name), spellFactory.Build(currentTexture));
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "status_effect_sprite")) {
		StatusEffectType type = StatusEffect::StringToStatusEffectType(name);
		if (type == HUNGER && !boost::iequals(name, "hunger")) {
			LOG("Unknown Status Effect: " << std::string(name));
			return success;
		}
		tileSet->SetStatusSprite(type, statusEffectFactory.Build(currentTexture));
	}
		
	return success;
}

void TileSetParserV2::error(const char *msg) {
	LOG(msg);
	success = false;
}

ConstructionSpriteSet TileSetParserV2::ConstructionSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	ConstructionSpriteSet spriteSet = ConstructionSpriteSet();
	if (connectionMapped) {
		if (mainSprites.size() > 0) {
			spriteSet.AddSprite(Sprite(currentTexture, mainSprites.begin(), mainSprites.end()));
		}
		if (underConstructionSprites.size() > 0) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, underConstructionSprites.begin(), underConstructionSprites.end()));
		}
		if (unreadyTrapSprites.size() > 0) {
			spriteSet.AddUnreadyTrapSprite(Sprite(currentTexture, unreadyTrapSprites.begin(), unreadyTrapSprites.end()));
		}
	} else {
		for (std::vector<int>::iterator iter = mainSprites.begin(); iter != mainSprites.end(); ++iter) {
			spriteSet.AddSprite(Sprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = underConstructionSprites.begin(); iter != underConstructionSprites.end(); ++iter) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = unreadyTrapSprites.begin(); iter != unreadyTrapSprites.end(); ++iter) {
			spriteSet.AddUnreadyTrapSprite(Sprite(currentTexture, *iter));
		}
		spriteSet.SetWidth(width);
	}
	spriteSet.SetOpenSprite(openDoor);
	return spriteSet;
}

SpellSpriteSet TileSetParserV2::SpellSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	return SpellSpriteSet(Sprite(currentTexture, sprites.begin(), sprites.end(), fps));
}


TileSetMetadataParserV2::TileSetMetadataParserV2()
	: metadata(),
	  parser()
{
	SetupTilesetParser(parser);
	
}

TileSetMetadata TileSetMetadataParserV2::Run(boost::filesystem::path dataFilePath) {
	metadata = TileSetMetadata(dataFilePath.branch_path());
	metadata.valid = true;

	parser.run(dataFilePath.string().c_str(), this);

	return metadata;
}

bool TileSetMetadataParserV2::parserProperty(TCODParser*, const char *name, TCOD_value_type_t, TCOD_value_t value) {
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
		
void TileSetMetadataParserV2::error(const char *err) {
	LOG_FUNC("TileSetMetadataParserV2: " << err, "TileSetMetadataParserV2::error");
	metadata.valid = false;
}
		
bool TileSetMetadataParserV2::parserNewStruct(TCODParser*, const TCODParserStruct* str, const char* val) 
{ 
	if (boost::iequals(str->getName(), "tileset_data")) {
		metadata.name = val;
	}
	return true; 
}

bool TileSetMetadataParserV2::parserFlag(TCODParser*, const char*) { return true; }
bool TileSetMetadataParserV2::parserEndStruct(TCODParser*, const TCODParserStruct*, const char*) { return true; }

TileSetModMetadataParserV2::TileSetModMetadataParserV2() 
	: parser(),
      metadata() {
		  SetupTilesetParser(parser);
}

std::list<TilesetModMetadata> TileSetModMetadataParserV2::Run(boost::filesystem::path path) {
	metadata.clear();
	location = path.branch_path();
	parser.run(path.string().c_str(), this);
	return metadata;
}

bool TileSetModMetadataParserV2::parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
	if (boost::iequals(str->getName(), "tileset_extension")) {
		metadata.push_back(TilesetModMetadata(location));
	}
	return true;
}

bool TileSetModMetadataParserV2::parserFlag(TCODParser *parser,const char *name) {
	return true;
}

bool TileSetModMetadataParserV2::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
	if (boost::iequals(name, "tileWidth")) {
		metadata.back().width = value.i;
	} else if (boost::iequals(name, "tileHeight")) {
		metadata.back().height = value.i;
	}
	return true;
}

bool TileSetModMetadataParserV2::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	return true;
}

void TileSetModMetadataParserV2::error(const char *msg) {
	LOG_FUNC("TileSetModMetadataParserV2: " << msg, "TileSetModMetadataParserV2::error");
}
