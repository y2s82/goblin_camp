/* Copyright 2010-2011 Ilkka Halila
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

#include <iostream>

#include "TCODMapRenderer.hpp"
#include <libtcod.hpp>
#include "MapMarker.hpp"
#include "Game.hpp"
#include "MathEx.hpp"

TCODMapRenderer::TCODMapRenderer(TCODConsole * mapConsole) :
	console(mapConsole),
	cursorChar('X'),
	upleft(0,0)
{
}

TCODMapRenderer::~TCODMapRenderer() {}

namespace {
	template <typename MapT>
	inline void InternalDrawMapItems(const char *name, MapT& map, Coordinate& upleft, TCODConsole *buffer) {
		for (typename MapT::iterator it = map.begin(); it != map.end(); ) {
			typename MapT::mapped_type ptr = it->second;

			if (ptr.get() != NULL) {
				ptr->Draw(upleft, buffer);
				++it;
			} else {
#ifdef DEBUG
				std::cout << "!!! NULL POINTER !!! " << name << " ; id " << it->first << std::endl;
#endif
				typename MapT::iterator tmp = it;
				++it;
				map.erase(tmp);
			}
		}
	}
}

void TCODMapRenderer::PreparePrefabs() {}

//TODO: Optimize. This causes the biggest performance hit by far right now 
void TCODMapRenderer::DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH)
{
	int charX, charY;
	TCODSystem::getCharSize(&charX, &charY);
	if (viewportW == -1) { 
		viewportW = console->getWidth();
	} else {
		viewportW /= charX;
	}
	if (viewportH == -1) {
		viewportH = console->getHeight();
	} else {
		viewportH /= charY;
	}
	viewportX /= charX;
	viewportY /= charY;

	upleft = Coordinate(FloorToInt::convert(focusX) - (viewportW / 2), FloorToInt::convert(focusY) - (viewportH / 2));

	int screenDeltaX = upleft.X();
	int screenDeltaY = upleft.Y();
	TCODConsole minimap(viewportW, viewportH);
	for (int y = upleft.Y(); y < upleft.Y() + minimap.getHeight(); ++y) {
		for (int x = upleft.X(); x < upleft.X() + minimap.getWidth(); ++x) {
			Coordinate xy(x,y);
			if (map->IsInside(xy)) {
				minimap.putCharEx(x-screenDeltaX,y-(screenDeltaY), map->GetGraphic(xy), map->GetForeColor(xy), map->GetBackColor(xy));

				if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
					boost::weak_ptr<WaterNode> wwater = map->GetWater(xy);
					if (boost::shared_ptr<WaterNode> water = wwater.lock()) {
						if (water->Depth() > 0)
							minimap.putCharEx(x-screenDeltaX, y-screenDeltaY, water->GetGraphic(), water->GetColor(), TCODColor::black);
					}
					boost::weak_ptr<FilthNode> wfilth = map->GetFilth(xy);
					if (boost::shared_ptr<FilthNode> filth = wfilth.lock()) {
						if (filth->Depth() > 0)
							minimap.putCharEx(x-screenDeltaX, y-screenDeltaY, filth->GetGraphic(), filth->GetColor(), TCODColor::black);
					}
					int natNum = map->GetNatureObject(xy);
					if (natNum >= 0) {
						Game::Inst()->natureList[natNum]->Draw(upleft,&minimap);
					}
				}
				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					minimap.setCharBackground(x-screenDeltaX,y-screenDeltaY, map->IsTerritory(xy) ? TCODColor(45,85,0) : TCODColor(80,0,0));
				}
			}
			else {
				minimap.putCharEx(x-screenDeltaX,y-screenDeltaY, TCOD_CHAR_BLOCK3, TCODColor::black, TCODColor::white);
			}
		}
	}

	if (!(map->GetOverlayFlags() & TERRAIN_OVERLAY)) {
		InternalDrawMapItems("static constructions",  Game::Inst()->staticConstructionList, upleft, &minimap);
		InternalDrawMapItems("dynamic constructions", Game::Inst()->dynamicConstructionList, upleft, &minimap);
		//TODO: Make this consistent
		for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end();) {
			if (!itemi->second) {
				std::map<int,boost::shared_ptr<Item> >::iterator tmp = itemi;
				++itemi;
				Game::Inst()->itemList.erase(tmp);
				continue;
			} else if (!itemi->second->ContainedIn().lock()) {
				itemi->second->Draw(upleft, &minimap);
			}
			++itemi;
		}
	}

	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= upleft.X() && markerX < upleft.X() + viewportW
			&& markerY >= upleft.Y() && markerY < upleft.Y() + viewportH) {
				minimap.putCharEx(markerX - upleft.X(), markerY - upleft.Y(), markeri->second.Graphic(), markeri->second.Color(), TCODColor::black);
		}
	}


	InternalDrawMapItems("NPCs",                  Game::Inst()->npcList, upleft, &minimap);
	for (std::list<boost::weak_ptr<FireNode> >::iterator firei = Game::Inst()->fireList.begin(); firei != Game::Inst()->fireList.end(); ++firei) {
		if (firei->lock()) firei->lock()->Draw(upleft, &minimap);
	}
	for (std::list<boost::shared_ptr<Spell> >::iterator spelli = Game::Inst()->spellList.begin(); spelli != Game::Inst()->spellList.end(); ++spelli) {
		(*spelli)->Draw(upleft, &minimap);
	}

	TCODConsole::blit(&minimap, 0, 0, viewportW, viewportH, console, viewportX, viewportY);
}

Coordinate TCODMapRenderer::TileAt(int x, int y, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const {
	// Convert viewport to tile space
	int charX, charY;
	TCODSystem::getCharSize(&charX, &charY);
	if (viewportW == -1) { 
		viewportW = console->getWidth();
	} else {
		viewportW /= charX;
	}
	if (viewportH == -1) {
		viewportH = console->getHeight();
	} else {
		viewportH /= charY;
	}
	viewportX /= charX;
	viewportY /= charY;

	return Coordinate(FloorToInt::convert(focusX) - (viewportW / 2) + (x - viewportX) / charX, FloorToInt::convert(focusY) - (viewportH / 2) + (y - viewportY) / charY);
}

float TCODMapRenderer::ScrollRate() const {
	return 1.0f;
}

void TCODMapRenderer::SetCursorMode(CursorType mode) {
	switch (mode){
	case Cursor_Construct:
		cursorChar = 'C'; break;
	case Cursor_Stockpile:
		cursorChar = '='; break;
	case Cursor_TreeFelling:
	case Cursor_Order:
		cursorChar = 'X'; break;
	case Cursor_Harvest:
		cursorChar = 'H'; break;
	case Cursor_Tree:
		cursorChar = 'T'; break;
	case Cursor_Dismantle:
		cursorChar = 'D'; break;
	case Cursor_Undesignate:
		cursorChar = 'U'; break;
	case Cursor_Bog:
		cursorChar = 'B'; break;
	case Cursor_Dig:
		cursorChar = '_'; break;
	case Cursor_AddTerritory:
		cursorChar = '+'; break;
	case Cursor_RemoveTerritory:
		cursorChar = '-'; break;
	case Cursor_Gather:
		cursorChar = 'G'; break;
	default:
		cursorChar = 'X'; break;
	}
}

void TCODMapRenderer::SetCursorMode(const NPCPreset& preset) {
	cursorChar = preset.graphic;
}

void TCODMapRenderer::SetCursorMode(const ItemPreset& preset) {
	cursorChar = preset.graphic;
}

void TCODMapRenderer::SetCursorMode(int other) {
	cursorChar = other;
}

void TCODMapRenderer::DrawCursor(const Coordinate& start, const Coordinate& end, bool placeable) {
	for (int x = std::max(0, start.X() - upleft.X()); x <= std::min(console->getWidth() - 1, end.X() - upleft.X()); ++x)
	{
		for (int y = std::max(0, start.Y() - upleft.Y()); y <= std::min(console->getHeight() - 1, end.Y() - upleft.Y()); ++y)
		{
			if (!placeable) console->putCharEx(x, y, cursorChar, TCODColor::red, TCODColor::black);
			else console->putCharEx(x, y, cursorChar, TCODColor::green, TCODColor::black);
		}
	}
}

void TCODMapRenderer::DrawCursor(const Coordinate& pos, bool placeable) {
	if (pos.X() - upleft.X() >= 0 && pos.X() - upleft.X() < console->getWidth()
		&& pos.Y() - upleft.Y() >= 0 && pos.Y() - upleft.Y() < console->getHeight())
	{
		if (!placeable) console->putCharEx(pos.X() - upleft.X(),pos.Y() - upleft.Y(), cursorChar, TCODColor::red, TCODColor::black);
		else console->putCharEx(pos.X() - upleft.X(), pos.Y() - upleft.Y(), cursorChar, TCODColor::green, TCODColor::black);
	}
}

void TCODMapRenderer::SetTranslucentUI(bool) {}
