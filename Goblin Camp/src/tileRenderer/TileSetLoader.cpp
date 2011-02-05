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

void TileSetLoader::SetupTilesetParser(TCODParser& parser) {
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
	tileTextureStruct->addProperty("blood", TCOD_TYPE_INT, false);

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
	tempConstruction(),
	tempSpell(),
	fireSprites(),
	fireFPS(15)
{
	SetupTilesetParser(parser);
}

TileSetLoader::~TileSetLoader() {
}

bool TileSetLoader::LoadTileSet(boost::filesystem::path path) {
	success = true;
	tileSetPath = path.branch_path();
	tileSet = boost::shared_ptr<TileSet>();

	parser.run(path.string().c_str(), this);

	if (!tileSet) {
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
		tempConstruction = TempConstruction();
	} else if (boost::iequals(str->getName(), "spell_sprite_data")) {
		tempSpell = TempSpell();
		currentSpriteSet = SS_SPELL;
	}

	return success;
}

bool TileSetLoader::parserFlag(TCODParser *parser,const char *name) {
	if (currentTexture) {
		switch (currentSpriteSet) {
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "connection_map")) {
				tempConstruction.connectionMapped = true;
			}
		}
	}
	return success;
}

bool TileSetLoader::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
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
				tileSet->SetBlood(Sprite(currentTexture, value.i));
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
			
			// Status Effects
			else if (boost::iequals(name, "default_hungry")) {
				tileSet->SetStatusSprite(HUNGER, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_thirsty")) {
				tileSet->SetStatusSprite(THIRST, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_panic")) {
				tileSet->SetStatusSprite(PANIC, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_concussion")) {
				tileSet->SetStatusSprite(CONCUSSION, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_drowsy")) {
				tileSet->SetStatusSprite(DROWSY, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_asleep")) {
				tileSet->SetStatusSprite(SLEEPING, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_poison")) {
				tileSet->SetStatusSprite(POISON, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_bleeding")) {
				tileSet->SetStatusSprite(BLEEDING, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_sluggish")) {
				tileSet->SetStatusSprite(BADSLEEP, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_rage")) {
				tileSet->SetStatusSprite(RAGE, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_eating")) {
				tileSet->SetStatusSprite(EATING, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_drinking")) {
				tileSet->SetStatusSprite(DRINKING, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_swimming")) {
				tileSet->SetStatusSprite(SWIM, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "default_burning")) {
				tileSet->SetStatusSprite(BURNING, Sprite(currentTexture, value.i));
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
			// TODO:
			// Record sprites, under construction sprites for later assembly once width/connection map flag is known
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tempConstruction.mainSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "underconstruction_sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					tempConstruction.underConstructionSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} 
			else if (boost::iequals(name, "openSprite")) {
				tempConstruction.openDoor = Sprite(currentTexture, value.i);
			} else if (boost::iequals(name, "width")) {
				tempConstruction.width = value.i;
			}
			break;
		case SS_SPELL:
			// TODO:
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

bool TileSetLoader::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	if (boost::iequals(str->getName(), "tile_texture_data")) {
		if (fireSprites.size() > 0) {
			tileSet->SetFireSprite(Sprite(currentTexture, fireSprites.begin(), fireSprites.end(), fireFPS));
		}
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
		ConstructionSpriteSet constructionSpriteSet = tempConstruction.Build(currentTexture);
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
			tileSet->SetDefaultSpellSpriteSet(tempSpell.Build(currentTexture));
		} else {
			tileSet->AddSpellSpriteSet(std::string(name), tempSpell.Build(currentTexture));
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
		tileSet->SetCursorSprites(type, Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)));
	} else if (size > 1) {
		tileSet->SetCursorSprites(type, Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 0)), Sprite(currentTexture, (intptr_t)TCOD_list_get(cursors, 1)));
	}
}

ConstructionSpriteSet TileSetLoader::TempConstruction::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	ConstructionSpriteSet spriteSet = ConstructionSpriteSet();
	if (connectionMapped) {
		if (mainSprites.size() > 0) {
			spriteSet.AddSprite(Sprite(currentTexture, mainSprites.begin(), mainSprites.end()));
		}
		if (underConstructionSprites.size() > 0) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, underConstructionSprites.begin(), underConstructionSprites.end()));
		}
	} else {
		for (std::vector<int>::iterator iter = mainSprites.begin(); iter != mainSprites.end(); ++iter) {
			spriteSet.AddSprite(Sprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = underConstructionSprites.begin(); iter != underConstructionSprites.end(); ++iter) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, *iter));
		}
		spriteSet.SetWidth(width);
	}
	spriteSet.SetOpenSprite(openDoor);
	return spriteSet;
}

SpellSpriteSet TileSetLoader::TempSpell::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	return SpellSpriteSet(Sprite(currentTexture, sprites.begin(), sprites.end(), fps));
}