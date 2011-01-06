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

TCODMapRenderer::TCODMapRenderer()
{
}

TCODMapRenderer::~TCODMapRenderer() {}

//TODO: Optimize. This causes the biggest performance hit by far right now 
void TCODMapRenderer::DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int posX, int posY, int sizeX, int sizeY)
{
	int screenDeltaX = upleft.X();
	int screenDeltaY = upleft.Y();
	TCODConsole * minimap = new TCODConsole(sizeX, sizeY);
	for (int y = upleft.Y(); y < upleft.Y() + minimap->getHeight(); ++y) {
		for (int x = upleft.X(); x < upleft.X() + minimap->getWidth(); ++x) {
			if (x >= 0 && x < map->Width() && y >= 0 && y < map->Height()) {

				minimap->putCharEx(x-screenDeltaX,y-(screenDeltaY), map->Graphic(x,y), map->ForeColor(x,y), map->BackColor(x,y));

				boost::weak_ptr<WaterNode> water = map->GetWater(x,y);
				if (water.lock()) {
					minimap->putCharEx(x-screenDeltaX, y-screenDeltaY, water.lock()->GetGraphic(), water.lock()->GetColor(), TCODColor::black);
				}
				boost::weak_ptr<FilthNode> filth = map->GetFilth(x,y);
				if (filth.lock() && filth.lock()->Depth() > 0) {
					minimap->putCharEx(x-screenDeltaX, y-screenDeltaY, filth.lock()->GetGraphic(), filth.lock()->GetColor(), TCODColor::black);
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					minimap->setCharBackground(x-screenDeltaX,y-screenDeltaY, map->IsTerritory(x,y) ? TCODColor::darkGreen : TCODColor::darkRed);
				}
			}
			else {
				minimap->putCharEx(x-screenDeltaX,y-screenDeltaY, TCOD_CHAR_BLOCK3, TCODColor::black, TCODColor::white);
			}
		}
	}

	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		markeri->second.Draw(upleft, minimap);
	}

	minimap->flush();
	TCODConsole::blit(minimap, 0, 0, sizeX, sizeY, console, posX, posY);
}