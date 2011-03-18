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

#include "tileRenderer/ogl/OGLTilesetRenderer.hpp"
#include "tileRenderer/ogl/OGLSprite.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "MathEx.hpp"
#include "Logger.hpp"

#include "data/paths.hpp"

namespace {

	void CheckGL_Error(const char* GLcall, const char* file, const int line) {
		GLenum errCode;
		if ((errCode = glGetError()) != GL_NO_ERROR) {
			LOG("" << file << "(" << line << "): error " << errCode << ": " << GLcall << std::endl); 
		}
	}

	static std::string TCOD_con_vertex_shader =
#ifndef NDEBUG
	"#version 110\n"
#endif
	"uniform vec2 termsize; "

	"void main(void) "
	"{ "

	"   gl_Position = gl_Vertex; "

	"   gl_TexCoord[0] = gl_MultiTexCoord0; "
	"   gl_TexCoord[0].x = gl_TexCoord[0].x*termsize.x; "
	"   gl_TexCoord[0].y = gl_TexCoord[0].y*termsize.y; "
	"} "
	;

	static std::string TCOD_con_pixel_shader =
#ifndef NDEBUG
	"#version 110\n"
#endif
	"uniform sampler2D font; "
	"uniform sampler2D term; "
	"uniform sampler2D termfcol; "
	"uniform sampler2D termbcol; "

	"uniform float fontw; "
	"uniform vec2 fontcoef; "
	"uniform vec2 termsize; "
	"uniform vec2 termcoef; "

	"void main(void) "
	"{ "
	"   vec2 rawCoord = gl_TexCoord[0].xy; "                           /* varying from [0, termsize) in x and y */
	"   vec2 conPos = floor(rawCoord); "                               /* console integer position */
	"   vec2 pixPos = fract(rawCoord); "                               /* pixel offset within console position */
	"   pixPos = vec2(pixPos.x*fontcoef.x,pixPos.y*fontcoef.y); "      /* Correct pixel offset for font tex location */

	"   vec2 address = vec2(conPos.x*termcoef.x,conPos.y*termcoef.y); "
	"	address=address+vec2(0.001, 0.001); "
	"   float inchar = texture2D(term, address).r*256.0; "         /* character */
	"   vec4 tcharfcol = texture2D(termfcol, address); "           /* front color */
	"   vec4 tcharbcol = texture2D(termbcol, address); "           /* back color */

	"   vec4 tchar = vec4(mod(floor(inchar),floor(fontw)),floor(inchar/fontw), 0.0, 0.0); "  /* 1D index to 2D index map for character */

	"   gl_FragColor = texture2D(font, vec2((tchar.x*fontcoef.x),(tchar.y*fontcoef.y))+pixPos.xy); "   /* magic func: finds pixel value in font file */

	"   gl_FragColor=gl_FragColor.a*tcharfcol+(1.0-gl_FragColor.a)*tcharbcol;  "      /* Coloring stage */
	"} "
	;


	static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB=0;
	static PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB=0;
	static PFNGLSHADERSOURCEARBPROC glShaderSourceARB=0;
	static PFNGLCOMPILESHADERARBPROC glCompileShaderARB=0;
	static PFNGLGETINFOLOGARBPROC glGetInfoLogARB=0;
	static PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB=0;
	static PFNGLATTACHOBJECTARBPROC glAttachObjectARB=0;
	static PFNGLLINKPROGRAMARBPROC glLinkProgramARB=0;
	static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB=0;
	static PFNGLUNIFORM2FARBPROC glUniform2fARB=0;
	static PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB=0;
	static PFNGLUNIFORM1FARBPROC glUniform1fARB=0;
	static PFNGLUNIFORM1IARBPROC glUniform1iARB=0;
	#ifdef TCOD_WINDOWS
	static PFNGLACTIVETEXTUREPROC glActiveTexture=0;
	#endif
}


