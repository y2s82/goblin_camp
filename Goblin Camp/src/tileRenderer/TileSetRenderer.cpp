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

#include "tileRenderer/TileSetRenderer.hpp"
#include "MapMarker.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "Data/Paths.hpp"

TileSetRenderer::TileSetRenderer(int resolutionX, int resolutionY, boost::shared_ptr<TileSet> ts) 
: screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta),
  mapSurface(NULL),
  tempBuffer(NULL),
  first(true),
  tileSet(ts),
  depTileset() {
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
   // TODO: Remove
   depTileset = boost::shared_ptr<TileSetTexture>(new TileSetTexture("tileset.png", 8, 8));
   TCODSystem::registerSDLRenderer(this);
}

TileSetRenderer::~TileSetRenderer() {}

void TileSetRenderer::PreparePrefabs() 
{
	for (std::vector<NPCPreset>::iterator npci = NPC::Presets.begin(); npci != NPC::Presets.end(); ++npci)
	{
		npci->graphicsHint = tileSet->GetGraphicsHintFor(*npci);
	}
	for (std::vector<NatureObjectPreset>::iterator nopi = NatureObject::Presets.begin(); nopi != NatureObject::Presets.end(); ++nopi)
	{
		nopi->graphicsHint = tileSet->GetGraphicsHintFor(*nopi);
	}

	for (std::vector<ItemPreset>::iterator itemi = Item::Presets.begin(); itemi != Item::Presets.end(); ++itemi)
	{
		itemi->graphicsHint = tileSet->GetGraphicsHintFor(*itemi);
	}
}

//TODO: Optimize. This causes the biggest performance hit by far right now 
void TileSetRenderer::DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int offsetX, int offsetY, int sizeX, int sizeY) {
	if (sizeX == -1) sizeX = console->getWidth();
	if (sizeY == -1) sizeY = console->getHeight();

	for (int x = offsetX; x < offsetX + sizeX; x++) {
		for (int y = offsetY; y < offsetY + sizeY; y++) {
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
			int mapX = x + offsetX;
			int mapY = y + offsetY;

			// Draw Terrain
			SDL_Rect dstRect(CalcDest(mapX, mapY));
			console->setDirty(mapX, mapY, 1, 1);
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				DrawTerrain(map, tileX, tileY, &dstRect);
				DrawWater(map, tileX, tileY, &dstRect);
				DrawConstruction(map, tileX, tileY, &dstRect);
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

	DrawMarkers(map, upleft, offsetX, offsetY, sizeX, sizeY);

	DrawNatureObjects(upleft, offsetX, offsetY, sizeX, sizeY);
	DrawItems(upleft, offsetX, offsetY, sizeX, sizeY);
	DrawNPCs(upleft, offsetX, offsetY, sizeX, sizeY);
}

void TileSetRenderer::DrawConstruction(Map* map, int tileX, int tileY, SDL_Rect * dstRect)
{
	boost::weak_ptr<Construction> constructPtr = Game::Inst()->GetConstruction(map->GetConstruction(tileX, tileY));
	boost::shared_ptr<Construction> construct = constructPtr.lock();
	if (construct) {
		// Bunch of cleverness here later
		depTileset->DrawTile(19, mapSurface, dstRect);
	}
}

void TileSetRenderer::DrawItems(Coordinate upleft, int offsetX, int offsetY, int sizeX, int sizeY) {
	for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end(); ++itemi) {
		if (!itemi->second->ContainedIn().lock()) {
			Coordinate itemPos = itemi->second->Position();
			if (itemPos.X() >= upleft.X() && itemPos.X() < upleft.X() + sizeX
				&& itemPos.Y() >= upleft.Y() && itemPos.Y() < upleft.Y() + sizeY)
			{
				SDL_Rect dstRect(CalcDest(offsetX + itemPos.X() - upleft.X(), offsetY + itemPos.Y() - upleft.Y()));
				tileSet->DrawItem(itemi->second, mapSurface, &dstRect);
			}
		}
	}
}

void TileSetRenderer::DrawNatureObjects(Coordinate upleft, int offsetX, int offsetY, int sizeX, int sizeY) {
	for (std::map<int,boost::shared_ptr<NatureObject> >::iterator planti = Game::Inst()->natureList.begin(); planti != Game::Inst()->natureList.end(); ++planti) {
		Coordinate plantPos = planti->second->Position();
		if (plantPos.X() >= upleft.X() && plantPos.X() < upleft.X() + sizeX
				&& plantPos.Y() >= upleft.Y() && plantPos.Y() < upleft.Y() + sizeY)
		{
			SDL_Rect dstRect(CalcDest(offsetX + plantPos.X() - upleft.X(), offsetY + plantPos.Y() - upleft.Y()));
			if (planti->second->Marked())
			{
				tileSet->DrawMarkedOverlay(mapSurface, &dstRect);
			}
			tileSet->DrawNatureObject(planti->second, mapSurface, &dstRect);
		}
	}
}

void TileSetRenderer::DrawNPCs(Coordinate upleft, int offsetX, int offsetY, int sizeX, int sizeY) {
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
		Coordinate npcPos = npci->second->Position();
		if (npcPos.X() >= upleft.X() && npcPos.X() < upleft.X() + sizeX
				&& npcPos.Y() >= upleft.Y() && npcPos.Y() < upleft.Y() + sizeY)
		{
			SDL_Rect dstRect(CalcDest(offsetX + npcPos.X() - upleft.X(), offsetY + npcPos.Y() - upleft.Y()));
			tileSet->DrawNPC(npci->second, mapSurface, &dstRect);
		}
	}
}

void TileSetRenderer::DrawMarkers(Map* map, Coordinate upleft, int offsetX, int offsetY, int sizeX, int sizeY)
{
	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= upleft.X() && markerY < upleft.X() + sizeX
			&& markerY >= upleft.Y() && markerY < upleft.Y() + sizeY) {
				SDL_Rect dstRect( CalcDest(markerX - upleft.X() + offsetX, markerY - upleft.Y() + offsetY));
				tileSet->DrawMarker(mapSurface, &dstRect);
		}
	}
}

void TileSetRenderer::DrawTerrain(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	TileType type(map->Type(tileX, tileY));
	tileSet->DrawTerrain(type, mapSurface, dstRect);
	if (map->GetBlood(tileX, tileY).lock())	{
		tileSet->DrawBlood(mapSurface, dstRect);
	}
	if (map->GroundMarked(tileX, tileY)) {
		tileSet->DrawMarkedOverlay(mapSurface, dstRect);
	}
	
}

void TileSetRenderer::DrawWater(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	boost::weak_ptr<WaterNode> water = map->GetWater(tileX,tileY);
	if (water.lock()) {
		if (water.lock()->Depth() > 0)
		{
			tileSet->DrawWater(water.lock()->Depth() / 5000, mapSurface, dstRect);
		}
	}
}

void TileSetRenderer::DrawFilth(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	boost::weak_ptr<FilthNode> filth = map->GetFilth(tileX,tileY);
	if (filth.lock() && filth.lock()->Depth() > 5) {
		tileSet->DrawFilthMajor(mapSurface, dstRect);
	} else if (filth.lock() && filth.lock()->Depth() > 0) {
		tileSet->DrawFilthMinor(mapSurface, dstRect);
	}
}

void TileSetRenderer::DrawTerritoryOverlay(Map* map, int tileX, int tileY, SDL_Rect * dstRect) {
	tileSet->DrawTerritoryOverlay(map->IsTerritory(tileX,tileY), mapSurface, dstRect);
}

void TileSetRenderer::render(void * surf) {
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

