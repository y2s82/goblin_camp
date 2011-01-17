/* Copyright 2010 Ilkka Halila
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

#include "TCODMapRenderer.hpp"
#include <libtcod.hpp>
#include "MapMarker.hpp"
#include "Game.hpp"

TCODMapRenderer::TCODMapRenderer()
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
void TCODMapRenderer::DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int posX, int posY, int sizeX, int sizeY)
{
	if (sizeX == -1) sizeX = console->getWidth();
	if (sizeY == -1) sizeY = console->getHeight();

	int screenDeltaX = upleft.X();
	int screenDeltaY = upleft.Y();
	TCODConsole minimap(sizeX, sizeY);
	for (int y = upleft.Y(); y < upleft.Y() + minimap.getHeight(); ++y) {
		for (int x = upleft.X(); x < upleft.X() + minimap.getWidth(); ++x) {
			if (x >= 0 && x < map->Width() && y >= 0 && y < map->Height()) {

				minimap.putCharEx(x-screenDeltaX,y-(screenDeltaY), map->Graphic(x,y), map->ForeColor(x,y), map->BackColor(x,y));

				boost::weak_ptr<WaterNode> water = map->GetWater(x,y);
				if (water.lock()) {
					minimap.putCharEx(x-screenDeltaX, y-screenDeltaY, water.lock()->GetGraphic(), water.lock()->GetColor(), TCODColor::black);
				}
				boost::weak_ptr<FilthNode> filth = map->GetFilth(x,y);
				if (filth.lock() && filth.lock()->Depth() > 0) {
					minimap.putCharEx(x-screenDeltaX, y-screenDeltaY, filth.lock()->GetGraphic(), filth.lock()->GetColor(), TCODColor::black);
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					minimap.setCharBackground(x-screenDeltaX,y-screenDeltaY, map->IsTerritory(x,y) ? TCODColor::darkGreen : TCODColor::darkRed);
				}
			}
			else {
				minimap.putCharEx(x-screenDeltaX,y-screenDeltaY, TCOD_CHAR_BLOCK3, TCODColor::black, TCODColor::white);
			}
		}
	}

	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= upleft.X() && markerY < upleft.X() + sizeX
			&& markerY >= upleft.Y() && markerY < upleft.Y() + sizeY) {
				minimap.putCharEx(markerX - upleft.X(), markerY - upleft.Y(), markeri->second.Graphic(), markeri->second.Color(), TCODColor::black);
		}
	}

	InternalDrawMapItems("static constructions",  Game::Inst()->staticConstructionList, upleft, &minimap);
	InternalDrawMapItems("dynamic constructions", Game::Inst()->dynamicConstructionList, upleft, &minimap);
		//TODO: Make this consistent
	for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end(); ++itemi) {
		if (!itemi->second->ContainedIn().lock()) itemi->second->Draw(upleft, &minimap);
	}
	InternalDrawMapItems("nature objects",        Game::Inst()->natureList, upleft, &minimap);
	InternalDrawMapItems("NPCs",                  Game::Inst()->npcList, upleft, &minimap);

	TCODConsole::blit(&minimap, 0, 0, sizeX, sizeY, console, posX, posY);
}