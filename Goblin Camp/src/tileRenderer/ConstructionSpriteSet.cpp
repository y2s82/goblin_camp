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

ConstructionSpriteSet::ConstructionSpriteSet()
	: sprites(), width(1) {}

ConstructionSpriteSet::~ConstructionSpriteSet() {}

void ConstructionSpriteSet::Draw(const Coordinate& constructUpleft, const Coordinate& pos, SDL_Surface * dst, SDL_Rect *dstRect) const {
	if (sprites.size() > 0) {
		int xOffset = (pos.X() - constructUpleft.X()) % width;
		int yOffset = (pos.Y() - constructUpleft.Y()) % (sprites.size() / width);

		int graphicIndex = xOffset + width * yOffset;
		sprites.at(graphicIndex).Draw(dst, dstRect);
	}
}

void ConstructionSpriteSet::AddSprite(const Sprite& sprite) {
	sprites.push_back(sprite);
}

void ConstructionSpriteSet::SetWidth(int val) {
	width = val;
}

bool ConstructionSpriteSet::IsValid() const {
	return sprites.size() > 0 && width > 0 && width <= sprites.size();
}