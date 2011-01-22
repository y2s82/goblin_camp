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
	: sprites(), width(1) {}

ConstructionSpriteSet::~ConstructionSpriteSet() {}

void ConstructionSpriteSet::Draw(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (sprites.size() > 0) {
		int xOffset = internalPos.X() % width;
		int yOffset = internalPos.Y() % (sprites.size() / width);

		int graphicIndex = xOffset + width * yOffset;
		sprites.at(graphicIndex).Draw(dst, dstRect);
	}
}

void ConstructionSpriteSet::DrawUnderConstruction(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (underconstructionSprites.size() == 1) {
		underconstructionSprites[0].Draw(dst, dstRect);
	} else {
		int xOffset = internalPos.X() % width;
		int yOffset = internalPos.Y() % (sprites.size() / width);

		int graphicIndex = xOffset + width * yOffset;
		sprites.at(graphicIndex).Draw(dst, dstRect);
	}
}

void ConstructionSpriteSet::AddSprite(const Sprite& sprite) {
	sprites.push_back(sprite);
}

void ConstructionSpriteSet::AddUnderConstructionSprite(const Sprite& sprite) {
	underconstructionSprites.push_back(sprite);
}

void ConstructionSpriteSet::SetWidth(int val) {
	width = val;
}

bool ConstructionSpriteSet::IsValid() const {
	return (sprites.size() > 0 && width > 0 && width <= sprites.size() && (sprites.size() / width) * width == sprites.size());
}

bool ConstructionSpriteSet::HasUnderConstructionSprites() const {
	return IsValid() && (underconstructionSprites.size() == 1 || underconstructionSprites.size() == sprites.size());
}