OGLTilesetRenderer::OGLTilesetRenderer(int screenWidth, int screenHeight, TCODConsole * mapConsole)
: TilesetRenderer(screenWidth, screenHeight, mapConsole),
  rawTiles(),
  texture(0),
  viewportW(0), viewportH(0),
  textureTilesW(0), textureTilesH(0),
  fontCharW(0), fontCharH(0),
  fontTexW(0), fontTexH(0),
  consoleTexW(0), consoleTexH(0),
  viewportDrawStack()
{
	TCODSystem::registerOGLRenderer(this);
	viewportDrawStack.resize(boost::extents[0][0]);	

	glCreateShaderObjectARB=(PFNGLCREATESHADEROBJECTARBPROC)SDL_GL_GetProcAddress("glCreateShaderObjectARB");
	glGetObjectParameterivARB=(PFNGLGETOBJECTPARAMETERIVARBPROC)SDL_GL_GetProcAddress("glGetObjectParameterivARB");
	glShaderSourceARB=(PFNGLSHADERSOURCEARBPROC)SDL_GL_GetProcAddress("glShaderSourceARB");
	glCompileShaderARB=(PFNGLCOMPILESHADERARBPROC)SDL_GL_GetProcAddress("glCompileShaderARB");
	glGetInfoLogARB=(PFNGLGETINFOLOGARBPROC)SDL_GL_GetProcAddress("glGetInfoLogARB");
	glCreateProgramObjectARB=(PFNGLCREATEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glCreateProgramObjectARB");
	glAttachObjectARB=(PFNGLATTACHOBJECTARBPROC)SDL_GL_GetProcAddress("glAttachObjectARB");
	glLinkProgramARB=(PFNGLLINKPROGRAMARBPROC)SDL_GL_GetProcAddress("glLinkProgramARB");
	glUseProgramObjectARB=(PFNGLUSEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glUseProgramObjectARB");
	glUniform2fARB=(PFNGLUNIFORM2FARBPROC)SDL_GL_GetProcAddress("glUniform2fARB");
	glGetUniformLocationARB=(PFNGLGETUNIFORMLOCATIONARBPROC)SDL_GL_GetProcAddress("glGetUniformLocationARB");
	glUniform1fARB=(PFNGLUNIFORM1FARBPROC)SDL_GL_GetProcAddress("glUniform1fARB");
	glUniform1iARB=(PFNGLUNIFORM1IARBPROC)SDL_GL_GetProcAddress("glUniform1iARB");
#ifdef TCOD_WINDOWS	
	glActiveTexture=(PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
#endif

	Uint32 rmask, gmask, bmask, amask;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	} else {
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	}

	boost::shared_ptr<SDL_Surface> fontSurface(IMG_Load(Paths::Get(Paths::Font).string().c_str()), SDL_FreeSurface);
	fontCharW = fontSurface->w / 16;
	fontCharH = fontSurface->h / 16;
	fontTexW = MathEx::NextPowerOfTwo(fontCharW * 16);
	fontTexH = MathEx::NextPowerOfTwo(fontCharH * 16);

	SDL_SetColorKey(fontSurface.get(), SDL_SRCCOLORKEY, SDL_MapRGB(fontSurface->format, 0, 0, 0));
	boost::shared_ptr<SDL_Surface> tempAlpha(SDL_DisplayFormatAlpha(fontSurface.get()), SDL_FreeSurface);
	SDL_SetAlpha(tempAlpha.get(), 0, SDL_ALPHA_TRANSPARENT);

	boost::shared_ptr<SDL_Surface> temp(SDL_CreateRGBSurface(SDL_SWSURFACE, fontTexW, fontTexH, 32, bmask, gmask, rmask, amask), SDL_FreeSurface);
	SDL_BlitSurface(tempAlpha.get(), NULL, temp.get(), NULL);

	glGenTextures(1, &fontTexture);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	SDL_LockSurface(temp.get());

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, temp->w, temp->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_UnlockSurface(temp.get());

	consoleTexW = MathEx::NextPowerOfTwo(screenWidth / fontCharW);
	consoleTexH = MathEx::NextPowerOfTwo(screenHeight / fontCharH);
}

OGLTilesetRenderer::~OGLTilesetRenderer() {}

Sprite_ptr OGLTilesetRenderer::CreateSprite(SpriteLayerType spriteLayer, boost::shared_ptr<TileSetTexture> tilesetTexture, int tile) {
	RawTileData rawTile = {tile, tilesetTexture};
	rawTileIterator existing = std::find(rawTiles.begin(), rawTiles.end(), rawTile);
	if (existing != rawTiles.end()) {
		return Sprite_ptr(new OGLSprite(this, existing - rawTiles.begin()));
	} else {
		int id = rawTiles.size();
		rawTiles.push_back(rawTile);
		return Sprite_ptr(new OGLSprite(this, id));
	}
}

Sprite_ptr OGLTilesetRenderer::CreateSprite(SpriteLayerType spriteLayer, boost::shared_ptr<TileSetTexture> tilesetTexture, const std::vector<int>& tiles, bool connectionMap, int frameRate, int frameCount) {
	if (tiles.empty())
		return Sprite_ptr();

	std::vector<int> tileIds;
	for (std::vector<int>::const_iterator tileIter = tiles.begin(); tileIter != tiles.end(); ++tileIter) {
		RawTileData rawTile = {*tileIter, tilesetTexture};
		rawTileIterator existing = std::find(rawTiles.begin(), rawTiles.end(), rawTile);
		if (existing != rawTiles.end()) {
			tileIds.push_back(existing - rawTiles.begin());
		} else {
			tileIds.push_back(rawTiles.size());
			rawTiles.push_back(rawTile);
		}
	}
	return Sprite_ptr(new OGLSprite(this, tileIds.begin(), tileIds.end(), connectionMap, frameRate, frameCount));
}
	
void OGLTilesetRenderer::DrawSprite(int screenX, int screenY, int tile)  {
	viewportDrawStack[2 * screenX][2 * screenY].push_back(tile);
	viewportDrawStack[2 * screenX + 1][2 * screenY].push_back(tile);
	viewportDrawStack[2 * screenX][2 * screenY + 1].push_back(tile);
	viewportDrawStack[2 * screenX + 1][2 * screenY + 1].push_back(tile);
}

void OGLTilesetRenderer::DrawSpriteCorner(int screenX, int screenY, int tile, Corner corner) {
	viewportDrawStack[2 * screenX + (corner & 0x1)][2 * screenY + ((corner & 0x2) >> 1)].push_back(tile);
}

void OGLTilesetRenderer::TilesetChanged() {
	viewportW = CeilToInt::convert(boost::numeric_cast<float>(GetScreenWidth()) / tileSet->TileWidth()) + 2;
	viewportH = CeilToInt::convert(boost::numeric_cast<float>(GetScreenHeight()) / tileSet->TileHeight()) + 2;

	// Twice the viewport size, so we can have different corners
	viewportDrawStack.resize(boost::extents[2 * viewportW][2 * viewportH]);
	InitaliseShaders();
}

void OGLTilesetRenderer::AssembleTextures() {
	GLint texSize; 
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	
	GLint width(0); 
	while (width == 0 && texSize > tileSet->TileWidth()) {
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL); 
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); 
		if (width == 0)
			texSize /= 2;
	}
	if (width == 0)
		return; // TODO: Error out

	// Get initial horizontal tiles (based on tex size)
	textureTilesW = std::min(texSize / tileSet->TileWidth(), boost::numeric_cast<GLint>(rawTiles.size()));
	GLint widthPixels = std::min(texSize, MathEx::NextPowerOfTwo(tileSet->TileWidth() * textureTilesW));
	// Final horizontal tiles
	textureTilesW = widthPixels / tileSet->TileWidth();

	// Vertical size calculated based on size needed based on width.
	GLint heightPixels = std::min(texSize, MathEx::NextPowerOfTwo(CeilToInt::convert((float)rawTiles.size() / textureTilesW) * tileSet->TileHeight()));
	textureTilesH = heightPixels / tileSet->TileHeight();

	// TODO: Error out
	if (textureTilesH * textureTilesW < rawTiles.size())
		return;

	// Build texture
	Uint32 rmask, gmask, bmask, amask;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	} else {
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	}
	boost::shared_ptr<SDL_Surface> tempSurface(SDL_CreateRGBSurface(SDL_SWSURFACE, widthPixels, heightPixels, 32, bmask, gmask, rmask, amask), SDL_FreeSurface);
	SDL_FillRect(tempSurface.get(), 0, SDL_MapRGBA(tempSurface->format, 0,0,0,0));
	int tile = 0;
	for (int y = 0; y < textureTilesH && tile < rawTiles.size(); ++y) {
		for (int x = 0; x < textureTilesW && tile < rawTiles.size(); ++x) {
			SDL_Rect target = {x * tileSet->TileWidth(), y * tileSet->TileHeight(), tileSet->TileWidth(), tileSet->TileHeight()};
			SDL_SetAlpha(rawTiles[tile].texture->GetInternalSurface().get(), 0, SDL_ALPHA_OPAQUE);
			rawTiles[tile].texture->DrawTile(rawTiles[tile].tile, tempSurface.get(), &target);
			tile++;
		}
	}
	rawTiles.clear();

	glGenTextures(1, &texture);
	CheckGL_Error("glGenTextures", __FILE__, __LINE__);
	glBindTexture(GL_TEXTURE_2D, texture);
	CheckGL_Error("glBindTexture", __FILE__, __LINE__);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	if (SDL_MUSTLOCK(tempSurface.get())) {
		SDL_LockSurface(tempSurface.get());
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthPixels, heightPixels, 0, GL_BGRA, GL_UNSIGNED_BYTE, tempSurface->pixels);
	if (SDL_MUSTLOCK(tempSurface.get())) {
		SDL_UnlockSurface(tempSurface.get());
	}
	CheckGL_Error("glTexImage2D", __FILE__, __LINE__);
}

