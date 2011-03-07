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

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <libtcod.hpp>
#include <SDL.h>

#include "Coordinate.hpp"
#include "tileRenderer/TileSetTexture.hpp"

enum SpriteType
{
	SPRITE_Single = 0x0,
	SPRITE_Animated = 0x1,
	SPRITE_SimpleConnectionMap = 0x2,
	SPRITE_TwoLayerConnectionMap = 0x6, // Two layered contains Simple
	SPRITE_NormalConnectionMap = 0x8,
	SPRITE_ExtendedConnectionMap = 0x18, // Extended constains Normal
	SPRITE_ConnectionMap = 0x1E // Connection Map encompasses all variants
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
	int frameCount;

public:
	explicit Sprite();
	explicit Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	template <typename IterT> explicit Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate = 15, int frameCount = 1);
	~Sprite();

	bool Exists() const;
	bool IsConnectionMap() const;
	bool IsAnimated() const;

	// Standard Tile Drawing
	void Draw(SDL_Surface * dst, SDL_Rect * dstRect) const; 
	
	// Connection Map Drawing
	typedef boost::function<bool (Direction)> ConnectedFunction;
	typedef boost::function<int (Direction)> LayeredConnectedFunction;

	void Draw(ConnectedFunction, SDL_Surface * dst, SDL_Rect * dstRect) const;
	void Draw(int layer, LayeredConnectedFunction, SDL_Surface * dst, SDL_Rect * dstRect) const;

private:
	void DrawSimpleConnected(ConnectedFunction, SDL_Surface * dst, SDL_Rect * dstRect) const;
	inline int CurrentFrame() const { return (type & SPRITE_Animated) ? ((TCODSystem::getElapsedMilli() / frameTime) % frameCount) : 0; }
};

template <typename IterT> Sprite::Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate, int frames)
	: tiles(),
	  texture(tilesetTexture),
	  type(SPRITE_Single),
	  frameTime(1000 / frameRate),
	  frameCount(frames > 0 ? frames : 1)
{
	std::vector<int> indices;
	for(; start != end; ++start) {
		indices.push_back(*start);
	}

	// Assume all tiles are for animation if it isn't a connection map
	if (!connectionMap) {
		frameCount = indices.size();
		tiles.assign(indices.begin(), indices.end());
		type = (frameCount > 1) ? SPRITE_Animated : SPRITE_Single;
		return;
	}

	if (frameCount > indices.size()) {
		frameCount = indices.size();
	}
	int numTiles = indices.size() / frameCount;
	if (numTiles == 0) { 
		frameCount = 0;
		return;
	}
	
	switch (numTiles) {
		case 47:
			type = SPRITE_ExtendedConnectionMap; break;
		case 19:
			type = SPRITE_TwoLayerConnectionMap; break;
		case 16:
			type = SPRITE_NormalConnectionMap; break;
		case 5:
			type = SPRITE_SimpleConnectionMap; break;
		default:
			type = SPRITE_Single;
			numTiles = 1;
			break;
	}

	if (frameCount > 1) {
		type = static_cast<SpriteType>(type | SPRITE_Animated);
	}

	for (int tile = 0; tile < numTiles; ++tile) {
		for (int frame = 0; frame < frameCount; ++frame) {
			tiles.push_back(indices[tile + frame * numTiles]);
		}
	}

}