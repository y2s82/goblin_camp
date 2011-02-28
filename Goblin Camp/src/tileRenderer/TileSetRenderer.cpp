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
#include "data/Config.hpp"
#include "MathEx.hpp"

#include "tileRenderer/DrawConstructionVisitor.hpp"

namespace {

	// TODO: Move this to MathEx
	int NextPowerOfTwo(int val)
	{
		val--;
		val = (val >> 1) | val;
		val = (val >> 2) | val;
		val = (val >> 4) | val;
		val = (val >> 8) | val;
		val = (val >> 16) | val;
		val++;
		return val;
	}
}


 
TileSetRenderer::TileSetRenderer(int resolutionX, int resolutionY, boost::shared_ptr<TileSet> ts, TCODConsole * mapConsole) 
: tcodConsole(mapConsole),
  permutationTable(10, 473U),
  translucentUI(Config::GetCVar<bool>("translucentUI")),
  screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta),
  mapSurface(),
  tileSet(ts),
  mapOffsetX(0),
  mapOffsetY(0),
  startTileX(0),
  startTileY(0),
  cursorMode(Cursor_None),
  cursorHint(-1) {
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
   SDL_Surface * temp = SDL_CreateRGBSurface(0, NextPowerOfTwo(screenWidth), NextPowerOfTwo(screenHeight), 32, rmask, gmask, bmask, amask);
   SDL_SetAlpha(temp, 0, SDL_ALPHA_OPAQUE);
   mapSurface = boost::shared_ptr<SDL_Surface>(SDL_DisplayFormat(temp), SDL_FreeSurface);
   SDL_FreeSurface(temp);

   if (!mapSurface)
   {
	   LOG(SDL_GetError());
   }
   TCODSystem::registerSDLRenderer(this, translucentUI);
}

TileSetRenderer::~TileSetRenderer() {}


namespace {
	bool TerrainConnectionTest(Map* map, Coordinate origin, TileType type, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->GetType(coord.X(), coord.Y()) == type;
	}

	bool SnowConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		if (map->GetType(coord.X(), coord.Y()) == TILESNOW)
			return true;
		int natNum = -1;
		if ((natNum = map->GetNatureObject(coord.X(), coord.Y())) >= 0) {
			return Game::Inst()->natureList[natNum]->IsIce();
		}
		return false;
	}

	bool CorruptionConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->GetCorruption(coord.X(), coord.Y()) >= 100;
	}

	int WaterConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		int natNum = -1;
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height()) {
			return 2;
		}
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord.X(), coord.Y()).lock()) {
			return (water->Depth() > 0) ? 1 : 0;
		}
		else if ((natNum = map->GetNatureObject(coord.X(), coord.Y())) >= 0) {
			return (Game::Inst()->natureList[natNum]->IsIce()) ? 2 : 0;
		}
		return 0;
	}

	bool WaterNoIceConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height()) {
			return true;
		}
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord.X(), coord.Y()).lock()) {
			return (water->Depth() > 0);
		}
		return false;
	}

	int FilthConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<FilthNode> filth = map->GetFilth(coord.X(), coord.Y()).lock()) {
			return (filth->Depth() > 4) ? 2 : 1;
		}
		return 0;
	}

	bool MajorFilthConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<FilthNode> filth = map->GetFilth(coord.X(), coord.Y()).lock()) {
			return filth->Depth() > 4;
		}
		return false;
	}

	bool BloodConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<BloodNode> blood = map->GetBlood(coord.X(), coord.Y()).lock()) {
			return blood->Depth() > 0;
		}
		return false;
	}
	
	bool TerritoryConnectionTest(Map* map, Coordinate origin, bool owned, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		return map->IsTerritory(coord.X(),coord.Y()) == owned;
	}
	
	bool GroundMarkedConnectionTest(Map * map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		return map->GroundMarked(coord.X(), coord.Y());
	}
}


