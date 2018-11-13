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
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/ogl/OGLViewportLayer.hpp"
#include <boost/multi_array.hpp>

struct RawTileData {
	unsigned int tile;
	boost::shared_ptr<TileSetTexture> texture;
};

// FIXME
class TCODLIB_API ITCODOGLRenderer {
public :
	virtual ~ITCODOGLRenderer() {}
	virtual void render() = 0;
};

class OGLTilesetRenderer : public TilesetRenderer, public ITCODOGLRenderer
{
public:
	explicit OGLTilesetRenderer(int screenWidth, int screenHeight, TCODConsole * mapConsole = 0);
	~OGLTilesetRenderer();

	Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	Sprite_ptr CreateSprite(boost::shared_ptr<TileSetTexture> tilesetTexture, const std::vector<int>& tiles, bool connectionMap, int frameRate = 15, int frameCount = 1);
	
	inline void DrawSprite(int screenX, int screenY, int tile) {
		DrawSpriteCorner(screenX, screenY, tile, TopLeft);
		DrawSpriteCorner(screenX, screenY, tile, TopRight);
		DrawSpriteCorner(screenX, screenY, tile, BottomLeft);
		DrawSpriteCorner(screenX, screenY, tile, BottomRight);
	};
	void DrawSpriteCorner(int screenX, int screenY, int tile, Corner corner);

	void render();

protected:
	void PreDrawMap(int viewportX, int viewportY, int viewportW, int viewportH);
	void PostDrawMap();
	void DrawNullTile(int screenX, int screenY);
	bool TilesetChanged();

private:
	std::vector<RawTileData> rawTiles;
	typedef std::vector<RawTileData>::iterator rawTileIterator;

	// Tiles texture
	boost::shared_ptr<const unsigned int> tilesTexture; 
	unsigned int tilesTextureW;
	unsigned int tilesTextureH;

	// UI Font
	boost::shared_ptr<const unsigned int> fontTexture;
	unsigned int fontCharW, fontCharH;
	unsigned int fontTexW, fontTexH;

	// Console Rendering
	boost::shared_ptr<const unsigned int> consoleProgram;

	enum ConsoleTexureTypes {
		Character,
		ForeCol,
		BackCol,
		ConsoleTextureTypesCount
	};
	boost::array<boost::shared_ptr<const unsigned int>, ConsoleTextureTypesCount> consoleTextures;
	unsigned int consoleTexW, consoleTexH;
	boost::array<std::vector<unsigned char>, ConsoleTextureTypesCount> consoleData;
	static boost::array<unsigned char, ConsoleTextureTypesCount> consoleDataAlignment;
	
	// Viewports
	bool renderInProgress;
	static const int VIEWPORT_LAYERS = 5;
	boost::array<ViewportLayer, VIEWPORT_LAYERS> viewportLayers;
	boost::array<boost::shared_ptr<const unsigned int>, VIEWPORT_LAYERS> viewportTextures;
	struct RenderTile {
		int x;
		int y;
		unsigned int tile;

		explicit RenderTile(int x, int y, unsigned int tile) : x(x), y(y), tile(tile) {}
	};
	std::vector<RenderTile> renderQueue;
	int viewportW, viewportH;
	int viewportTexW, viewportTexH;
	boost::shared_ptr<const unsigned int> viewportProgram;
	
	bool InitialiseConsoleTextures();
	bool InitialiseConsoleShaders();
	
	void RenderViewport();
	void RenderOGLViewport();
	void RenderGLSLViewport();
	void RenderGLSLTile(int tile, int x, int y);
	void RenderOGLTile(int tile, int x, int y);

	void RenderConsole();
	void RenderOGLConsole();
	void RenderGLSLConsole();

	bool AssembleTextures();
};

const bool operator==(const RawTileData& lhs, const RawTileData& rhs);
