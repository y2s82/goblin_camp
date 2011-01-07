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
   TCODSystem::getCharSize(&charWidth, &charHeight);
   tileMap.resize(boost::extents[resolutionX / charWidth][resolutionY / charHeight]);
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
	for (int x = posX; x < sizeX; x++) {
		for (int y = posY; y < sizeY; y++) {
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
			TileType type(TILENONE);
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				type = map->Type(tileX, tileY);
			}
			if (tileMap[x + posX][y + posY] != type)
			{
				tileMap[x + posX][y + posY] = type;
				console->setDirty(x + posX, y + posY,1,1);
				SDL_Rect srcRect={8 * type, 0, 8, 8};
				SDL_Rect dstRect={8 * (x + posX), 8 * (y + posY), 8, 8};
				SDL_BlitSurface(tiles,&srcRect,mapSurface,&dstRect);
			}

				/*boost::weak_ptr<WaterNode> water = map->GetWater(x,y);
				if (water.lock()) {
					minimap->putCharEx(x-screenDeltaX, y-screenDeltaY, water.lock()->GetGraphic(), water.lock()->GetColor(), TCODColor::black);
				}
				boost::weak_ptr<FilthNode> filth = map->GetFilth(x,y);
				if (filth.lock() && filth.lock()->Depth() > 0) {
					minimap->putCharEx(x-screenDeltaX, y-screenDeltaY, filth.lock()->GetGraphic(), filth.lock()->GetColor(), TCODColor::black);
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					minimap->setCharBackground(x-screenDeltaX,y-screenDeltaY, map->IsTerritory(x,y) ? TCODColor::darkGreen : TCODColor::darkRed);
				}*/
		}
	}


	/*
	

	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		markeri->second.Draw(upleft, minimap);
	}
	TCODConsole::
	minimap->flush();
	TCODConsole::blit(minimap, 0, 0, sizeX, sizeY, console, posX, posY);*/
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