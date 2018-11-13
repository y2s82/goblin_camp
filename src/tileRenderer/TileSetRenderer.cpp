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
#include "Game.hpp"
#include "MathEx.hpp"

#include "tileRenderer/DrawConstructionVisitor.hpp"
 
TilesetRenderer::TilesetRenderer(int resolutionX, int resolutionY, TCODConsole * mapConsole) 
: tcodConsole(mapConsole),
  permutationTable(10, 473U),
  tileSet(),
  translucentUI(false),
  startPixelX(0),
  startPixelY(0),
  pixelW(0),
  pixelH(0),
  mapOffsetX(0),
  mapOffsetY(0),
  startTileX(0),
  startTileY(0),
  tilesX(0),
  tilesY(0),
  cursorMode(Cursor_None),
  cursorHint(-1),
  screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta)
{ 
}

TilesetRenderer::~TilesetRenderer() {}


namespace {
	bool TerrainConnectionTest(Map* map, Coordinate origin, TileType type, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->GetType(coord) == type;
	}

	bool GrassConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (!map->IsInside(coord))
			return true;
		TileType type = map->GetType(coord);
		return type == TILEGRASS || type == TILESNOW;
	}

	bool SnowConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (!map->IsInside(coord))
			return true;
		if (map->GetType(coord) == TILESNOW)
			return true;
		int natNum = -1;
		if ((natNum = map->GetNatureObject(coord)) >= 0) {
			return Game::Inst()->natureList[natNum]->IsIce();
		}
		return false;
	}

	bool CorruptionConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (!map->IsInside(coord))
			return true;
		return map->GetCorruption(coord) >= 100;
	}

	bool BurntConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (!map->IsInside(coord))
			return true;
		return map->GetType(coord) == TILEGRASS && map->Burnt(coord) >= 10;
	}

	int WaterConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		int natNum = -1;
		if (!map->IsInside(coord))
			return 2;
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord).lock()) {
			return (water->Depth() > 0) ? 1 : 0;
		}
		else if ((natNum = map->GetNatureObject(coord)) >= 0) {
			return (Game::Inst()->natureList[natNum]->IsIce()) ? 2 : 0;
		}
		return 0;
	}

	bool WaterNoIceConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (!map->IsInside(coord))
			return true;
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord).lock()) {
			return (water->Depth() > 0);
		}
		return false;
	}

	int FilthConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<FilthNode> filth = map->GetFilth(coord).lock()) {
			return (filth->Depth() > 4) ? 2 : 1;
		}
		return 0;
	}

	bool MajorFilthConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<FilthNode> filth = map->GetFilth(coord).lock()) {
			return filth->Depth() > 4;
		}
		return false;
	}

	bool BloodConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<BloodNode> blood = map->GetBlood(coord).lock()) {
			return blood->Depth() > 0;
		}
		return false;
	}
	
	bool TerritoryConnectionTest(Map* map, Coordinate origin, bool owned, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		return map->IsTerritory(coord) == owned;
	}
	
	bool GroundMarkedConnectionTest(Map * map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		return map->GroundMarked(coord);
	}
}


void TilesetRenderer::PreparePrefabs() 
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

Coordinate TilesetRenderer::TileAt(int x, int y, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	float left = focusX * tileSet->TileWidth() - viewportW * 0.5f;
	float up = focusY * tileSet->TileHeight() - viewportH * 0.5f;
	
	return Coordinate(FloorToInt::convert((left + x) / tileSet->TileWidth()), FloorToInt::convert((up + y) / tileSet->TileHeight()));
}

