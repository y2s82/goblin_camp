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

#include <boost/noncopyable.hpp>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "Coordinate.hpp"
#include <libtcod.hpp>
#include "tileRenderer/Corner.hpp"


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

class Sprite : private boost::noncopyable
{
public:
	virtual ~Sprite() = 0;

	bool IsConnectionMap() const;
	bool IsTwoLayeredConnectionMap() const;
	bool IsAnimated() const;

	// Connection Map Drawing
	typedef boost::function<bool (Direction)> ConnectedFunction;
	typedef boost::function<int (Direction)> LayeredConnectedFunction;

	// Standard Tile Drawing
	void Draw(int screenX, int screenY) const; 
	void Draw(int screenX, int screenY, ConnectedFunction) const;
	void Draw(int screenX, int screenY, int connectionLayer, LayeredConnectedFunction) const;

protected:
	explicit Sprite();
	explicit Sprite(int tile);
	template <typename IterT> explicit Sprite(IterT start, IterT end, bool connectionMap, int frameRate = 15, int frameCount = 1);

	std::vector<int> tiles;
	SpriteType type;
	int frameTime;
	int frameCount;

	virtual void DrawInternal(int screenX, int screenY, int tile) const = 0;
	virtual void DrawInternal(int screenX, int screenY, int tile, Corner corner) const = 0;

private:
	void DrawSimpleConnected(int screenX, int screenY, Sprite::ConnectedFunction) const;
	inline int CurrentFrame() const { return (type & SPRITE_Animated) ? ((TCODSystem::getElapsedMilli() / frameTime) % frameCount) : 0; }

};

template <typename IterT> Sprite::Sprite(IterT start, IterT end, bool connectionMap, int frameRate, int frames)
	: tiles(),
	  type(SPRITE_Single),
	  frameTime(1000 / frameRate),
	  frameCount(frames > 0 ? frames : 1)
{
	std::vector<int> indices;
	for(; start != end; ++start) {
		indices.push_back(*start);
	}
	if (indices.size() == 0) {
		return;
	}

	// Assume all tiles are for animation if it isn't a connection map
	if (!connectionMap) {
		frameCount = indices.size();
		tiles.assign(indices.begin(), indices.end());
		type = (frameCount > 1) ? SPRITE_Animated : SPRITE_Single;
		return;
	}

	if (frameCount > static_cast<int>(indices.size())) {
		frameCount = static_cast<int>(indices.size());
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

class SpritePtr {
public:
	explicit SpritePtr() : ptr() {}
	explicit SpritePtr(Sprite * sprite) : ptr(sprite) {}

	Sprite * get() { return ptr.get(); }
	// bool Exists() const { return ptr; }
        bool Exists() const { return true; } // FIXME

	void Draw(int screenX, int screenY) const {	if (ptr) ptr->Draw(screenX, screenY); }; 

	void Draw(int screenX, int screenY, Sprite::ConnectedFunction connectedFunction) const { 
		if (ptr) ptr->Draw(screenX, screenY, connectedFunction); 
	}

	void Draw(int screenX, int screenY, int connectionLayer, Sprite::LayeredConnectedFunction connectedFunction) const {
		if (ptr) ptr->Draw(screenX, screenY, connectionLayer, connectedFunction);
	}

	bool IsConnectionMap() const { return ptr && ptr->IsConnectionMap(); }
	bool IsTwoLayeredConnectionMap() const { return ptr && ptr->IsTwoLayeredConnectionMap(); }
	bool IsAnimated() const { return ptr && ptr->IsAnimated(); }

private:
	boost::shared_ptr<Sprite> ptr;
};

typedef SpritePtr Sprite_ptr;