void TileSetRenderer::PreparePrefabs() 
{
	for (std::vector<NPCPreset>::iterator npci = NPC::Presets.begin(); npci != NPC::Presets.end(); ++npci) {
		npci->graphicsHint = tileSet->GetGraphicsHintFor(*npci);
	}
	for (std::vector<NatureObjectPreset>::iterator nopi = NatureObject::Presets.begin(); nopi != NatureObject::Presets.end(); ++nopi) {
		nopi->graphicsHint = tileSet->GetGraphicsHintFor(*nopi);
	}
	for (std::vector<ItemPreset>::iterator itemi = Item::Presets.begin(); itemi != Item::Presets.end(); ++itemi) {
		itemi->graphicsHint = tileSet->GetGraphicsHintFor(*itemi);
	}
	for (std::vector<ConstructionPreset>::iterator constructi = Construction::Presets.begin(); constructi != Construction::Presets.end(); ++constructi) {
		constructi->graphicsHint = tileSet->GetGraphicsHintFor(*constructi);
	}
	for (std::vector<SpellPreset>::iterator spelli = Spell::Presets.begin(); spelli != Spell::Presets.end(); ++spelli) {
		spelli->graphicsHint = tileSet->GetGraphicsHintFor(*spelli);
	}
}

Coordinate TileSetRenderer::TileAt(int x, int y, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	float left = focusX * tileSet->TileWidth() - viewportW * 0.5f;
	float up = focusY * tileSet->TileHeight() - viewportH * 0.5f;
	
	return Coordinate(FloorToInt::convert((left + x) / tileSet->TileWidth()), FloorToInt::convert((up + y) / tileSet->TileHeight()));
}

void TileSetRenderer::DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	// Merge viewport to console
	if (tcodConsole != 0) {
		int charX, charY;
		TCODSystem::getCharSize(&charX, &charY);
		int offsetX = FloorToInt::convert(float(viewportX) / charX);
		int offsetY = FloorToInt::convert(float(viewportY) / charY);
		int sizeX = CeilToInt::convert(float(viewportW + offsetX * charX) / charX) - offsetX;
		int sizeY = CeilToInt::convert(float(viewportH + offsetY * charY) / charY) - offsetY;
		viewportX = offsetX * charX;
		viewportY = offsetY * charY;
		viewportW = sizeX * charX;
		viewportH = sizeY * charY;

		for (int x = offsetX; x < offsetX + sizeX; x++) {
			for (int y = offsetY; y < offsetY + sizeY; y++) {
				tcodConsole->putCharEx(x, y, ' ', TCODColor::black, keyColor);
			}
		}
	}

	SDL_Rect viewportRect;
	viewportRect.x = viewportX;
	viewportRect.y = viewportY;
	viewportRect.w = viewportW;
	viewportRect.h = viewportH;
	
	SDL_SetClipRect(mapSurface.get(), &viewportRect);
	
	// These are over-estimates, sometimes fewer tiles are needed.
	int startPixelX = FloorToInt::convert(focusX * tileSet->TileWidth() - viewportRect.w / 2);
	int startPixelY = FloorToInt::convert(focusY * tileSet->TileHeight() - viewportRect.h / 2);
	startTileX = FloorToInt::convert(boost::numeric_cast<float>(startPixelX) / tileSet->TileWidth());
	startTileY = FloorToInt::convert(boost::numeric_cast<float>(startPixelY) / tileSet->TileHeight());
	mapOffsetX = startTileX * tileSet->TileWidth() - startPixelX + viewportX;
	mapOffsetY = startTileY * tileSet->TileHeight() - startPixelY + viewportY;
	int tilesX = CeilToInt::convert((focusX * tileSet->TileWidth() + viewportRect.w / 2) / tileSet->TileWidth()) - startTileX;
	int tilesY = CeilToInt::convert((focusY * tileSet->TileHeight() + viewportRect.h / 2) / tileSet->TileHeight()) - startTileY;
	
    // And then render to map
	for (int y = 0; y < tilesY; ++y) {
		for (int x = 0; x <= tilesX; ++x) {
			int tileX = x + startTileX;
			int tileY = y + startTileY;
			
			// Draw Terrain
			SDL_Rect dstRect(CalcDest(tileX, tileY));
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				DrawTerrain(map, tileX, tileY, &dstRect);			
				
				if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
					if (boost::shared_ptr<Construction> construction = (Game::Inst()->GetConstruction(map->GetConstruction(tileX,tileY))).lock()) {
						DrawConstructionVisitor visitor(this, tileSet.get(), map, mapSurface.get(), &dstRect, Coordinate(tileX,tileY));
						construction->AcceptVisitor(visitor);
					} else  {
						DrawFilth(map, tileX, tileY, &dstRect);
					}

					int natNum = map->GetNatureObject(tileX, tileY);
					if (natNum >= 0) {
						boost::shared_ptr<NatureObject> natureObj = Game::Inst()->natureList[natNum];
						if (natureObj->Marked()) {
							tileSet->DrawMarkedOverlay(mapSurface.get(), &dstRect);
						}
						if (!natureObj->IsIce()) {
							tileSet->DrawNatureObject(natureObj, mapSurface.get(), &dstRect);
						}
					}
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					DrawTerritoryOverlay(map, tileX, tileY, &dstRect);
				}
			}
			else {
				// Out of world
				SDL_FillRect(mapSurface.get(), &dstRect, 0);
			}
		}
	}

	DrawMarkers(map, startTileX, startTileY, tilesX, tilesY);

	if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
		DrawItems(startTileX, startTileY, tilesX, tilesY);
	}
	DrawNPCs(startTileX, startTileY, tilesX, tilesY);
	DrawFires(startTileX, startTileY, tilesX, tilesY);
	DrawSpells(startTileX, startTileY, tilesX, tilesY);
	
	if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
		for (int y = 0; y < tilesY; ++y) {
			for (int x = 0; x <= tilesX; ++x) {
				int tileX = x + startTileX;
				int tileY = y + startTileY;

				// Corruption
				if (map->GetCorruption(tileX, tileY) >= 100) {
					SDL_Rect dstRect(CalcDest(tileX, tileY));
					tileSet->DrawCorruptionOverlay(boost::bind(&CorruptionConnectionTest, map, Coordinate(tileX, tileY), _1), mapSurface.get(), &dstRect);
				}
			}
		}
	}

	SDL_SetClipRect(mapSurface.get(), NULL);
}