void TilesetRenderer::DrawMap(Map* mapToDraw, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	// Merge viewport to console (because the console is composed of discrete tiles, expand to fill the space used to render the world).
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

	PreDrawMap(viewportX, viewportY, viewportW, viewportH);
	
	// Setup rendering state
	map = mapToDraw;
	startPixelX = viewportX;
	startPixelY = viewportY;
	int absStartPixelX = FloorToInt::convert(focusX * tileSet->TileWidth() - viewportW / 2);
	int absStartPixelY = FloorToInt::convert(focusY * tileSet->TileHeight() - viewportH / 2);
	pixelW = viewportW;
	pixelH = viewportH;
	startTileX = FloorToInt::convert(boost::numeric_cast<float>(absStartPixelX) / tileSet->TileWidth());
	startTileY = FloorToInt::convert(boost::numeric_cast<float>(absStartPixelY) / tileSet->TileHeight());
	mapOffsetX = startTileX * tileSet->TileWidth() - absStartPixelX;
	mapOffsetY = startTileY * tileSet->TileHeight() - absStartPixelY;
	tilesX = CeilToInt::convert((focusX * tileSet->TileWidth() + viewportW / 2) / tileSet->TileWidth()) - startTileX;
	tilesY = CeilToInt::convert((focusY * tileSet->TileHeight() + viewportH / 2) / tileSet->TileHeight()) - startTileY;

    // And then render to map
	for (int y = 0; y < tilesY; ++y) {
		for (int x = 0; x <= tilesX; ++x) {
			Coordinate pos(x + startTileX, y + startTileY);
			
			// Draw Terrain
			if (map->IsInside(pos)) {
				DrawTerrain(x, y, pos);			
				
				if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
					if (boost::shared_ptr<Construction> construction = (Game::Inst()->GetConstruction(map->GetConstruction(pos))).lock()) {
						DrawConstructionVisitor visitor(this, tileSet.get(), x, y, pos);
						construction->AcceptVisitor(visitor);
					} else  {
						DrawFilth(x, y, pos);
					}

					int natNum = map->GetNatureObject(pos);
					if (natNum >= 0) {
						boost::shared_ptr<NatureObject> natureObj = Game::Inst()->natureList[natNum];
						if (natureObj->Marked()) {
							tileSet->DrawMarkedOverlay(x, y);
						}
						if (!natureObj->IsIce()) {
							tileSet->DrawNatureObject(x, y, natureObj);
						}
					}
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					DrawTerritoryOverlay(x, y, pos);
				}
			}
			else {
				// Out of world
				DrawNullTile(x, y);
			}
		}
	}

	DrawMarkers();

	if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
		DrawItems();
	}
	DrawNPCs();
	DrawFires();
	DrawSpells();
	
	if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
		for (int y = 0; y < tilesY; ++y) {
			for (int x = 0; x <= tilesX; ++x) {
				Coordinate tile(x+startTileX, y+startTileY);
				// Corruption
				if (map->GetCorruption(tile) >= 100) {
					TileType type = map->GetType(tile);
					const TerrainSprite& terrainSprite = (type == TILESNOW) ? tileSet->GetTerrainSprite(TILEGRASS) : tileSet->GetTerrainSprite(type);
					terrainSprite.DrawCorruptionOverlay(x, y, boost::bind(&CorruptionConnectionTest, map, tile, _1));
				}
			}
		}
	}

	PostDrawMap();
}

float TilesetRenderer::ScrollRate() const {
	return 16.0f / (tileSet->TileWidth() + tileSet->TileHeight());
}

void TilesetRenderer::SetCursorMode(CursorType mode) {
	cursorMode = mode;
	cursorHint = -1;
}

void TilesetRenderer::SetCursorMode(const NPCPreset& preset) {
	cursorMode = Cursor_NPC_Mode;
	cursorHint = preset.graphicsHint;
}

void TilesetRenderer::SetCursorMode(const ItemPreset& preset) {
	cursorMode = Cursor_Item_Mode;
	cursorHint = preset.graphicsHint;
}

void TilesetRenderer::SetCursorMode(int other) {
	cursorMode = Cursor_None;
	cursorHint = -1;
}
	
void TilesetRenderer::DrawCursor(const Coordinate& pos, bool placeable) {
	tileSet->DrawCursor(pos.X() - startTileX, pos.Y() - startTileY, cursorMode, cursorHint, placeable);
}

void TilesetRenderer::DrawCursor(const Coordinate& start, const Coordinate& end, bool placeable) {
	for (int x = start.X(); x <= end.X(); ++x) {
		for (int y = start.Y(); y <= end.Y(); ++y) {
			tileSet->DrawCursor(x - startTileX, y - startTileY, cursorMode, cursorHint, placeable);
		}
	}
}