void OGLTilesetRenderer::render() {
	glClearColor(0,0,0,255);
	glClear(GL_COLOR_BUFFER_BIT);
	int consoleW = TCODConsole::root->getWidth();
	int consoleH = TCODConsole::root->getHeight();
	for (int x = 0; x < TCODConsole::root->getWidth(); ++x) {
		for (int y = 0; y < TCODConsole::root->getHeight(); ++y) {
			TCODColor backCol = TCODConsole::root->getCharBackground(x,y);
			unsigned char alpha = (backCol == GetKeyColor()) ? 0 : ((backCol == TCODColor::black) ? 128 : 255);
			consoleData[BackCol][4 * (x + y * consoleW)] = backCol.r;
			consoleData[BackCol][4 * (x + y * consoleW) + 1] = backCol.g;
			consoleData[BackCol][4 * (x + y * consoleW) + 2] = backCol.b;
			consoleData[BackCol][4 * (x + y * consoleW) + 3] = alpha;

			unsigned char c = TCODConsole::root->getCharCode(x,y);
			if (c == -1) c = TCODSystem::asciiToTCOD(TCODConsole::root->getChar(x,y));
			consoleData[Character][x + y * consoleW] = c;
			if (c != 0) {
				TCODColor foreCol = TCODConsole::root->getCharForeground(x,y); 
				consoleData[ForeCol][3 * (x + y * consoleW)] = foreCol.r;
				consoleData[ForeCol][3 * (x + y * consoleW) + 1] = foreCol.g;
				consoleData[ForeCol][3 * (x + y * consoleW) + 2] = foreCol.b;
			} else {
				consoleData[Character][x + y * consoleW] = 0;
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, consoleTextures[Character]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tcodConsole->getWidth(), tcodConsole->getHeight(), GL_RED, GL_UNSIGNED_BYTE, consoleData[Character].begin()._Ptr);

	glBindTexture(GL_TEXTURE_2D, consoleTextures[ForeCol]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tcodConsole->getWidth(), tcodConsole->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, consoleData[ForeCol].begin()._Ptr);

	glBindTexture(GL_TEXTURE_2D, consoleTextures[BackCol]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tcodConsole->getWidth(), tcodConsole->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, consoleData[BackCol].begin()._Ptr);
	
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, texture);
	CheckGL_Error("glBindTexture", __FILE__, __LINE__);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	glColor4f(1.0,1.0,1.0,1.0);

	float texCoordTileW = 0.5f / textureTilesW;
	float texCoordTileH = 0.5f / textureTilesH;

	float offsetX = (float) mapOffsetX / tileSet->TileWidth();
	float offsetY = (float) mapOffsetY / tileSet->TileHeight();

	for (int x = 0; x < 2 * viewportW; ++x) {
		for (int y = 0; y < 2 * viewportH; ++y) {
			for (std::vector<unsigned int>::iterator tile = viewportDrawStack[x][y].begin(); tile != viewportDrawStack[x][y].end(); ++tile) {
				unsigned int srcX = 2 * (*tile % textureTilesW);
				unsigned int srcY = 2 * (*tile / textureTilesW);
				srcX += (x & 0x1);
				srcY += (y & 0x1);
				glTexCoord2f(srcX * texCoordTileW, srcY * texCoordTileH );
				glVertex2f( offsetX + x, -offsetY + viewportH * 2 - y - 4);
				glTexCoord2f(srcX * texCoordTileW, (srcY+1)*texCoordTileH );
				glVertex2f( offsetX + x, -offsetY + viewportH * 2 - y - 5);
				glTexCoord2f((srcX+1)*texCoordTileW, (srcY+1)*texCoordTileH );
				glVertex2f( offsetX + x+1, -offsetY + viewportH * 2 - y - 5);
				glTexCoord2f((srcX+1)*texCoordTileW, srcY*texCoordTileH );
				glVertex2f( offsetX + x+1, -offsetY + viewportH * 2 - y - 4);
			}
		}
	}
	glEnd();
	CheckGL_Error("render", __FILE__, __LINE__);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	/* rendering console */
	glUseProgramObjectARB(consoleProgram);
	
	/* Technically all these glUniform calls can be moved to SFConsole() when the shader is loaded */
	/* None of these change */
	/* The Textures still need to bind to the same # Activetexture throughout though */
	glUniform2fARB(glGetUniformLocationARB(consoleProgram,"termsize"), (float) tcodConsole->getWidth(), (float) tcodConsole->getHeight());
	glUniform2fARB(glGetUniformLocationARB(consoleProgram,"termcoef"), 1.0f/consoleTexW, 1.0f/consoleTexH);
	glUniform1fARB(glGetUniformLocationARB(consoleProgram,"fontw"), (float)16);
	glUniform2fARB(glGetUniformLocationARB(consoleProgram,"fontcoef"), (float)(fontCharW)/(fontTexW), (float)(fontCharH)/(fontTexH));

	
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
	glUniform1iARB(glGetUniformLocationARB(consoleProgram,"font"),0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, consoleTextures[Character]);
	glUniform1iARB(glGetUniformLocationARB(consoleProgram,"term"),1);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, consoleTextures[ForeCol]);
	glUniform1iARB(glGetUniformLocationARB(consoleProgram,"termfcol"),2);
	
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, consoleTextures[BackCol]);
	glUniform1iARB(glGetUniformLocationARB(consoleProgram,"termbcol"),3);
	
	glBegin(GL_QUADS);
	    glTexCoord2f(0.0f, 1.0f);
		glVertex3f(-1.0f,-1.0f,0.0f);
	    glTexCoord2f(1.0f, 1.0f);
		glVertex3f(1.0f,-1.0f,0.0f);
	    glTexCoord2f(1.0f, 0.0f);
		glVertex3f(1.0f,1.0f, 0.0f);
	    glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f,1.0f,0.0f);
	glEnd();
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgramObjectARB(0);
}

