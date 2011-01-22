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

#include "Spell.hpp"
#include "Game.hpp"
#include "Random.hpp"

Spell::Spell(Coordinate pos, int vtype) : Entity(),
	type(vtype), dead(false), immaterial(false)
{
	x = pos.X();
	y = pos.Y();

	switch (type) {
	case 0:
		graphic = 250;
		color = TCODColor::orange;
		attack.Type(DAMAGE_FIRE);
		TCOD_dice_t dice;
		dice.addsub = 2;
		dice.multiplier = 1;
		dice.nb_dices = 1;
		dice.nb_faces = 3;
		attack.Amount(dice);
		name = "Spark";
		break;

	case 1: //Smoke
		graphic = 177;
		color = TCODColor::darkGrey;
		immaterial = true;
		name = "Smoke";
		break;

	case 2:
		graphic = 176;
		color = TCODColor::lightGrey;
		immaterial = true;
		name = "Steam";
		break;
	}
}

Spell::~Spell() {}

void Spell::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, Map::Inst()->GetBackColor(x,y));
	}
}

void Spell::Impact(int speedChange) {
	SetVelocity(0);
	flightPath.clear();

	if (type == 0) Game::Inst()->CreateFire(Position());
	dead = true;
}

void Spell::UpdateVelocity() {
	if (velocity > 0) {
		nextVelocityMove += velocity;
		while (nextVelocityMove > 100) {
			nextVelocityMove -= 100;
			if (flightPath.size() > 0) {

				if (flightPath.back().height < ENTITYHEIGHT) { //We're flying low enough to hit things
					int tx = flightPath.back().coord.X();
					int ty = flightPath.back().coord.Y();

					if (!immaterial) {
						if (Map::Inst()->BlocksWater(tx,ty) || !Map::Inst()->IsWalkable(tx,ty)) { //We've hit an obstacle
							if (Map::Inst()->GetConstruction(tx,ty) > -1) {
								if (boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(tx,ty)).lock()) {
									construct->Damage(&attack);
								}
							}
							if (attack.Type() == DAMAGE_FIRE) {
								/*The item's attack was a fire attack, so theres a chance it'll create fire on the 
								obstacle it hit */
								Game::Inst()->CreateFire(flightPath.back().coord, 15);
							}
							Impact(velocity);
							return;
						}
						if (Map::Inst()->NPCList(tx,ty)->size() > 0) { //Hit a creature
							if (Random::Generate(std::max(1, flightPath.back().height) - 1) < (signed int)(2 + Map::Inst()->NPCList(tx,ty)->size())) {

								boost::shared_ptr<NPC> npc = Game::Inst()->npcList[*Map::Inst()->NPCList(tx,ty)->begin()];
								npc->Damage(&attack);

								Position(flightPath.back().coord);
								Impact(velocity);
								return;
							}
						}
					}
				}

				Position(flightPath.back().coord);

				if (flightPath.back().height <= 0) { //Hit the ground early
					Impact(velocity);
					return;
				}

				flightPath.pop_back();
			} else { Impact(velocity); return; } //No more flightpath
		}
	} 
}

bool Spell::IsDead() {return dead;}