//TODO factorize all those DrawFoo
void TilesetRenderer::DrawItems() const {
	for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end(); ++itemi) {
      if (itemi->second == 0) { // should not be here. but it happens. null pointer
          itemi = Game::Inst()->itemList.erase(itemi); // delete this shit
          if ( itemi == Game::Inst()->itemList.end() )break;
          continue;
      }

		if (!itemi->second->ContainedIn().lock()) {
			Coordinate itemPos = itemi->second->Position();
			Coordinate start(startTileX,startTileY), extent(tilesX,tilesY);
			if (itemPos.insideExtent(start, extent))
				tileSet->DrawItem((itemPos-start).X(), (itemPos-start).Y(), itemi->second);
		}
	}
}

void TilesetRenderer::DrawNPCs() const {
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
		Coordinate npcPos = npci->second->Position();
		Coordinate start(startTileX,startTileY), extent(tilesX,tilesY);
		if (npcPos.insideExtent(start, extent))
			tileSet->DrawNPC((npcPos-start).X(), (npcPos-start).Y(), npci->second);
	}
}

void TilesetRenderer::DrawSpells() const {
	for (std::list<boost::shared_ptr<Spell> >::iterator spelli = Game::Inst()->spellList.begin(); spelli != Game::Inst()->spellList.end(); ++spelli) {
		Coordinate spellPos = (*spelli)->Position();
		Coordinate start(startTileX,startTileY), extent(tilesX,tilesY);
		if (spellPos.insideExtent(start, extent))
			tileSet->DrawSpell((spellPos-start).X(), (spellPos-start).Y(), *spelli);
	}
}

void TilesetRenderer::DrawFires() const {
	for (std::list<boost::weak_ptr<FireNode> >::iterator firei = Game::Inst()->fireList.begin(); firei != Game::Inst()->fireList.end(); ++firei) {
		if (boost::shared_ptr<FireNode> fire = firei->lock())
		{
			Coordinate firePos = fire->Position();
			Coordinate start(startTileX,startTileY), extent(tilesX,tilesY);
			if (firePos.insideExtent(start, extent))
				tileSet->DrawFire((firePos-start).X(), (firePos-start).Y(), fire);
		}
	}
}

void TilesetRenderer::DrawMarkers() const {
	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		Coordinate markerPos = markeri->second.Position();
		Coordinate start(startTileX,startTileY), extent(tilesX,tilesY);
		if (markerPos.insideExtent(start, extent))
			tileSet->DrawMarker((markerPos-start).X(), (markerPos-start).Y());
	}
}

