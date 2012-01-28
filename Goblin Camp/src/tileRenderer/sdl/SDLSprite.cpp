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

#include "tileRenderer/sdl/SDLSprite.hpp"

SDLSprite::SDLSprite(SDLTilesetRenderer * const renderer, boost::shared_ptr<TileSetTexture> tilesetTexture, int tile)
	: Sprite(tile),
	  renderer(renderer),
	  texture(tilesetTexture)
{
}

SDLSprite::~SDLSprite() {}

void SDLSprite::DrawInternal(int screenX, int screenY, int tile) const {
	renderer->DrawSprite(screenX, screenY, texture, tile);
}

void SDLSprite::DrawInternal(int screenX, int screenY, int tile, Corner corner) const {
	renderer->DrawSpriteCorner(screenX, screenY, texture, tile, corner);
}