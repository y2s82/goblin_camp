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

#include "tileRenderer/Sprite.hpp"
#include "Construction.hpp"
#include "ConstructionVisitor.hpp"
#include <boost/shared_ptr.hpp>

class ConstructionSprite
{
public:
	explicit ConstructionSprite();
	~ConstructionSprite();

	void AddSprite(const Sprite& sprite);
	void AddUnderConstructionSprite(const Sprite& sprite);
	void AddUnreadyTrapSprite(const Sprite& sprite);
	void SetWidth(int width);
	void SetOpenSprite(const Sprite& sprite);

	bool IsValid() const;
	bool HasUnderConstructionSprites() const;
	bool IsConnectionMap() const;
		
	// Normal draw
	void Draw(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawUnderConstruction(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawUnreadyTrap(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawOpen(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect * dstRect) const;

	// Connection map draw
	void Draw(Sprite::ConnectedFunction, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawUnderConstruction(Sprite::ConnectedFunction, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawUnreadyTrap(Sprite::ConnectedFunction, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawOpen(Sprite::ConnectedFunction, SDL_Surface * dst, SDL_Rect *dstRect) const;
private:	
	std::vector<Sprite> sprites;
	std::vector<Sprite> underconstructionSprites;
	std::vector<Sprite> unreadyTrapSprites;
	Sprite openSprite;
	int width;
};

class ConstructionSpriteFactory
{
public:
	explicit ConstructionSpriteFactory();
	~ConstructionSpriteFactory();

	void Reset();
	ConstructionSprite Build(boost::shared_ptr<TileSetTexture> currentTexture);

	template <typename IterT> void SetSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetUnderConstructionSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetUnreadyTrapSpriteIndices(IterT start, IterT end);

	void SetOpenDoorSprite(const Sprite& sprite);
	void SetWidth(int width);
	void SetFPS(int fps);
	void SetFrameCount(int frameCount);
	void SetConnectionMap(bool isConnectionMap);

private:
	std::vector<int> spriteIndices;
	std::vector<int> underConstructionSpriteIndices;
	std::vector<int> unreadyTrapSpriteIndices;
	Sprite openDoorSprite;
	int width;
	int frameRate;
	int frameCount;
	bool connectionMapped;
};

template <typename IterT> void ConstructionSpriteFactory::SetSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		spriteIndices.push_back(*iter);
	}
}

template <typename IterT> void ConstructionSpriteFactory::SetUnderConstructionSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		underConstructionSpriteIndices.push_back(*iter);
	}
}

template <typename IterT> void ConstructionSpriteFactory::SetUnreadyTrapSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		unreadyTrapSpriteIndices.push_back(*iter);
	}
}