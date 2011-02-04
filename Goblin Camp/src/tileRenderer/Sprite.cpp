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

#include "tileRenderer/Sprite.hpp"
#include "tileRenderer/TileSetUtil.hpp"

Sprite::Sprite() : tiles(), texture(), type(SPRITE_Single), frameTime(15) {}
Sprite::Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile)
	: tiles(),
	  texture(tilesetTexture),
	  type(SPRITE_Single),
	  frameTime(15)
{
	tiles.push_back(tile);
}



Sprite::~Sprite() {}

bool Sprite::Exists() const
{
	return tiles.size() > 0;
}

bool Sprite::IsConnectionMap() const {
	return type & (SPRITE_ConnectionMap | SPRITE_ExtendedConnectionMap);
}

bool Sprite::IsExtendedConnectionMap() const {
	return type & SPRITE_ExtendedConnectionMap;
}

bool Sprite::IsAnimated() const {
	return type & SPRITE_Animated;
}

void Sprite::Draw(SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (Exists()) {
		if (IsAnimated()) {
			int frame = (TCODSystem::getElapsedMilli() / frameTime) % tiles.size();
			texture->DrawTile(tiles[frame], dst, dstRect);
		} else {
			texture->DrawTile(tiles[0], dst, dstRect);
		}
	}
}
	
void Sprite::Draw(ConnectedFunction connected, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (IsConnectionMap()) {
		if (IsExtendedConnectionMap()) {

		} else {
			int index = TilesetUtil::CalcConnectionMapIndex(connected(NORTH), connected(EAST), connected(SOUTH), connected(WEST));
			texture->DrawTile(tiles[index], dst, dstRect);
		}
	} else {
		Draw(dst, dstRect);
	}

}




