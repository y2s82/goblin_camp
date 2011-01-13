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

#include <libtcod.hpp>
#include "MapRenderer.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/TileSet.hpp"
#include <SDL.h>

class TileSetRenderer : public MapRenderer, public ITCODSDLRenderer, private boost::noncopyable
{
public:
	TileSetRenderer(int screenWidth, int screenHeight, boost::shared_ptr<TileSet> tileSet);
	~TileSetRenderer();

	void DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int offsetX = 0, int offsetY = 0, int sizeX = -1, int sizeY = -1) ;
	void PreparePrefabs();
	void render(void *sdlSurface);
private:
	// the font characters size
	int screenWidth, screenHeight;
	SDL_Surface *mapSurface;
	SDL_Surface *tempBuffer;
	boost::shared_ptr<TileSet> tileSet;
	boost::shared_ptr<TileSetTexture> depTileset;
	TCODColor keyColor;
	bool first;

	void DrawTerrain			(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawWater				(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawFilth				(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawTerritoryOverlay	(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawConstruction		(Map* map, int tileX, int tileY, SDL_Rect * dstRect);

	void DrawMarkers(Map * map, Coordinate upleft, int posX, int posY, int sizeX, int sizeY);
	void DrawItems(Coordinate upleft, int tileX, int tileY, int sizeX, int sizeY);
	void DrawNatureObjects(Coordinate upleft, int tileX, int tileY, int sizeX, int sizeY);
	void DrawNPCs(Coordinate upleft, int tileX, int tileY, int sizeX, int sizeY);

	SDL_Rect CalcDest(int mapPosX, int mapPosY) { SDL_Rect dstRect = {tileSet->TileWidth() * mapPosX, tileSet->TileHeight() * mapPosY, tileSet->TileWidth(), tileSet->TileHeight()}; return dstRect; }
};