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
#include "tileRenderer/ConstructionSpriteSet.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Door.hpp"
#include "SpawningPool.hpp"
#include "Logger.hpp"

ConstructionSpriteSet::ConstructionSpriteSet()
	: sprites(), 
	underconstructionSprites(),
	unreadyTrapSprites(),
	openSprite(),
	width(1){}

ConstructionSpriteSet::~ConstructionSpriteSet() {}

void ConstructionSpriteSet::Draw(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (IsValid()) {
		int xOffset = internalPos.X() % width;
		int yOffset = internalPos.Y() % (sprites.size() / width);

		int graphicIndex = xOffset + width * yOffset;
		sprites.at(graphicIndex).Draw(dst, dstRect);
	}
}

void ConstructionSpriteSet::DrawUnderConstruction(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
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

void ConstructionSpriteSet::DrawUnreadyTrap(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
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

void ConstructionSpriteSet::DrawOpen(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (openSprite.Exists()) {
		openSprite.Draw(dst, dstRect);
	} else {
		Draw(internalPos, dst, dstRect);
	}
}

void ConstructionSpriteSet::Draw(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (IsValid() && IsConnectionMap()) {
		sprites[0].Draw(connected, dst, dstRect);
	}
}

void ConstructionSpriteSet::DrawUnderConstruction(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (underconstructionSprites.size() > 0) {
		underconstructionSprites[0].Draw(connected, dst, dstRect);
	}
}

void ConstructionSpriteSet::DrawUnreadyTrap(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (unreadyTrapSprites.size() > 0) {
		unreadyTrapSprites[0].Draw(connected, dst, dstRect);
	} else {
		Draw(connected, dst, dstRect);
	}
}

void ConstructionSpriteSet::DrawOpen(Sprite::ConnectedFunction connected, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (openSprite.Exists()) {
		openSprite.Draw(dst, dstRect);
	} else {
		Draw(connected, dst, dstRect);
	}
}

void ConstructionSpriteSet::AddSprite(const Sprite& sprite) {
	sprites.push_back(sprite);
}

void ConstructionSpriteSet::AddUnderConstructionSprite(const Sprite& sprite) {
	underconstructionSprites.push_back(sprite);
}

void ConstructionSpriteSet::AddUnreadyTrapSprite(const Sprite& sprite) {
	unreadyTrapSprites.push_back(sprite);
}

void ConstructionSpriteSet::SetOpenSprite(const Sprite& sprite) {
	openSprite = sprite;
}

void ConstructionSpriteSet::SetWidth(int val) {
	width = val;
}

bool ConstructionSpriteSet::IsValid() const {
	return sprites.size() > 0 && (sprites[0].IsConnectionMap() || (width > 0 && width <= sprites.size() && (sprites.size() / width) * width == sprites.size()));
}

bool ConstructionSpriteSet::IsConnectionMap() const {
	return sprites.size() > 0 && sprites[0].IsConnectionMap();
}

bool ConstructionSpriteSet::HasUnderConstructionSprites() const {
	return (underconstructionSprites.size() > 0);
}

