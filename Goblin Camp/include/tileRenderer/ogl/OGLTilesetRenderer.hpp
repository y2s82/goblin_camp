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

#include "tileRenderer/TilesetRenderer.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include <boost/multi_array.hpp>

struct RawTileData {
	unsigned int tile;
	boost::shared_ptr<TileSetTexture> texture;
};

class OGLTilesetRenderer : public TilesetRenderer, public ITCODOGLRenderer
{
public:
	explicit OGLTilesetRenderer(int screenWidth, int screenHeight, TCODConsole * mapConsole = 0);
	~OGLTilesetRenderer();

	Sprite_ptr CreateSprite(SpriteLayerType spriteLayer, boost::shared_ptr<TileSetTexture> tilesetTexture, int tile);
	Sprite_ptr CreateSprite(SpriteLayerType spriteLayer, boost::shared_ptr<TileSetTexture> tilesetTexture, const std::vector<int>& tiles, bool connectionMap, int frameRate = 15, int frameCount = 1);
	
	void DrawSprite(int screenX, int screenY, int tile);
	void DrawSpriteCorner(int screenX, int screenY, int tile, Corner corner);

	void AssembleTextures();

	void render();

protected:
	void PreDrawMap();
	void PostDrawMap();
	void DrawNullTile(int screenX, int screenY);
	void TilesetChanged();

private:
	std::vector<RawTileData> rawTiles;
	typedef std::vector<RawTileData>::iterator rawTileIterator;

	// TODO: Make this OGL GLuint (need to prevent SDL\SDLOpenGL.h leaking out)
	unsigned int texture; 
	unsigned int textureTilesW;
	unsigned int textureTilesH;

	unsigned int fontTexture;
	unsigned int fontCharW, fontCharH;
	unsigned int fontTexW, fontTexH;
	unsigned int consoleProgram, consoleVertShader, consoleFragShader;

	enum ConsoleTexureTypes {
		Character,
		ForeCol,
		BackCol,
		ConsoleTextureTypesCount
	};
	boost::array<unsigned int, ConsoleTextureTypesCount> consoleTextures;
	unsigned int consoleTexW, consoleTexH;
	boost::array<std::vector<unsigned char>, ConsoleTextureTypesCount> consoleData;
	static boost::array<unsigned char, ConsoleTextureTypesCount> consoleDataAlignment;
	boost::array<bool, ConsoleTextureTypesCount> consoleDirty;
	
	
	boost::multi_array<std::vector<unsigned int>, 2> viewportDrawStack;
	typedef boost::multi_array<std::vector<unsigned int>, 2>::iterator viewportColumnIterator;
	typedef boost::multi_array<std::vector<unsigned int>, 2>::subarray<1>::type::iterator viewportRowIterator;
	int viewportW, viewportH;

	unsigned int LoadShader(std::string shader, unsigned int type);
	bool LoadProgram(std::string vertShaderCode, std::string fragShaderCode, unsigned int *vertShader, unsigned int *fragShader, unsigned int *program);
	bool InitaliseShaders();
};

const bool operator==(const RawTileData& lhs, const RawTileData& rhs);