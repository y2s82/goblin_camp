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

#include <SDL.h>
#include <SDL_image.h>

#include "tileRenderer/TileSetTexture.hpp"
#include "Logger.hpp"

TileSetTexture::TileSetTexture(boost::filesystem::path path, int tileW, int tileH)
	: tileWidth(tileW), tileHeight(tileH), tileXDim(0), tileYDim(0), tileCount(0), tiles()
{
	SDL_Surface * temp = IMG_Load(path.string().c_str());
	if (temp != NULL) {
		tiles = boost::shared_ptr<SDL_Surface>(SDL_DisplayFormatAlpha(temp), SDL_FreeSurface);
		SDL_FreeSurface(temp);
	}
	if (tiles) {
	    tileXDim = (tiles->w / tileWidth);
		tileYDim = (tiles->h / tileHeight);
		tileCount = tileXDim * tileYDim;
    } else {
		LOG(SDL_GetError());
	    tileCount = 0;
	}
}

TileSetTexture::~TileSetTexture()
{
}

int TileSetTexture::Count() const
{
	return tileCount;
}

// dstRect may be modified by BlitSurface; this is documented
void TileSetTexture::DrawTile(int tile, SDL_Surface * dst, SDL_Rect * dstRect) const
{
	if (tile < tileCount)
	{
		int xCoord = tile % tileXDim;
		int yCoord = tile / tileXDim;
		SDL_Rect srcRect = {
			static_cast<Sint16>(xCoord * tileWidth),
			static_cast<Sint16>(yCoord * tileHeight),
			static_cast<Uint16>(tileWidth),
			static_cast<Uint16>(tileHeight)
		};
		SDL_BlitSurface(tiles.get(),&srcRect, dst, dstRect);
	}
}

// dstRect may be modified by BlitSurface; this is documented
void TileSetTexture::DrawTileCorner(int tile, Corner corner, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (tile < tileCount) {
		int xCoord = tile % tileXDim;
		int yCoord = tile / tileXDim;
		int halfWidth = tileWidth >> 1;
		int halfHeight = tileHeight >> 1;
		SDL_Rect srcRect = {
			static_cast<Sint16>(xCoord * tileWidth),
			static_cast<Sint16>(yCoord * tileHeight),
			static_cast<Uint16>(halfWidth),
			static_cast<Uint16>(halfHeight)
		};
		
		dstRect->h = halfHeight;
		if (corner & 0x1) {
			srcRect.x += halfWidth;
			dstRect->x += halfWidth;
			srcRect.w = tileWidth - halfWidth;
			dstRect->w = tileWidth - halfWidth;
		} else {
			dstRect->w = halfWidth;
		}
		if (corner & 0x2) {
			srcRect.y += halfHeight;
			dstRect->y += halfHeight;
			srcRect.h = tileHeight - halfHeight;
			dstRect->h = tileHeight - halfHeight;
		} else {
			dstRect->h = halfHeight;
		}
		
		SDL_BlitSurface(tiles.get(),&srcRect, dst, dstRect);
	}
}

boost::shared_ptr<SDL_Surface> TileSetTexture::GetInternalSurface() {
	return tiles;
}
