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

#include "tileRenderer/TileSetRenderer.hpp"
#include <SDL.h>

class SDLTilesetRenderer : public TilesetRenderer, public ITCODSDLRenderer
{
public:
	explicit SDLTilesetRenderer(int screenWidth, int screenHeight, TCODConsole * mapConsole = 0);
	~SDLTilesetRenderer();

	Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, const std::vector<int>& tiles, bool connectionMap, int frameRate = 15, int frameCount = 1);
	
	void DrawSprite(int screenX, int screenY, boost::shared_ptr<TileSetTexture> texture, int tile) const;
	void DrawSpriteCorner(int screenX, int screenY, boost::shared_ptr<TileSetTexture> texture, int tile, Corner corner) const;

	void render(void *sdlSurface);  // FIXME: inherited a virtual from ITCODSDLRenderer
	void render(void *sdlSurface, void*sdlScreen);

	void SetTranslucentUI(bool translucent);
protected:
	void PreDrawMap(int viewportX, int viewportY, int viewportW, int viewportH);
	void PostDrawMap();
	void DrawNullTile(int screenX, int screenY);
private:
	boost::shared_ptr<SDL_Surface> mapSurface;

	SDL_Rect CalcDest(int screenX, int screenY) const {
		SDL_Rect dstRect = {
			static_cast<Sint16>(tileSet->TileWidth() * (screenX) + mapOffsetX + startPixelX),
			static_cast<Sint16>(tileSet->TileHeight() * (screenY) + mapOffsetY + startPixelY),
			static_cast<Uint16>(tileSet->TileWidth()),
			static_cast<Uint16>(tileSet->TileHeight())
		};
		return dstRect;
	}
};
