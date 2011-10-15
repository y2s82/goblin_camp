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
#include "tileRenderer/PermutationTable.hpp"

class TilesetRenderer : public MapRenderer
{
	friend class DrawConstructionVisitor;
public:
	explicit TilesetRenderer(int screenWidth, int screenHeight, TCODConsole * mapConsole = 0);
	virtual ~TilesetRenderer() = 0;

	virtual Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile) = 0;
	virtual Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, const std::vector<int>& tiles, bool connectionMap, int frameRate = 15, int frameCount = 1) = 0;
	template <typename IterT> static Sprite_ptr CreateSprite(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate = 15, int frameCount = 1);

	bool SetTileset(boost::shared_ptr<TileSet> tileSet);

	Coordinate TileAt(int screenX, int screenY, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const;

	void DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) ;
	void PreparePrefabs();
	float ScrollRate() const;

	int GetScreenWidth() const;
	int GetScreenHeight() const;
	TCODColor GetKeyColor() const;

	void SetCursorMode(CursorType mode);
	void SetCursorMode(const NPCPreset& preset);
	void SetCursorMode(const ItemPreset& preset);
	void SetCursorMode(int other);
	void DrawCursor(const Coordinate& pos, bool placeable);
	void DrawCursor(const Coordinate& start, const Coordinate& end, bool placeable);

	virtual void SetTranslucentUI(bool translucent);

protected:
	virtual void PreDrawMap(int viewportX, int viewportY, int viewportW, int viewportH) = 0;
	virtual void PostDrawMap() = 0;
	virtual void DrawNullTile(int screenX, int screenY) = 0;

	virtual bool TilesetChanged();

	TCODConsole * tcodConsole;
	PermutationTable permutationTable;
	boost::shared_ptr<TileSet> tileSet;
	bool translucentUI;

	// Current render state
	Map * map;
	int startPixelX, startPixelY;
	int pixelW, pixelH;
	int mapOffsetX, mapOffsetY; // This is the pixel offset when drawing to the viewport
	int startTileX, startTileY; // Top-left tile
	int tilesX, tilesY; // Num tiles to render in this window
	CursorType cursorMode;
	int cursorHint;	
	
	void DrawTerrain			(int screenX, int screenY, Coordinate pos) const;
	void DrawFilth				(int screenX, int screenY, Coordinate pos) const;
	void DrawTerritoryOverlay	(int screenX, int screenY, Coordinate pos) const;
	
	void DrawMarkers() const;
	void DrawItems() const;
	void DrawNPCs() const;
	void DrawSpells() const;
	void DrawFires() const;

private:
	// the font characters size
	int screenWidth, screenHeight;
	TCODColor keyColor;
};

template <typename IterT> Sprite_ptr TilesetRenderer::CreateSprite(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> tilesetTexture, IterT start, IterT end, bool connectionMap, int frameRate, int frameCount) {
	std::vector<int> tiles(start, end);
	return spriteFactory->CreateSprite(tilesetTexture, tiles, connectionMap, frameRate, frameCount);
}

boost::shared_ptr<TilesetRenderer> CreateTilesetRenderer(int width, int height, TCODConsole * console, std::string tilesetName);