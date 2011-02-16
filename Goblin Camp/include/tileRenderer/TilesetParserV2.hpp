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

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <libtcod.hpp>
#include "tileRenderer/TileSetLoader.hpp"
#include "tileRenderer/TileSet.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/NPCSpriteSet.hpp"
#include "tileRenderer/ItemSpriteSet.hpp"
#include "tileRenderer/ConstructionSpriteSet.hpp"
#include "tileRenderer/SpellSpriteSet.hpp"
#include "tileRenderer/StatusEffectSprite.hpp"

class TileSetParserV2 : public ITCODParserListener
{
public:
	explicit TileSetParserV2();
	~TileSetParserV2();

	boost::shared_ptr<TileSet> Run(boost::filesystem::path tileSetPath);

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name);
	bool parserFlag(TCODParser *parser,const char *name);
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value);
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name);
	void error(const char *msg);

private:
	TCODParser parser;

	boost::shared_ptr<TileSet> tileSet;
	bool success;

	enum SpriteSet
	{
		SS_NONE,
		SS_NPC,
		SS_ITEM,
		SS_NATURE,
		SS_CONSTRUCTION,
		SS_SPELL,
		SS_STATUS_EFFECT
	};
	TileSetParserV2::SpriteSet currentSpriteSet;
	
	// Path where textures are found
	boost::filesystem::path tileSetPath;
	boost::shared_ptr<TileSetTexture> currentTexture;

	std::string tileSetName;
	int tileWidth;
	int tileHeight;	

	std::vector<int> fireSprites;
	int fireFPS;

	struct ConstructionSpriteFactory {
		std::vector<int> mainSprites;
		std::vector<int> underConstructionSprites;
		std::vector<int> unreadyTrapSprites;
		bool connectionMapped;
		int width;
		Sprite openDoor;

		ConstructionSpriteFactory() 
		: mainSprites(), 
		  underConstructionSprites(), 
		  unreadyTrapSprites(),
		  connectionMapped(false), 
		  width(1), 
		  openDoor() {}

		ConstructionSpriteSet Build(boost::shared_ptr<TileSetTexture> currentTexture);
	};
	ConstructionSpriteFactory constructionFactory;

	struct SpellSpriteFactory {
		std::vector<int> sprites;
		int fps;

		SpellSpriteFactory() : sprites(), fps(15) {}

		SpellSpriteSet Build(boost::shared_ptr<TileSetTexture> currentTexture);
	};
	SpellSpriteFactory spellFactory;

	StatusEffectSpriteFactory statusEffectFactory;
		
	NPCSpriteSet npcSpriteSet;
	NatureObjectSpriteSet natureObjectSpriteSet;
	ItemSpriteSet itemSpriteSet;
	
	static const char * uninitialisedTilesetError;
	
	void SetCursorSprites(CursorType type, TCOD_list_t cursors);
};

class TileSetMetadataParserV2 : public ITCODParserListener {
public:
	TileSetMetadataParserV2();

	TileSetMetadata Run(boost::filesystem::path path);

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name);
	bool parserFlag(TCODParser *parser,const char *name);
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value);
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name);
	void error(const char *msg);

private:
	TCODParser parser;
	TileSetMetadata metadata;

};