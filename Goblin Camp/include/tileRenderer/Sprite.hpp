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

#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include "Coordinate.hpp"
#include "tileRenderer/TileSetTexture.hpp"

enum SpriteType
{
	SPRITE_Single = 0x0,
	SPRITE_Animated = 0x1,
	SPRITE_SimpleConnectionMap = 0x2,
	SPRITE_NormalConnectionMap = 0x4,
	SPRITE_ExtendedConnectionMap = 0x8
};

/****************
/* Sprite
/* Description of a single tile that can be drawn. 
/* Supports animation and connection maps in addition to simple single titles.
/*
/* TODO: Animation supporting connection maps?
/****************/
class Sprite
{
private:
	std::vector<int> tiles;
	boost::shared_ptr<TileSetTexture> texture;
	SpriteType type;
	int frameTime;
	int width;

public:
	explicit Sprite();
	explicit Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	template <typename IterT> explicit Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, SpriteType type = SPRITE_ExtendedConnectionMap);
	template <typename IterT> explicit Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, int frameRate, SpriteType type = SPRITE_Animated);
	~Sprite();

	bool Exists() const;
	bool IsConnectionMap() const;
	bool IsAnimated() const;

	// Standard Tile Drawing
	void Draw(SDL_Surface * dst, SDL_Rect * dstRect) const; 
	
	// Connection Map Drawing
	typedef boost::function<bool (Direction)> ConnectedFunction;
	void Draw(ConnectedFunction, SDL_Surface * dst, SDL_Rect * dstRect) const;

private:
	void DrawSimpleConnected(ConnectedFunction, SDL_Surface * dst, SDL_Rect * dstRect) const;
};

template <typename IterT> Sprite::Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, SpriteType spriteType)
	: tiles(),
	  texture(tilesetTexture),
	  type(spriteType),
	  frameTime(15)
{
	for(; start != end; ++start) {
		tiles.push_back(*start);
	}
	if (tiles.size() <= 1) {
		type = SPRITE_Single;
	}
	if ((type & SPRITE_ExtendedConnectionMap) && tiles.size() < 47) {
		// Down grade to connection map
		type = static_cast<SpriteType>((type & ~SPRITE_ExtendedConnectionMap) | SPRITE_NormalConnectionMap);
	}
	if ((type & SPRITE_NormalConnectionMap) && tiles.size() < 16) {
		type = static_cast<SpriteType>((type & ~SPRITE_NormalConnectionMap) | SPRITE_SimpleConnectionMap);
	}
	if ((type & SPRITE_SimpleConnectionMap) && tiles.size() < 5) {
		type = static_cast<SpriteType>(type & ~SPRITE_SimpleConnectionMap);
	}
}

template <typename IterT> Sprite::Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, int frameRate, SpriteType spriteType)
	: tiles(),
	  texture(tilesetTexture),
	  type(spriteType),
	  frameTime(1000 / frameRate)
{
	for(; start != end; ++start) {
		tiles.push_back(*start);
	}
	if (tiles.size() <= 1) {
		type = SPRITE_Single;
	}
	if ((type & SPRITE_ExtendedConnectionMap) && tiles.size() < 47) {
		// Down grade to connection map
		type = static_cast<SpriteType>((type & ~SPRITE_ExtendedConnectionMap) | SPRITE_NormalConnectionMap);
	}
	if ((type & SPRITE_NormalConnectionMap) && tiles.size() < 16) {
		type = static_cast<SpriteType>((type & ~SPRITE_NormalConnectionMap) | SPRITE_SimpleConnectionMap);
	}
	if ((type & SPRITE_SimpleConnectionMap) && tiles.size() < 5) {
		type = static_cast<SpriteType>(type & ~SPRITE_SimpleConnectionMap);
	}
}