void OGLTilesetRenderer::PreDrawMap() {
	for (viewportColumnIterator col = viewportDrawStack.begin(); col != viewportDrawStack.end(); ++col) {
		for (viewportRowIterator row = (*col).begin(); row != (*col).end(); ++row) {
			(*row).clear();
		}
	}
}

void OGLTilesetRenderer::PostDrawMap() {
}

void OGLTilesetRenderer::DrawNullTile(int screenX, int screenY) {
}

const bool operator==(const RawTileData& lhs, const RawTileData& rhs) {
	return lhs.tile == rhs.tile && lhs.texture == rhs.texture;
}

GLuint OGLTilesetRenderer::LoadShader(std::string shader, GLuint type) {
	int success;
	int infologLength = 0;
	int charsWritten = 0;
    char *infoLog;
	GLuint v = glCreateShaderObjectARB(type);
	const char * shaderTxt = shader.c_str();
	glShaderSourceARB(v, 1, &shaderTxt, 0);
	glCompileShaderARB(v);

	glGetObjectParameterivARB(v, GL_COMPILE_STATUS, &success);
	if(success!=GL_TRUE)
	{
	    /* something went wrong */
		glGetObjectParameterivARB(v, GL_INFO_LOG_LENGTH,&infologLength);
		if(infologLength>0)
		{
			infoLog = (char *)malloc(infologLength);
			glGetInfoLogARB(v, infologLength, &charsWritten, infoLog);
			LOG("GLSL ERROR : " << infoLog);
			free(infoLog);
		}
		return 0;
	}

	return v;
}

