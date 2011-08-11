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

#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include <libtcod.hpp>
#include "tileRenderer/Sprite.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/sdl/SDLTilesetRenderer.hpp"

/****************/
// SDLSprite
// Description of a single tile that can be drawn. 
// Supports animation and connection maps in addition to just single tiles.
/****************/
class SDLSprite : public Sprite
{
public:
	explicit SDLSprite(SDLTilesetRenderer * const renderer, boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	template <typename IterT> explicit SDLSprite(SDLTilesetRenderer * const renderer, boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate = 15, int frameCount = 1);
	~SDLSprite();
	
protected:
	void DrawInternal(int screenX, int screenY, int tile) const;
	void DrawInternal(int screenX, int screenY, int tile, Corner corner) const;

private:
	SDLTilesetRenderer * renderer;
	boost::shared_ptr<TileSetTexture> texture;
};

template <typename IterT> SDLSprite::SDLSprite(SDLTilesetRenderer * const renderer, boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate, int frames)
	: Sprite(start, end, connectionMap, frameRate, frames),
	  renderer(renderer),
	  texture(tilesetTexture)
{
}
