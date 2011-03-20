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

#include "tileRenderer/ogl/OGLSprite.hpp"

OGLSprite::OGLSprite(OGLTilesetRenderer * const renderer, int tile)
	: Sprite(tile),
	  renderer(renderer)
{
}

OGLSprite::~OGLSprite() {}

void OGLSprite::DrawInternal(int screenX, int screenY, int tile) const {
	if (tile != -1) {
		renderer->DrawSprite(screenX, screenY, tile);
	}
}

void OGLSprite::DrawInternal(int screenX, int screenY, int tile, Corner corner) const {
	if (tile != -1) {
		renderer->DrawSpriteCorner(screenX, screenY, tile, corner);
	}
}