bool OGLTilesetRenderer::LoadProgram(std::string vertShaderCode, std::string fragShaderCode, GLuint *vertShader, GLuint *fragShader, GLuint *program) {
	/* Create and load Program and Shaders */
	int success;
	*program = glCreateProgramObjectARB();

	*vertShader = LoadShader(vertShaderCode, GL_VERTEX_SHADER);
	if ( *vertShader == 0 ) return false;
	glAttachObjectARB(*program, *vertShader);

	*fragShader = LoadShader(fragShaderCode, GL_FRAGMENT_SHADER);
	if ( *fragShader == 0 ) return false;
	glAttachObjectARB(*program, *fragShader);

	glLinkProgramARB(*program);

	glGetObjectParameterivARB(*program, GL_LINK_STATUS, &success);
	if(success!=GL_TRUE)
	{
		/* something went wrong */
		int infologLength = 0;
		int charsWritten = 0;
		char *infoLog;
		glGetObjectParameterivARB(*program, GL_INFO_LOG_LENGTH,&infologLength);
		if (infologLength > 0)
	    {
	        infoLog = (char *)malloc(infologLength);
	        glGetInfoLogARB(*program, infologLength, &charsWritten, infoLog);
			printf("OPENGL ERROR: Program link Error");
			printf("%s\n",infoLog);
	        free(infoLog);
	    }
		return false;
	}
	return true;
}

