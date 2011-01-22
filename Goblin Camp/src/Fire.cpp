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

#include "Fire.hpp"
#include "Random.hpp"
#include "Map.hpp"
#include "Game.hpp"
#include "Water.hpp"

FireNode::FireNode(int vx, int vy, int vtemp) : x(vx), y(vy),
	temperature(vtemp) {
		color.r = Random::Generate(225, 255);
		color.g = Random::Generate(0, 250);
		color.b = 0;
		graphic = Random::Generate(176,178);
}

FireNode::~FireNode() {}

Coordinate FireNode::GetPosition() { return Coordinate(x,y); }

void FireNode::AddHeat(int value) { temperature += value; }

int FireNode::GetHeat() { return temperature; }

void FireNode::SetHeat(int value) { temperature = value; }

void FireNode::Draw(Coordinate upleft, TCODConsole* console) {
	int screenX = x - upleft.X();
	int screenY = y - upleft.Y();

	if (screenX >= 0 && screenX < console->getWidth() &&
		screenY >= 0 && screenY < console->getHeight()) {
			console->putCharEx(screenX, screenY, graphic, color, TCODColor::black);
	}
}

void FireNode::Update() {
	graphic = Random::Generate(176,178);
	color.r = Random::Generate(225, 255);
	color.g = Random::Generate(0, 250);

	if (boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(x, y).lock()) {
		if (water->Depth() > 0) {
			if (water->Depth() >= temperature) {
				water->Depth(water->Depth() - temperature);
				temperature = 0;
			} else {
				temperature -= water->Depth();
				water->Depth(0);
			}
			Game::Inst()->CreateSpell(Coordinate(x,y), 2);
		}
	} else {
		if (Random::Generate(5) == 0) { 
			--temperature;
			Map::Inst()->Burn(x, y);
		}

		if (Map::Inst()->Type(x, y) != TILEGRASS || Map::Inst()->Burnt(x, y) >= 10) temperature -= 3;

		if (Random::Generate(20) == 0) {
			Game::Inst()->CreateSpell(Coordinate(x,y), 0);
		}

		if (Random::Generate(60) == 0) {
			Game::Inst()->CreateSpell(Coordinate(x,y), 1);
		}
	}
}