float TileSetRenderer::ScrollRate() const {
	return 16.0f / (tileSet->TileWidth() + tileSet->TileHeight());
}

void TileSetRenderer::SetCursorMode(CursorType mode) {
	cursorMode = mode;
	cursorHint = -1;
}

void TileSetRenderer::SetCursorMode(const NPCPreset& preset) {
	cursorMode = Cursor_NPC_Mode;
	cursorHint = preset.graphicsHint;
}

void TileSetRenderer::SetCursorMode(const ItemPreset& preset) {
	cursorMode = Cursor_Item_Mode;
	cursorHint = preset.graphicsHint;
}

void TileSetRenderer::SetCursorMode(int other) {
	cursorMode = Cursor_None;
	cursorHint = -1;
}
	
void TileSetRenderer::DrawCursor(const Coordinate& pos, float focusX, float focusY, bool placeable) {
	startTileX = int((focusX * tileSet->TileWidth() - screenWidth / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - screenHeight / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + screenWidth / 2);
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + screenHeight / 2);

	SDL_Rect dstRect(CalcDest(pos.X(), pos.Y()));
	tileSet->DrawCursor(cursorMode, cursorHint, placeable, mapSurface.get(), &dstRect);
}

void TileSetRenderer::DrawCursor(const Coordinate& start, const Coordinate& end, float focusX, float focusY, bool placeable) {
	startTileX = int((focusX * tileSet->TileWidth() - screenWidth / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - screenHeight / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + screenWidth / 2);
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + screenHeight / 2);

	for (int x = start.X(); x <= end.X(); ++x) {
		for (int y = start.Y(); y <= end.Y(); ++y) {
			SDL_Rect dstRect(CalcDest(x, y));
			tileSet->DrawCursor(cursorMode, cursorHint, placeable, mapSurface.get(), &dstRect);
		}
	}
	
}

