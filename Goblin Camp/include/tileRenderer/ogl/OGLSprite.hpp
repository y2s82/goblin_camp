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
#include "tileRenderer/ogl/OGLTilesetRenderer.hpp"

class OGLSprite : public Sprite
{
public:
	explicit OGLSprite(OGLTilesetRenderer * const renderer, int id);
	template <typename IterT> explicit OGLSprite(OGLTilesetRenderer * const renderer, IterT start, IterT end, bool connectionMap, int frameRate = 15, int frameCount = 1);
	~OGLSprite();
	
protected:
	void DrawInternal(int screenX, int screenY, int tile) const;
	void DrawInternal(int screenX, int screenY, int tile, Corner corner) const;

private:
	OGLTilesetRenderer * renderer;
};

template <typename IterT> OGLSprite::OGLSprite(OGLTilesetRenderer * const renderer, IterT start, IterT end, bool connectionMap, int frameRate, int frames)
	: Sprite(start, end, connectionMap, frameRate, frames),
	  renderer(renderer)
{
}