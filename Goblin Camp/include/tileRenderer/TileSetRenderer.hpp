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
	TileSetRenderer(int screenWidth, int screenHeight, boost::shared_ptr<TileSet> tileSet, TCODConsole * mapConsole = 0);
	~TileSetRenderer();

	Coordinate TileAt(int screenX, int screenY, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const;
	void DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) ;
	void PreparePrefabs();

	void SetCursorMode(CursorType mode);
	void SetCursorMode(const NPCPreset& preset);
	void SetCursorMode(const ItemPreset& preset);
	void SetCursorMode(int other);
	void DrawCursor(const Coordinate& pos, float focusX, float focusY, bool placeable);
	void DrawCursor(const Coordinate& start, const Coordinate& end, float focusX, float focusY, bool placeable);

	void render(void *sdlSurface);
private:
	TCODConsole * tcodConsole;

	// the font characters size
	int screenWidth, screenHeight;
	boost::shared_ptr<SDL_Surface> mapSurface;
	boost::shared_ptr<SDL_Surface> tempBuffer;
	boost::shared_ptr<TileSet> tileSet;
	TCODColor keyColor;
	int mapOffsetX, mapOffsetY; // This is the pixel offset when drawing to the viewport
	int startTileX, startTileY;
	CursorType cursorMode;
	int cursorHint;

	void DrawTerrain			(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawWater				(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawFilth				(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawTerritoryOverlay	(Map* map, int tileX, int tileY, SDL_Rect * dstRect);
	void DrawConstruction		(Map* map, int tileX, int tileY, SDL_Rect * dstRect);

	void DrawMarkers(Map * map, int startTileX, int startTileY, int sizeX, int sizeY);
	void DrawItems(int startTileX, int startTileY, int sizeX, int sizeY);
	void DrawNatureObjects(int startTileX, int startTileY, int sizeX, int sizeY);
	void DrawNPCs(int startTileX, int startTileY, int sizeX, int sizeY);
	void DrawSpells(int startTileX, int startTileY, int sizeX, int sizeY);
	void DrawFires(int startTile, int startTileY, int sizeX, int sizeY);

	SDL_Rect CalcDest(int mapPosX, int mapPosY) { SDL_Rect dstRect = {tileSet->TileWidth() * (mapPosX - startTileX) + mapOffsetX, tileSet->TileHeight() * (mapPosY - startTileY) + mapOffsetY, tileSet->TileWidth(), tileSet->TileHeight()}; return dstRect; }
};