void TileSetRenderer::DrawItems(int startX, int startY, int sizeX, int sizeY) const {
	for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end(); ++itemi) {
		if (!itemi->second->ContainedIn().lock()) {
			Coordinate itemPos = itemi->second->Position();
			if (itemPos.X() >= startX && itemPos.X() < startX + sizeX
				&& itemPos.Y() >= startY && itemPos.Y() < startY + sizeY)
			{
				SDL_Rect dstRect(CalcDest(itemPos.X(), itemPos.Y()));
				tileSet->DrawItem(itemi->second, mapSurface.get(), &dstRect);
			}
		}
	}
}

void TileSetRenderer::DrawNPCs(int startX, int startY, int sizeX, int sizeY) const {
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
		Coordinate npcPos = npci->second->Position();
		if (npcPos.X() >= startX && npcPos.X() < startX + sizeX
				&& npcPos.Y() >= startY && npcPos.Y() < startY + sizeY)
		{
			SDL_Rect dstRect(CalcDest(npcPos.X(), npcPos.Y()));
			tileSet->DrawNPC(npci->second, mapSurface.get(), &dstRect);
		}
	}
}

void TileSetRenderer::DrawSpells(int startX, int startY, int sizeX, int sizeY) const {
	for (std::list<boost::shared_ptr<Spell> >::iterator spelli = Game::Inst()->spellList.begin(); spelli != Game::Inst()->spellList.end(); ++spelli) {
		Coordinate spellPos = (*spelli)->Position();
		if (spellPos.X() >= startX && spellPos.X() < startX + sizeX
				&& spellPos.Y() >= startY && spellPos.Y() < startY + sizeY)
		{
			SDL_Rect dstRect(CalcDest(spellPos.X(), spellPos.Y()));
			tileSet->DrawSpell(*spelli, mapSurface.get(), &dstRect);
		}
	}
}

void TileSetRenderer::DrawFires(int startX, int startY, int sizeX, int sizeY) const {
	for (std::list<boost::weak_ptr<FireNode> >::iterator firei = Game::Inst()->fireList.begin(); firei != Game::Inst()->fireList.end(); ++firei) {
		if (boost::shared_ptr<FireNode> fire = firei->lock())
		{
			Coordinate firePos = fire->GetPosition();
			if (firePos.X() >= startX && firePos.X() < startX + sizeX
					&& firePos.Y() >= startY && firePos.Y() < startY + sizeY)
			{
				SDL_Rect dstRect(CalcDest(firePos.X(), firePos.Y()));
				tileSet->DrawFire(fire, mapSurface.get(), &dstRect);
			}
		}
	}
}

void TileSetRenderer::DrawMarkers(Map * map, int startX, int startY, int sizeX, int sizeY) const {
	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= startX && markerX < startX + sizeX
			&& markerY >= startY && markerY < startY + sizeY) {
				SDL_Rect dstRect( CalcDest(markerX, markerY));
				tileSet->DrawMarker(mapSurface.get(), &dstRect);
		}
	}
}

