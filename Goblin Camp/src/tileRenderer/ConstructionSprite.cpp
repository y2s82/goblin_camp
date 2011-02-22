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
#include "tileRenderer/ConstructionSprite.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Door.hpp"
#include "SpawningPool.hpp"
#include "Logger.hpp"

ConstructionSprite::ConstructionSprite()
	: sprites(), 
	underconstructionSprites(),
	unreadyTrapSprites(),
	openSprite(),
	width(1){}

ConstructionSprite::~ConstructionSprite() {}

void ConstructionSprite::Draw(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (IsValid()) {
		int xOffset = internalPos.X() % width;
		int yOffset = internalPos.Y() % (sprites.size() / width);

		int graphicIndex = xOffset + width * yOffset;
		sprites.at(graphicIndex).Draw(dst, dstRect);
	}
}

void ConstructionSprite::DrawUnderConstruction(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (underconstructionSprites.size() > 0)
	{
		if (!IsConnectionMap() && underconstructionSprites.size() == sprites.size() && IsValid()) {
			int xOffset = internalPos.X() % width;
			int yOffset = internalPos.Y() % (sprites.size() / width);

			int graphicIndex = xOffset + width * yOffset;
			underconstructionSprites.at(graphicIndex).Draw(dst, dstRect);
		} else {
			underconstructionSprites.at(0).Draw(dst, dstRect);
		}
	}
}

void ConstructionSprite::DrawUnreadyTrap(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (unreadyTrapSprites.size() > 0)
	{
		if (!IsConnectionMap() && unreadyTrapSprites.size() == sprites.size() && IsValid()) {
			int xOffset = internalPos.X() % width;
			int yOffset = internalPos.Y() % (sprites.size() / width);

			int graphicIndex = xOffset + width * yOffset;
			unreadyTrapSprites.at(graphicIndex).Draw(dst, dstRect);
		} else {
			unreadyTrapSprites.at(0).Draw(dst, dstRect);
		}
	} else { 
		Draw(internalPos, dst, dstRect);
	}
}

void ConstructionSprite::DrawOpen(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (openSprite.Exists()) {
		openSprite.Draw(dst, dstRect);
	} else {
		Draw(internalPos, dst, dstRect);
	}
}

void ConstructionSprite::Draw(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (IsValid() && IsConnectionMap()) {
		sprites[0].Draw(connected, dst, dstRect);
	}
}

void ConstructionSprite::DrawUnderConstruction(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (underconstructionSprites.size() > 0) {
		underconstructionSprites[0].Draw(connected, dst, dstRect);
	}
}

void ConstructionSprite::DrawUnreadyTrap(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (unreadyTrapSprites.size() > 0) {
		unreadyTrapSprites[0].Draw(connected, dst, dstRect);
	} else {
		Draw(connected, dst, dstRect);
	}
}

void ConstructionSprite::DrawOpen(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (openSprite.Exists()) {
		openSprite.Draw(dst, dstRect);
	} else {
		Draw(connected, dst, dstRect);
	}
}

void ConstructionSprite::AddSprite(const Sprite& sprite) {
	sprites.push_back(sprite);
}

void ConstructionSprite::AddUnderConstructionSprite(const Sprite& sprite) {
	underconstructionSprites.push_back(sprite);
}

void ConstructionSprite::AddUnreadyTrapSprite(const Sprite& sprite) {
	unreadyTrapSprites.push_back(sprite);
}

void ConstructionSprite::SetOpenSprite(const Sprite& sprite) {
	openSprite = sprite;
}

void ConstructionSprite::SetWidth(int val) {
	width = val;
}

bool ConstructionSprite::IsValid() const {
	return sprites.size() > 0 && (sprites[0].IsConnectionMap() || (width > 0 && width <= sprites.size() && (sprites.size() / width) * width == sprites.size()));
}

bool ConstructionSprite::IsConnectionMap() const {
	return sprites.size() > 0 && sprites[0].IsConnectionMap();
}

bool ConstructionSprite::HasUnderConstructionSprites() const {
	return (underconstructionSprites.size() > 0);
}

ConstructionSpriteFactory::ConstructionSpriteFactory() 
	: spriteIndices(),
	  underConstructionSpriteIndices(),
	  unreadyTrapSpriteIndices(),
	  openDoorSprite(),
	  width(1),
	  frameRate(15),
	  frameCount(1),
	  connectionMapped(false)
{}

ConstructionSpriteFactory::~ConstructionSpriteFactory() {}

void ConstructionSpriteFactory::Reset() {
	spriteIndices.clear();
	underConstructionSpriteIndices.clear();
	unreadyTrapSpriteIndices.clear();
	openDoorSprite = Sprite();
	width = 1;
	frameRate = 15;
	frameCount = 1;
	connectionMapped = false;
}

ConstructionSprite ConstructionSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	ConstructionSprite spriteSet = ConstructionSprite();
	if (connectionMapped) {
		if (spriteIndices.size() > 0) {
			spriteSet.AddSprite(Sprite(currentTexture, spriteIndices.begin(), spriteIndices.end(), true));
		}
		if (underConstructionSpriteIndices.size() > 0) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, underConstructionSpriteIndices.begin(), underConstructionSpriteIndices.end(), true));
		}
		if (unreadyTrapSpriteIndices.size() > 0) {
			spriteSet.AddUnreadyTrapSprite(Sprite(currentTexture, unreadyTrapSpriteIndices.begin(), unreadyTrapSpriteIndices.end(), true));
		}
	} else {
		int numSprites = spriteIndices.size() / frameCount;
		for (int sprite = 0; sprite < numSprites; ++sprite)
		{
			std::vector<int> frames;
			for (int frame = 0; frame < frameCount; ++frame) {
				frames.push_back(spriteIndices.at(sprite + frame * numSprites));
			}
			spriteSet.AddSprite(Sprite(currentTexture, frames.begin(), frames.end(), false, frameRate));
		}
		
		for (std::vector<int>::iterator iter = underConstructionSpriteIndices.begin(); iter != underConstructionSpriteIndices.end(); ++iter) {
			spriteSet.AddUnderConstructionSprite(Sprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = unreadyTrapSpriteIndices.begin(); iter != unreadyTrapSpriteIndices.end(); ++iter) {
			spriteSet.AddUnreadyTrapSprite(Sprite(currentTexture, *iter));
		}
		spriteSet.SetWidth(width);
	}
	spriteSet.SetOpenSprite(openDoorSprite);
	return spriteSet;
}

void ConstructionSpriteFactory::SetOpenDoorSprite(const Sprite& sprite) {
	openDoorSprite = sprite;
}

void ConstructionSpriteFactory::SetWidth(int w) {
	width = w;
}

void ConstructionSpriteFactory::SetFPS(int fps) {
	frameRate = fps;
}

void ConstructionSpriteFactory::SetFrameCount(int frames) {
	frameCount = frames;
}

void ConstructionSpriteFactory::SetConnectionMap(bool isConnectionMap) {
	connectionMapped = isConnectionMap;
}