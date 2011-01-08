/* Copyright 2010 Ilkka Halila
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

#include "SDLMapRenderer.hpp"
#include "MapMarker.hpp"
#include "Logger.hpp"

SDLMapRenderer::SDLMapRenderer(int resolutionX, int resolutionY) 
: screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta),
  mapSurface(NULL),
  tempBuffer(NULL),
  first(true) {
   TCODSystem::getCharSize(&tileWidth, &tileHeight);
   Uint32 rmask, gmask, bmask, amask;
   #if SDL_BYTEORDER == SDL_BIG_ENDIAN
       rmask = 0xff000000;
       gmask = 0x00ff0000;
       bmask = 0x0000ff00;
       amask = 0x000000ff;
   #else
       rmask = 0x000000ff;
       gmask = 0x0000ff00;
       bmask = 0x00ff0000;
       amask = 0xff000000;
   #endif
   mapSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, rmask, gmask, bmask, amask);
   tempBuffer = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, rmask, gmask, bmask, amask);
   SDL_SetAlpha(mapSurface, 0, SDL_ALPHA_OPAQUE);
   SDL_SetAlpha(tempBuffer, 0, SDL_ALPHA_OPAQUE);
   if (mapSurface == NULL)
   {
	   LOG(SDL_GetError());
   }
   TCODSystem::registerSDLRenderer(this);

   // TODO: Unhardcode tile set loading, move to different class? Nyeh
   tiles = IMG_Load("tileset.png");
   if (tiles == NULL)
   {
	   LOG(SDL_GetError());
   }
}

SDLMapRenderer::~SDLMapRenderer() {}

//TODO: Optimize. This causes the biggest performance hit by far right now 
void SDLMapRenderer::DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int posX, int posY, int sizeX, int sizeY) {
	for (int x = posX; x < posX + sizeX; x++) {
		for (int y = posY; y < posY + sizeY; y++) {
			console->putCharEx(x, y, ' ', TCODColor::black, keyColor);
		}
	}
    // And then render to map
	int screenDeltaX = upleft.X();
	int screenDeltaY = upleft.Y();
	for (int y = 0; y < sizeY; ++y) {
		for (int x = 0; x < sizeX; ++x) {
			int tileX = x + upleft.X();
			int tileY = y + upleft.Y();
			int mapX = x + posX;
			int mapY = y + posY;

			// Draw Terrain
			SDL_Rect dstRect = {tileWidth * mapX, tileHeight * mapY, tileWidth, tileHeight};
			console->setDirty(mapX, mapY, 1, 1);
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				DrawTile(map, tileX, tileY, &dstRect);
				DrawWater(map, tileX, tileY, &dstRect);
				DrawFilth(map, tileX, tileY, &dstRect);
				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					DrawTerritoryOverlay(map, tileX, tileY, &dstRect);
				}
			}
			else {
				// Out of world
				SDL_FillRect(mapSurface, &dstRect, 0);
			}
		}
	}

	DrawMarkers(map, upleft, posX, posY, sizeX, sizeY);
}

void SDLMapRenderer::DrawMarkers(Map* map, Coordinate upleft, int posX, int posY, int sizeX, int sizeY)
{
	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= upleft.X() && markerY < upleft.X() + sizeX
			&& markerY >= upleft.Y() && markerY < upleft.Y() + sizeY) {
				SDL_Rect dstRect = {tileWidth * (markerX - upleft.X() + posX), tileHeight * (markerY - upleft.Y() + posY), tileWidth, tileHeight};
				SDL_Rect srcRect = { 7 * tileWidth, tileHeight, tileWidth, tileHeight };
				SDL_BlitSurface(tiles, &srcRect, mapSurface, &dstRect);
		}
	}
}

void SDLMapRenderer::DrawTile(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	TileType type(map->Type(tileX, tileY));
	SDL_Rect srcRect={tileWidth * type, 0, tileWidth, tileHeight};
	SDL_BlitSurface(tiles,&srcRect,mapSurface, dstRect);
	if (map->GetBlood(tileX, tileY).lock())
	{
		SDL_Rect srcRect={tileWidth * 9, tileHeight, tileWidth, tileHeight};
		SDL_BlitSurface(tiles,&srcRect,mapSurface, dstRect);
	}
	if (map->GroundMarked(tileX, tileY)) {
		SDL_Rect srcRect={tileWidth * 8, tileHeight, tileWidth, tileHeight};
		SDL_BlitSurface(tiles,&srcRect,mapSurface, dstRect);
	}
	
}

void SDLMapRenderer::DrawWater(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	boost::weak_ptr<WaterNode> water = map->GetWater(tileX,tileY);
	if (water.lock()) {
		switch (water.lock()->Depth()) {
		case 1:
			{
				SDL_Rect srcRect={0 * tileWidth, tileHeight, tileWidth, tileHeight};
				SDL_BlitSurface(tiles,&srcRect,mapSurface,dstRect);
			}
			break;
		case 2:
			{
				SDL_Rect srcRect={tileWidth * 1, tileHeight, tileWidth, tileHeight};
				SDL_BlitSurface(tiles,&srcRect,mapSurface,dstRect);	
			}
			break;
		default:
			if (water.lock()->Depth() > 0)
			{
				SDL_Rect srcRect={tileWidth * 2, tileHeight, tileWidth, tileHeight};
				SDL_BlitSurface(tiles,&srcRect,mapSurface,dstRect);
			}
		}
	}
}

void SDLMapRenderer::DrawFilth(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	boost::weak_ptr<FilthNode> filth = map->GetFilth(tileX,tileY);
	if (filth.lock() && filth.lock()->Depth() > 0) {
		SDL_Rect srcRect={tileWidth * ((filth.lock()->Depth() > 5) ? 4 : 3), tileHeight, tileWidth, tileHeight};
		SDL_BlitSurface(tiles,&srcRect,mapSurface,dstRect);
	}
}

void SDLMapRenderer::DrawTerritoryOverlay(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	SDL_Rect srcRect={tileWidth * ((map->IsTerritory(tileX,tileY)) ? 6 : 5), tileHeight, tileWidth, tileHeight};
	SDL_BlitSurface(tiles,&srcRect,mapSurface,dstRect);
}

void SDLMapRenderer::render(void * surf) {
	  SDL_Surface *screen = (SDL_Surface *)surf;
      if ( first ) {
		 first=false;
		 SDL_SetColorKey(screen,SDL_SRCCOLORKEY, SDL_MapRGBA(screen->format, keyColor.r, keyColor.g, keyColor.b, 0));
      }

      SDL_Rect srcRect={0,0,screenWidth,screenHeight};
      SDL_Rect dstRect={0,0,screenWidth,screenHeight};
	  SDL_FillRect(tempBuffer, &dstRect, SDL_MapRGBA(tempBuffer->format, 0,0,0,255));
	  SDL_BlitSurface(mapSurface,&srcRect,tempBuffer,&dstRect);
	  SDL_BlitSurface(screen,&srcRect,tempBuffer,&dstRect);
      SDL_BlitSurface(tempBuffer,&srcRect,screen,&dstRect);   
}