void TileSetRenderer::DrawTerrain(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	TileType type(map->GetType(tileX, tileY));
	Coordinate pos(tileX, tileY);

	if (type == TILESNOW) {
		tileSet->DrawTerrain(type, boost::bind(&SnowConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
	} else {
		tileSet->DrawTerrain(type, boost::bind(&TerrainConnectionTest, map, pos, type, _1), mapSurface.get(), dstRect);
	}
	
	if (tileSet->HasTerrainDetails()) {
		int detailIndex = permutationTable.Hash(permutationTable.Hash(tileX) + tileY) % tileSet->GetDetailRange(); 
		tileSet->DrawDetail(detailIndex, mapSurface.get(), dstRect);
	}

	// Corruption
	if (map->GetCorruption(tileX, tileY) >= 100) {
		tileSet->DrawCorruption(boost::bind(&CorruptionConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
	}


	// Water
	if (tileSet->IsIceSupported()) {
		boost::weak_ptr<WaterNode> waterPtr = map->GetWater(tileX,tileY);
		if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
			if (water->Depth() > 0) {
				tileSet->DrawWater(boost::bind(&WaterConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
			}
		}
		int natNum = -1;
		if ((natNum = map->GetNatureObject(tileX, tileY)) >= 0) {
			if (Game::Inst()->natureList[natNum]->IsIce()) {
				tileSet->DrawIce(boost::bind(&WaterConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
			}
		}
	} else {
		boost::weak_ptr<WaterNode> waterPtr = map->GetWater(tileX,tileY);
		if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
			if (water->Depth() > 0) {
				tileSet->DrawWater(boost::bind(&WaterNoIceConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
			}
		}
	}
	if (boost::shared_ptr<BloodNode> blood = map->GetBlood(tileX, tileY).lock()) {
		if (blood->Depth() > 0) {
			tileSet->DrawBlood(boost::bind(&BloodConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
		}
	}
	if (map->GroundMarked(tileX, tileY)) {
		tileSet->DrawMarkedOverlay(boost::bind(&GroundMarkedConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
	}
	
}

void TileSetRenderer::DrawFilth(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	if (boost::shared_ptr<FilthNode> filth = map->GetFilth(tileX, tileY).lock()) {
		if (filth->Depth() > 4) {
			tileSet->DrawFilthMajor(boost::bind(&FilthConnectionTest, map, Coordinate(tileX, tileY), _1), mapSurface.get(), dstRect);
		} else if (filth->Depth() > 0) {
			tileSet->DrawFilthMinor(boost::bind(&FilthConnectionTest, map, Coordinate(tileX, tileY), _1), mapSurface.get(), dstRect);
		}
	}
}

void TileSetRenderer::DrawTerritoryOverlay(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	bool isOwned(map->IsTerritory(tileX,tileY));
	tileSet->DrawTerritoryOverlay(isOwned, boost::bind(&TerritoryConnectionTest, map, Coordinate(tileX, tileY), isOwned, _1), mapSurface.get(), dstRect);
}

namespace {
	inline void setPixelAlpha(SDL_Surface *surface, int x, int y, Uint32 keyColor)
	{
		SDL_PixelFormat *fmt = surface->format;
		int bpp = fmt->BytesPerPixel;
		if (bpp != 4) return;

		Uint32 *p = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch) + x;
		Uint32 c = (*p | fmt->Amask);
		if (c == keyColor) {
			*p = *p & ~fmt->Amask;
		} else if (c == fmt->Amask) {
			*p = (*p & ~fmt->Amask) | (128 << fmt->Ashift);
		}
	}

}

void TileSetRenderer::render(void * surf,void * sdl_screen) {
	SDL_Surface *tcod = (SDL_Surface *)surf;
	SDL_Surface *screen = (SDL_Surface *)sdl_screen;

	SDL_Rect srcRect={0,0,screenWidth,screenHeight};
	SDL_Rect dstRect={0,0,screenWidth,screenHeight};
	SDL_LowerBlit(mapSurface.get(), &srcRect, screen, &dstRect);
	
	if (translucentUI) {
		Uint32 keyColorVal = SDL_MapRGBA(tcod->format, keyColor.r, keyColor.g, keyColor.b, 255);
		if (SDL_MUSTLOCK(tcod))
		{
			SDL_LockSurface(tcod);
		}
		for (int x = 0; x < screenWidth; ++x) {
			for (int y = 0; y < screenHeight; ++y) {
				setPixelAlpha(tcod,x,y, keyColorVal);
			}
		}
		if (SDL_MUSTLOCK(tcod))
		{
			SDL_UnlockSurface(tcod);
		}
	}
	else {
		SDL_SetColorKey(tcod,SDL_SRCCOLORKEY, SDL_MapRGBA(tcod->format, keyColor.r, keyColor.g, keyColor.b, 255));
	}
	SDL_LowerBlit(tcod, &srcRect, screen, &dstRect);
}