boost::array<unsigned char, OGLTilesetRenderer::ConsoleTextureTypesCount> OGLTilesetRenderer::consoleDataAlignment = {1, 3, 4};

bool OGLTilesetRenderer::InitaliseShaders() {
	int i;
	TCOD_color_t *fCol;
	if (!LoadProgram(TCOD_con_vertex_shader, TCOD_con_pixel_shader, &consoleVertShader, &consoleFragShader, &consoleProgram)) return false;
	
    /* Generate Textures */
	GLuint tempTex[ConsoleTextureTypesCount];
	glGenTextures(ConsoleTextureTypesCount, tempTex);
	for (int i = 0; i < ConsoleTextureTypesCount; ++i) {
		consoleTextures[i] = tempTex[i];
		consoleData[i] = std::vector<unsigned char>(consoleDataAlignment[i] * tcodConsole->getWidth() * tcodConsole->getHeight());
		consoleDirty[i] = true;
	}

	/* Character Texture */
	glBindTexture(GL_TEXTURE_2D, consoleTextures[Character]);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, consoleTexW, consoleTexH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

    /* ForeCol Texture */
	glBindTexture(GL_TEXTURE_2D, consoleTextures[ForeCol]);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, consoleTexW, consoleTexH, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    /* BackCol Texture */
	glBindTexture(GL_TEXTURE_2D, consoleTextures[BackCol]);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, consoleTexW, consoleTexH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	
	return true;
}