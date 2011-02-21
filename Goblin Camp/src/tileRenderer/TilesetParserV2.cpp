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

const char* TileSetParserV2::uninitialisedTilesetError = "tileset must be defined and tileWidth & tileHeight must be provided first";

namespace {
	void SetupTilesetParser(TCODParser& parser) {
		TCODParserStruct* creatureSpriteStruct = parser.newStructure("creature_sprite");
		creatureSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		creatureSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);
		creatureSpriteStruct->addFlag("equipmentMap");
		creatureSpriteStruct->addListProperty("weaponTypes", TCOD_TYPE_STRING, false);
		creatureSpriteStruct->addListProperty("armorTypes", TCOD_TYPE_STRING, false);
		creatureSpriteStruct->addListProperty("armourTypes", TCOD_TYPE_STRING, false);

		TCODParserStruct* natureObjectSpriteStruct = parser.newStructure("plant_sprite");
		natureObjectSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		natureObjectSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);

		TCODParserStruct* itemSpriteStruct = parser.newStructure("item_sprite");
		itemSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		itemSpriteStruct->addFlag("drawWhenWielded");
		itemSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);

		TCODParserStruct* spellSpriteStruct = parser.newStructure("spell_sprite");
		spellSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		spellSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);

		TCODParserStruct* constructionSpriteStruct = parser.newStructure("construction_sprite");
		constructionSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		constructionSpriteStruct->addListProperty("underconstructionSprites", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addProperty("openSprite", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addListProperty("unreadyTrapSprites", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addProperty("width", TCOD_TYPE_INT, false);
		constructionSpriteStruct->addFlag("connectionMap");
	
		TCODParserStruct* statusEffectSpriteStruct = parser.newStructure("status_effect_sprite");
		statusEffectSpriteStruct->addListProperty("sprites", TCOD_TYPE_INT, true);
		statusEffectSpriteStruct->addProperty("flashRate", TCOD_TYPE_INT, false);
		statusEffectSpriteStruct->addProperty("fps", TCOD_TYPE_INT, false);
		statusEffectSpriteStruct->addFlag("alwaysOn");

		TCODParserStruct* tileTextureStruct = parser.newStructure("texture");

		// Terrain tile types
		tileTextureStruct->addProperty("unknownTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("grassTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("ditchTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("riverbedTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("bogTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("rockTerrain", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("mudTerrain", TCOD_TYPE_INT, false);

		tileTextureStruct->addListProperty("details", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("detailRange", TCOD_TYPE_INT, false);

		// Terrain modifiers
		tileTextureStruct->addListProperty("water", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("minorFilth", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("majorFilth", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("marker", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("markerFPS", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("blood", TCOD_TYPE_INT, false);

		// Overlays
		tileTextureStruct->addProperty("nonTerritory", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("territory", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("marked", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("corruption", TCOD_TYPE_INT, false);

		tileTextureStruct->addProperty("defaultUnderconstruction", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("fire", TCOD_TYPE_INT, false);
		tileTextureStruct->addProperty("fireFPS", TCOD_TYPE_INT, false);

		// Cursors
		tileTextureStruct->addListProperty("defaultTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("constructionTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("stockpileTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("treeFellingTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("harvestTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("orderTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("treeTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("dismantleTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("undesignateTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("bogTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("digTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("addTerritoryTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("removeTerritoryTileHighlight", TCOD_TYPE_INT, false);
		tileTextureStruct->addListProperty("gatherTileHighlight", TCOD_TYPE_INT, false);	
	
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

		TCODParserStruct* tilesetStruct = parser.newStructure("tileset");
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
	markerFrames(),
	markerFPS(15),
	fireFrames(),
	fireFPS(15),
	constructionFactory(),
	animSpriteFactory(),
	statusEffectFactory(),
	npcSpriteFactory(),
	itemSprite()
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
	tileSetPath = dataFilePath.parent_path();
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
	tileSetPath = dataFilePath.parent_path();
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
	// Root element for main tileset data files
	if (boost::iequals(str->getName(), "tileset"))
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

	// Root element for tileset extension data files
	if (boost::iequals(str->getName(), "tileset_extension"))
	{
		readTexture = true;
		return success;
	}

	if (!readTexture)
		return success;

	// Texture structure
	if (boost::iequals(str->getName(), "texture")) {
		currentTexture = boost::shared_ptr<TileSetTexture>(new TileSetTexture(tileSetPath / name, tileWidth, tileHeight));
		markerFrames.clear();
		markerFPS = 15;
		fireFrames.clear();
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
	if (boost::iequals(str->getName(), "creature_sprite")) {
		npcSpriteFactory.Reset();
		currentSpriteSet = SS_NPC;
	} else if (boost::iequals(str->getName(), "plant_sprite")) {
		animSpriteFactory = AnimatedSpriteFactory();
		currentSpriteSet = SS_NATURE;
	} else if (boost::iequals(str->getName(), "item_sprite")) {
		itemSprite = ItemSprite();
		animSpriteFactory = AnimatedSpriteFactory();
		currentSpriteSet = SS_ITEM;
	} else if (boost::iequals(str->getName(), "construction_sprite")) {
		currentSpriteSet = SS_CONSTRUCTION;
		constructionFactory = ConstructionSpriteFactory();
	} else if (boost::iequals(str->getName(), "spell_sprite")) {
		animSpriteFactory = AnimatedSpriteFactory();
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
			if (boost::iequals(name, "connectionMap")) {
				constructionFactory.connectionMapped = true;
			}
			break;
		case SS_STATUS_EFFECT:
			if (boost::iequals(name, "alwaysOn")) {
				statusEffectFactory.SetAlwaysOn(true);
			}
			break;
		case SS_ITEM:
			if (boost::iequals(name, "drawWhenWielded")) {
				itemSprite.renderWhenWielded = true;
			}
			break;
		case SS_NPC:
			if (boost::iequals(name, "equipmentMap")) {
				npcSpriteFactory.SetEquipmentMap(true);
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
			if (boost::iequals(name, "unknownTerrain")) {
				tileSet->SetTerrain(TILENONE, Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "grassTerrain")) {
				tileSet->SetTerrain(TILEGRASS, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "ditchTerrain")) {
				tileSet->SetTerrain(TILEDITCH, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "riverbedTerrain")) {
				tileSet->SetTerrain(TILERIVERBED, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "bogTerrain")) {
				tileSet->SetTerrain(TILEBOG, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "rockTerrain")) {
				tileSet->SetTerrain(TILEROCK, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "mudTerrain")) {
				tileSet->SetTerrain(TILEMUD, Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} 

			else if (boost::iequals(name, "details")) {
				for (intptr_t * iter = (intptr_t*)TCOD_list_begin(value.list); iter != (intptr_t*)TCOD_list_end(value.list); ++iter) {
					tileSet->AddDetailSprite(Sprite(currentTexture, *iter));
				}
			} else if (boost::iequals(name, "detailRange")) {
				tileSet->SetDetailRange(value.i);
			}
			
			// Terrain Modifiers
			else if (boost::iequals(name, "water")) {
				tileSet->SetWater(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "minorFilth")) {
				tileSet->SetFilthMinor(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "majorFilth")) {
				tileSet->SetFilthMajor(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} else if (boost::iequals(name, "marker")) {
				for (intptr_t * iter = (intptr_t*)TCOD_list_begin(value.list); iter != (intptr_t*)TCOD_list_end(value.list); ++iter) {
					markerFrames.push_back(*iter);
				}
			} else if (boost::iequals(name, "markerFPS")) {
				markerFPS = value.i;
			} else if (boost::iequals(name, "blood")) {
				tileSet->SetBlood(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			} 
			
			// Overlays
			else if (boost::iequals(name, "nonTerritory")) {
				tileSet->SetNonTerritoryOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "territory")) {
				tileSet->SetTerritoryOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "marked")) {
				tileSet->SetMarkedOverlay(Sprite(currentTexture, value.i));
			} else if (boost::iequals(name, "corruption")) {
				tileSet->SetCorruption(Sprite(currentTexture, (intptr_t*)TCOD_list_begin(value.list), (intptr_t*)TCOD_list_end(value.list)));
			}

			// Cursors
			else if (boost::iequals(name, "defaultTileHighlight")) {
				SetCursorSprites(Cursor_None, value.list);
			} else if (boost::iequals(name, "constructionTileHighlight")) {
				SetCursorSprites(Cursor_Construct, value.list);
			} else if (boost::iequals(name, "stockpileTileHighlight")) {
				SetCursorSprites(Cursor_Stockpile, value.list);
			} else if (boost::iequals(name, "treeFellingTileHighlight")) {
				SetCursorSprites(Cursor_TreeFelling, value.list);
			} else if (boost::iequals(name, "harvestTileHighlight")) {
				SetCursorSprites(Cursor_Harvest, value.list);
			} else if (boost::iequals(name, "orderTileHighlight")) {
				SetCursorSprites(Cursor_Order, value.list);
			} else if (boost::iequals(name, "treeTileHighlight")) {
				SetCursorSprites(Cursor_Tree, value.list);
			} else if (boost::iequals(name, "dismantleTileHighlight")) {
				SetCursorSprites(Cursor_Dismantle, value.list);
			} else if (boost::iequals(name, "undesignateTileHighlight")) {
				SetCursorSprites(Cursor_Undesignate, value.list);
			} else if (boost::iequals(name, "bogTileHighlight")) {
				SetCursorSprites(Cursor_Bog, value.list);
			} else if (boost::iequals(name, "digTileHighlight")) {
				SetCursorSprites(Cursor_Dig, value.list);
			} else if (boost::iequals(name, "addTerritoryTileHighlight")) {
				SetCursorSprites(Cursor_AddTerritory, value.list);
			} else if (boost::iequals(name, "removeTerritoryTileHighlight")) {
				SetCursorSprites(Cursor_RemoveTerritory, value.list);
			} else if (boost::iequals(name, "gatherTileHighlight")) {
				SetCursorSprites(Cursor_Gather, value.list);
			} 
						
			else if (boost::iequals(name, "defaultUnderconstruction")) {
				tileSet->SetDefaultUnderConstructionSprite(Sprite(currentTexture, value.i));
			}

			// Capture fps/sprite set and build fire from this later
			else if (boost::iequals(name, "fireFPS")) {
				fireFPS = value.i;
			}
			else if (boost::iequals(name, "fire")) {
				for (intptr_t * iter = (intptr_t*)TCOD_list_begin(value.list); iter != (intptr_t*)TCOD_list_end(value.list); ++iter) {
					fireFrames.push_back(*iter);
				}
			}
			break;
		case SS_NPC: 
			if (boost::iequals(name, "sprites")) {
				for (intptr_t * iter = (intptr_t*)TCOD_list_begin(value.list); iter != (intptr_t*)TCOD_list_end(value.list); ++iter) {
					npcSpriteFactory.AddSpriteFrame(*iter);
				}
			} else if (boost::iequals(name, "fps")) {
				npcSpriteFactory.SetFPS(value.i);
			} else if (boost::iequals(name, "weaponTypes")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i) {
					npcSpriteFactory.AddWeaponType(std::string((char*)TCOD_list_get(value.list, i)));
				}
			} else if (boost::iequals(name, "armourTypes") || boost::iequals(name, "armorTypes")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i) {
					npcSpriteFactory.AddArmourType(std::string((char*)TCOD_list_get(value.list, i)));
				}
			}

			break;
		case SS_ITEM:
		case SS_NATURE:
		case SS_SPELL:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					animSpriteFactory.sprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "fps")) {
				animSpriteFactory.fps = value.i;
			}
			break;	
		case SS_CONSTRUCTION:
			if (boost::iequals(name, "sprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.mainSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "underConstructionSprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.underConstructionSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "unreadyTrapSprites")) {
				for (int i = 0; i < TCOD_list_size(value.list); ++i)
					constructionFactory.unreadyTrapSprites.push_back((intptr_t)TCOD_list_get(value.list, i));
			} else if (boost::iequals(name, "openSprite")) {
				constructionFactory.openDoor = Sprite(currentTexture, value.i);
			} else if (boost::iequals(name, "width")) {
				constructionFactory.width = value.i;
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
	} else if (boost::iequals(str->getName(), "texture")) {
		if (fireFrames.size() > 0) {
			tileSet->SetFireSprite(Sprite(currentTexture, fireFrames.begin(), fireFrames.end(), fireFPS));
		}
		if (markerFrames.size() > 0) {
			tileSet->SetMarker(Sprite(currentTexture, markerFrames.begin(), markerFrames.end(), markerFPS));
		}
		currentTexture = boost::shared_ptr<TileSetTexture>();
		return success;
	} 
	
	if (!readTexture)
		return success;
	
	if (boost::iequals(str->getName(), "creature_sprite")) {
		if (name == 0) {
			tileSet->SetDefaultNPCSprite(npcSpriteFactory.Build(currentTexture));
		} else {
			tileSet->AddNPCSprite(std::string(name), npcSpriteFactory.Build(currentTexture));
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "plant_sprite")) {
		NatureObjectSpriteSet plantSprite;
		plantSprite.tile = animSpriteFactory.Build(currentTexture);
		if (name == 0) {
			tileSet->SetDefaultNatureObjectSpriteSet(plantSprite);
		} else {
			tileSet->AddNatureObjectSpriteSet(std::string(name), plantSprite);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "item_sprite")) {
		itemSprite.tile = animSpriteFactory.Build(currentTexture);
		if (name == 0) {
			tileSet->SetDefaultItemSprite(itemSprite);
		} else {
			tileSet->AddItemSprite(std::string(name), itemSprite);
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "construction_sprite")) {
		ConstructionSpriteSet constructionSpriteSet = constructionFactory.Build(currentTexture);
		if (constructionSpriteSet.IsValid()) {
			if (name == 0) {
				tileSet->SetDefaultConstructionSpriteSet(constructionSpriteSet);
			} else {
				tileSet->AddConstructionSpriteSet(std::string(name), constructionSpriteSet);
			}
		} else {
			if (name == 0) {
				LOG("Skipping invalid construction sprite: default");
			} else {
				LOG("Skipping invalid construction sprite: " << std::string(name));
			}
		}
		currentSpriteSet = SS_NONE;
	} else if (boost::iequals(str->getName(), "spell_sprite")) {
		if (name == 0) {
			tileSet->SetDefaultSpellSpriteSet(SpellSpriteSet(animSpriteFactory.Build(currentTexture)));
		} else {
			tileSet->AddSpellSpriteSet(std::string(name), SpellSpriteSet(animSpriteFactory.Build(currentTexture)));
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

Sprite TileSetParserV2::AnimatedSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	return Sprite(currentTexture, sprites.begin(), sprites.end(), fps);
}


TileSetMetadataParserV2::TileSetMetadataParserV2()
	: metadata(),
	  parser()
{
	SetupTilesetParser(parser);
	
}

TileSetMetadata TileSetMetadataParserV2::Run(boost::filesystem::path dataFilePath) {
	metadata = TileSetMetadata(dataFilePath.parent_path());
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
	if (boost::iequals(str->getName(), "tileset")) {
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
	location = path.parent_path();
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
