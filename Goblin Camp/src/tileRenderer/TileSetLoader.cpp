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
#include "tileRenderer/TilesetParserV1.hpp"
#include "Logger.hpp"

TileSetMetadata::TileSetMetadata()
	: path(),
	  name(),
	  author(),
	  version(),
	  description(),
	  width(0),
	  height(0),
	  valid(false)
{
}

TileSetMetadata::TileSetMetadata(boost::filesystem::path tilesetPath)
	: path(tilesetPath),
	  name(),
	  author(),
	  version(),
	  description(),
	  width(0),
	  height(0),
	  valid(false)
{
}

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

boost::shared_ptr<TileSet> TileSetLoader::LoadTileSet(boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	fs::path tileSetV1Path = path / "tileset.dat";

	if (fs::exists(tileSetV1Path))
	{
		TCODParser parser = TCODParser();
		SetupTilesetParser(parser);
		TileSetParserV1 parserListener(path);

		parser.run(tileSetV1Path.string().c_str(), &parserListener);
		if (parserListener.Success())
		{
			return parserListener.GetTileSet();
		}
	}
	return boost::shared_ptr<TileSet>();
}

TileSetMetadata TileSetLoader::LoadTileSetMetadata(boost::filesystem::path path) {
	namespace fs = boost::filesystem;
	TileSetMetadata metadata(path);

	fs::path tileSetV1Path = path / "tileset.dat";
	if (fs::exists(tileSetV1Path))
	{
		TCODParser parser = TCODParser();
		SetupTilesetParser(parser);
		TileSetMetadataParserListener parserListener;

		parser.run(tileSetV1Path.string().c_str(), &parserListener);
		return parserListener.GetMetadata();
	}
	return metadata;
}