void TilesetRenderer::DrawTerrain(int screenX, int screenY, Coordinate pos) const {
	TileType type(map->GetType(pos));
	const TerrainSprite& terrainSprite = (type == TILESNOW) ? tileSet->GetTerrainSprite(TILEGRASS) : tileSet->GetTerrainSprite(type);
	bool corrupted = map->GetCorruption(pos) >= 100;
	if (type == TILESNOW) {
		if (corrupted) {
			terrainSprite.DrawSnowedAndCorrupted(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&GrassConnectionTest, map, pos, _1), boost::bind(&SnowConnectionTest, map, pos, _1), boost::bind(&CorruptionConnectionTest, map, pos, _1));
		} else {
			terrainSprite.DrawSnowed(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&GrassConnectionTest, map, pos, _1), boost::bind(&SnowConnectionTest, map, pos, _1));
		}
	} else if (type == TILEGRASS) {
		bool burnt = type == TILEGRASS && map->Burnt(pos) >= 10;
		if (corrupted) {
			terrainSprite.DrawCorrupted(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&GrassConnectionTest, map, pos, _1), boost::bind(&CorruptionConnectionTest, map, pos, _1));
		} else if (burnt) {
			terrainSprite.DrawBurnt(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&GrassConnectionTest, map, pos, _1), boost::bind(&BurntConnectionTest, map, pos, _1));
		} else {
			terrainSprite.Draw(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&GrassConnectionTest, map, pos, _1));
		}
	} else {
		if (corrupted) {
			terrainSprite.DrawCorrupted(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&TerrainConnectionTest, map, pos, type, _1), boost::bind(&CorruptionConnectionTest, map, pos, _1));
		} else {
			terrainSprite.Draw(screenX, screenY, pos, permutationTable, map->heightMap->getValue(pos.X(), pos.Y()), boost::bind(&TerrainConnectionTest, map, pos, type, _1));
		}
	}
	
	// Water
	if (tileSet->IsIceSupported()) {
		boost::weak_ptr<WaterNode> waterPtr = map->GetWater(pos);
		if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
			if (water->Depth() > 0) {
				tileSet->DrawWater(screenX, screenY, boost::bind(&WaterConnectionTest, map, pos, _1));
			}
		}
		int natNum = -1;
		if ((natNum = map->GetNatureObject(pos)) >= 0) {
			if (Game::Inst()->natureList[natNum]->IsIce()) {
				tileSet->DrawIce(screenX, screenY, boost::bind(&WaterConnectionTest, map, pos, _1));
			}
		}
	} else {
		boost::weak_ptr<WaterNode> waterPtr = map->GetWater(pos);
		if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
			if (water->Depth() > 0) {
				tileSet->DrawWater(screenX, screenY, boost::bind(&WaterNoIceConnectionTest, map, pos, _1));
			}
		}
	}
	if (boost::shared_ptr<BloodNode> blood = map->GetBlood(pos).lock()) {
		if (blood->Depth() > 0) {
			tileSet->DrawBlood(screenX, screenY, boost::bind(&BloodConnectionTest, map, pos, _1));
		}
	}
	if (map->GroundMarked(pos)) {
		tileSet->DrawMarkedOverlay(screenX, screenY, boost::bind(&GroundMarkedConnectionTest, map, pos, _1));
	}
	
}

void TilesetRenderer::DrawFilth(int screenX, int screenY, Coordinate pos) const {
	if (boost::shared_ptr<FilthNode> filth = map->GetFilth(pos).lock()) {
		if (filth->Depth() > 4) {
			tileSet->DrawFilthMajor(screenX, screenY, boost::bind(&FilthConnectionTest, map, pos, _1));
		} else if (filth->Depth() > 0) {
			tileSet->DrawFilthMinor(screenX, screenY, boost::bind(&FilthConnectionTest, map, pos, _1));
		}
	}
}

void TilesetRenderer::DrawTerritoryOverlay(int screenX, int screenY, Coordinate pos) const {
	bool isOwned(map->IsTerritory(pos));
	tileSet->DrawTerritoryOverlay(screenX, screenY, isOwned, boost::bind(&TerritoryConnectionTest, map, pos, isOwned, _1));
}

bool TilesetRenderer::SetTileset(boost::shared_ptr<TileSet> newTileset) {
	tileSet = newTileset;
	return TilesetChanged();
}

int TilesetRenderer::GetScreenWidth() const {
	return screenWidth;
}

int TilesetRenderer::GetScreenHeight() const {
	return screenHeight;
}

TCODColor TilesetRenderer::GetKeyColor() const {
	return keyColor;
}

bool TilesetRenderer::TilesetChanged() {
	return true;
}

void TilesetRenderer::SetTranslucentUI(bool translucent) {
	translucentUI = translucent;
}

// Define these in their relevant cpps.
boost::shared_ptr<TilesetRenderer> CreateOGLTilesetRenderer(int width, int height, TCODConsole * console, std::string tilesetName);
boost::shared_ptr<TilesetRenderer> CreateSDLTilesetRenderer(int width, int height, TCODConsole * console, std::string tilesetName);

boost::shared_ptr<TilesetRenderer> CreateTilesetRenderer(int width, int height, TCODConsole * console, std::string tilesetName) {
    if (TCODSystem::getRenderer() == TCOD_RENDERER_SDL) {
	return CreateSDLTilesetRenderer(width, height, console, tilesetName);
    } /* else {
	 return CreateOGLTilesetRenderer(width, height, console, tilesetName);
	 } */  // FIXME
    // FIXME: what happens when neither of the above is available?
}
