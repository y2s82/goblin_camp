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
#include "tileRenderer/SpellSpriteSet.hpp"

SpellSpriteSet::SpellSpriteSet()
	: tiles(), frameTime(15) {}

SpellSpriteSet::~SpellSpriteSet() {}

void SpellSpriteSet::Draw(SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (tiles.size() > 0) {
		int frame = (TCODSystem::getElapsedMilli() / frameTime) % tiles.size();
		tiles[frame].Draw(dst, dstRect);
	}
}

void SpellSpriteSet::AddSprite(Sprite tile) {
	tiles.push_back(tile);
}

void SpellSpriteSet::SetFrameRate(int fps) {
	frameTime = 1000/fps;
}
