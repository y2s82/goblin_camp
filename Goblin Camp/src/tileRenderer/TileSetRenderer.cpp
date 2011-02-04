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
#include "data/Paths.hpp"
#include "MathEx.hpp"

#include "tileRenderer/DrawConstructionVisitor.hpp"

TileSetRenderer::TileSetRenderer(int resolutionX, int resolutionY, boost::shared_ptr<TileSet> ts, TCODConsole * mapConsole) 
: tcodConsole(mapConsole),
  screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta),
  mapSurface(),
  tempBuffer(),
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
   SDL_Surface * temp = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, rmask, gmask, bmask, amask);
   SDL_SetAlpha(temp, 0, SDL_ALPHA_OPAQUE);
   mapSurface = boost::shared_ptr<SDL_Surface>(SDL_DisplayFormat(temp), SDL_FreeSurface);
   SDL_FreeSurface(temp);

   temp = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, rmask, gmask, bmask, amask);
   tempBuffer = boost::shared_ptr<SDL_Surface>(SDL_DisplayFormat(temp), SDL_FreeSurface);
   SDL_SetColorKey(tempBuffer.get(),SDL_SRCCOLORKEY, SDL_MapRGBA(tempBuffer->format, keyColor.r, keyColor.g, keyColor.b, 255));
   // Make this a future option:
   //SDL_SetAlpha(tempBuffer.get(), SDL_SRCALPHA, 196);
   SDL_FreeSurface(temp);

   if (!mapSurface || !tempBuffer)
   {
	   LOG(SDL_GetError());
   }
   TCODSystem::registerSDLRenderer(this);
}

TileSetRenderer::~TileSetRenderer() {}

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
				tcodConsole->setDirty(x,y,1,1);
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
	startTileX = int((focusX * tileSet->TileWidth() - viewportRect.w / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - viewportRect.h / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + viewportRect.w / 2) + viewportX;
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + viewportRect.h / 2) + viewportY;
	int tilesX = CeilToInt::convert(boost::numeric_cast<float>(viewportRect.w) / tileSet->TileWidth()) + 2;
	int tilesY = CeilToInt::convert(boost::numeric_cast<float>(viewportRect.h) / tileSet->TileHeight()) + 2;
	
    // And then render to map
	for (int y = 0; y < tilesY; ++y) {
		for (int x = 0; x <= tilesX; ++x) {
			int tileX = x + startTileX;
			int tileY = y + startTileY;
			
			// Draw Terrain
			SDL_Rect dstRect(CalcDest(tileX, tileY));
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				DrawTerrain(map, tileX, tileY, &dstRect);
				if (boost::shared_ptr<Construction> construction = (Game::Inst()->GetConstruction(map->GetConstruction(tileX,tileY))).lock()) {
					DrawConstructionVisitor visitor(this, tileSet.get(), map, mapSurface.get(), &dstRect, Coordinate(tileX,tileY));
					construction->AcceptVisitor(visitor);
				} else  {
					DrawFilth(map, tileX, tileY, &dstRect);
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

	DrawNatureObjects(startTileX, startTileY, tilesX, tilesY);
	DrawItems(startTileX, startTileY, tilesX, tilesY);
	DrawNPCs(startTileX, startTileY, tilesX, tilesY);
	DrawFires(startTileX, startTileY, tilesX, tilesY);
	DrawSpells(startTileX, startTileY, tilesX, tilesY);

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

void TileSetRenderer::DrawNatureObjects(int startX, int startY, int sizeX, int sizeY) const {
	for (std::map<int,boost::shared_ptr<NatureObject> >::iterator planti = Game::Inst()->natureList.begin(); planti != Game::Inst()->natureList.end(); ++planti) {
		Coordinate plantPos = planti->second->Position();
		if (plantPos.X() >= startX && plantPos.X() < startX + sizeX
				&& plantPos.Y() >= startY && plantPos.Y() < startY + sizeY)
		{
			SDL_Rect dstRect(CalcDest(plantPos.X(), plantPos.Y()));
			if (planti->second->Marked())
			{
				tileSet->DrawMarkedOverlay(mapSurface.get(), &dstRect);
			}
			tileSet->DrawNatureObject(planti->second, mapSurface.get(), &dstRect);
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

namespace {
	bool TerrainConnectionTest(Map* map, Coordinate origin, TileType type, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->Type(coord.X(), coord.Y()) == type;
	}

	bool CorruptionConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->GetCorruption(coord.X(), coord.Y()) >= 100;
	}

	bool WaterConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height()) {
			return true;
		}
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord.X(), coord.Y()).lock()) {
			return water->Depth() > 0;
		}
		return false;
	}

	int Test(int one, int two) {
		return one + two;
	}
}

void TileSetRenderer::DrawTerrain(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	TileType type(map->Type(tileX, tileY));
	Coordinate pos(tileX, tileY);

	tileSet->DrawTerrain(type, boost::bind(&TerrainConnectionTest, map, pos, type, _1), mapSurface.get(), dstRect);
	
	// Corruption
	if (map->GetCorruption(tileX, tileY) >= 100) {
		tileSet->DrawCorruption(boost::bind(&CorruptionConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
	}


	// Water
	boost::weak_ptr<WaterNode> waterPtr = map->GetWater(tileX,tileY);
	if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
		if (water->Depth() > 0)
		{
			tileSet->DrawWater(boost::bind(&WaterConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
		}
	}
	if (map->GetBlood(tileX, tileY).lock())	{
		tileSet->DrawBlood(mapSurface.get(), dstRect);
	}
	if (map->GroundMarked(tileX, tileY)) {
		tileSet->DrawMarkedOverlay(mapSurface.get(), dstRect);
	}
	
}

void TileSetRenderer::DrawFilth(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	boost::weak_ptr<FilthNode> filth = map->GetFilth(tileX,tileY);
	if (filth.lock() && filth.lock()->Depth() > 5) {
		tileSet->DrawFilthMajor(mapSurface.get(), dstRect);
	} else if (filth.lock() && filth.lock()->Depth() > 0) {
		tileSet->DrawFilthMinor(mapSurface.get(), dstRect);
	}
}

void TileSetRenderer::DrawTerritoryOverlay(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	tileSet->DrawTerritoryOverlay(map->IsTerritory(tileX,tileY), mapSurface.get(), dstRect);
}

void TileSetRenderer::render(void * surf) {
	  SDL_Surface *screen = (SDL_Surface *)surf;

      SDL_Rect srcRect={0,0,screenWidth,screenHeight};
      SDL_Rect dstRect={0,0,screenWidth,screenHeight};
	  SDL_BlitSurface(screen,&srcRect,tempBuffer.get(),&dstRect);
	  SDL_BlitSurface(tempBuffer.get(),&srcRect,mapSurface.get(),&dstRect);
      SDL_BlitSurface(mapSurface.get(),&srcRect,screen,&dstRect);   
}

