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

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <SDL.h>
#include "tileRenderer/Corner.hpp"


/*******************/
// TileSetTexture
//
// Encapsulates a texture containing equally-sized
// tiles, allowing them to be drawn by an index. The index
// is row wise (i.e. left to right, top to bottom)
/******************/
class TileSetTexture
{
public:
	explicit TileSetTexture(boost::filesystem::path path, int tileWidth, int tileHeight);
	~TileSetTexture();

	int Count() const;

	/* Be aware that DrawTile and DrawTileCorner both propagate
	   SDL_BlitSurface semantics of modifying the dstRect reference to
	   store the final blit rectangle:

	     http://sdl.beuc.net/sdl.wiki/SDL_BlitSurface
	  
	   Client code should be careful to do a value copy of the
	   SDL_Rect if they don't want modifications to be propagated.
	 */
	void DrawTile(int tile, SDL_Surface * dst, SDL_Rect * dstRect) const;
	void DrawTileCorner(int tile, Corner corner, SDL_Surface * dst, SDL_Rect * dstRect) const;

	boost::shared_ptr<SDL_Surface> GetInternalSurface();

private:
	int tileWidth, tileHeight;
	int tileXDim, tileYDim;
	int tileCount;
	boost::shared_ptr<SDL_Surface> tiles;
};
