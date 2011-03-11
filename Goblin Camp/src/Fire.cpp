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

#include <boost/algorithm/string.hpp>

#include "Fire.hpp"
#include "Random.hpp"
#include "Map.hpp"
#include "Game.hpp"
#include "Water.hpp"
#include "SpawningPool.hpp"
#include "Stockpile.hpp"

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

	if (temperature > 800) temperature = 800;

	boost::shared_ptr<WaterNode> water = Map::Inst()->GetWater(x, y).lock();
	if (water && water->Depth() > 0 && Map::Inst()->IsUnbridgedWater(x, y)) {
		temperature = 0;
		water->Depth(water->Depth()-1);
		boost::shared_ptr<Spell> steam = Game::Inst()->CreateSpell(Coordinate(x,y), Spell::StringToSpellType("steam"));

		Coordinate direction;
		Direction wind = Map::Inst()->GetWindDirection();
		if (wind == NORTH || wind == NORTHEAST || wind == NORTHWEST) direction.Y(Random::Generate(1, 7));
		if (wind == SOUTH || wind == SOUTHEAST || wind == SOUTHWEST) direction.Y(Random::Generate(-7, -1));
		if (wind == EAST || wind == NORTHEAST || wind == SOUTHEAST) direction.X(Random::Generate(-7, -1));
		if (wind == WEST || wind == SOUTHWEST || wind == NORTHWEST) direction.X(Random::Generate(1, 7));
		direction = direction + Coordinate(Random::Generate(-1, 1), Random::Generate(-1, 1));
		steam->CalculateFlightPath(Coordinate(x,y) + direction, 5, 1);
	} else if (temperature > 0) {
		if (Random::Generate(10) == 0) { 
			--temperature;
			Map::Inst()->Burn(x, y);
		}

		if (Map::Inst()->GetType(x, y) != TILEGRASS) --temperature;
		if (Map::Inst()->Burnt(x, y) >= 10) --temperature;

		int inverseSparkChance = 150 - std::max(0, ((temperature - 50) / 8));

		if (Random::Generate(inverseSparkChance) == 0) {
			boost::shared_ptr<Spell> spark = Game::Inst()->CreateSpell(Coordinate(x,y), Spell::StringToSpellType("spark"));
			int distance = Random::Generate(0, 15);
			if (distance < 12) {
				distance = 1;
			} else if (distance < 14) {
				distance = 2;
			} else {
				distance = 3;
			}

			Coordinate direction;
			Direction wind = Map::Inst()->GetWindDirection();
			if (wind == NORTH || wind == NORTHEAST || wind == NORTHWEST) direction.Y(distance);
			if (wind == SOUTH || wind == SOUTHEAST || wind == SOUTHWEST) direction.Y(-distance);
			if (wind == EAST || wind == NORTHEAST || wind == SOUTHEAST) direction.X(-distance);
			if (wind == WEST || wind == SOUTHWEST || wind == NORTHWEST) direction.X(distance);
			if (Random::Generate(9) < 8) direction = direction + Coordinate(Random::Generate(-1, 1), Random::Generate(-1, 1));
			else direction = direction + Coordinate(Random::Generate(-3, 3), Random::Generate(-3, 3));

			spark->CalculateFlightPath(Coordinate(x,y) + direction, 50, 1);
		}

		if (Random::Generate(60) == 0) {
			boost::shared_ptr<Spell> smoke = Game::Inst()->CreateSpell(Coordinate(x,y), Spell::StringToSpellType("smoke"));
			Coordinate direction;
			Direction wind = Map::Inst()->GetWindDirection();
			if (wind == NORTH || wind == NORTHEAST || wind == NORTHWEST) direction.Y(Random::Generate(25, 75));
			if (wind == SOUTH || wind == SOUTHEAST || wind == SOUTHWEST) direction.Y(Random::Generate(-75, -25));
			if (wind == EAST || wind == NORTHEAST || wind == SOUTHEAST) direction.X(Random::Generate(-75, -25));
			if (wind == WEST || wind == SOUTHWEST || wind == NORTHWEST) direction.X(Random::Generate(25, 75));
			direction = direction + Coordinate(Random::Generate(-3, 3), Random::Generate(-3, 3));
			smoke->CalculateFlightPath(Coordinate(x,y) + direction, 5, 1);
		}

		if (temperature > 1 && Random::Generate(9) < 4) {
			//Burn npcs on the ground
			for (std::set<int>::iterator npci = Map::Inst()->NPCList(x,y)->begin(); npci != Map::Inst()->NPCList(x,y)->end(); ++npci) {
				if (!Game::Inst()->GetNPC(*npci)->HasEffect(FLYING)) Game::Inst()->GetNPC(*npci)->AddEffect(BURNING);
			}

			//Burn items
			for (std::set<int>::iterator itemi = Map::Inst()->ItemList(x,y)->begin(); itemi != Map::Inst()->ItemList(x,y)->end(); ++itemi) {
				boost::shared_ptr<Item> item = Game::Inst()->GetItem(*itemi).lock();
				if (item && item->IsFlammable()) {
					Game::Inst()->CreateItem(item->Position(), Item::StringToItemType("ash"));
					Game::Inst()->RemoveItem(item);
					temperature += 250;
					break;
				}
			}

			//Burn constructions
			int cons = Map::Inst()->GetConstruction(x,y);
			if (cons >= 0) {
				boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(cons).lock();
				if (construct) {
					if (construct->IsFlammable()) {
						if (Random::Generate(29) == 0) {
							Attack fire;
							TCOD_dice_t dice;
							dice.addsub = 1;
							dice.multiplier = 1;
							dice.nb_dices = 1;
							dice.nb_faces = 1;
							fire.Amount(dice);
							fire.Type(DAMAGE_FIRE);
							construct->Damage(&fire);
						}
						if (temperature < 15) temperature += 5;
					} else if (construct->HasTag(STOCKPILE) || construct->HasTag(FARMPLOT)) {
						/*Stockpiles are a special case. Not being an actual building, fire won't touch them.
						Instead fire should be able to burn the items stored in the stockpile*/
						boost::shared_ptr<Container> container = boost::static_pointer_cast<Stockpile>(construct)->Storage(Coordinate(x,y)).lock();
						if (container) {
							boost::shared_ptr<Item> item = container->GetFirstItem().lock();
							if (item && item->IsFlammable()) {
								container->RemoveItem(item);
								item->PutInContainer();
								Game::Inst()->CreateItem(item->Position(), Item::StringToItemType("ash"));
								Game::Inst()->RemoveItem(item);
								temperature += 250;
							}
						}
					} else if (construct->HasTag(SPAWNINGPOOL)) {
						boost::static_pointer_cast<SpawningPool>(construct)->Burn();
						if (temperature < 15) temperature += 5;
					}
				}
			}

			//Burn plantlife
			int natureObject = Map::Inst()->GetNatureObject(x, y);
			if (natureObject >= 0 && 
				!boost::iequals(Game::Inst()->natureList[natureObject]->Name(), "Scorched tree")) {
					bool tree = Game::Inst()->natureList[natureObject]->Tree();
					Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[natureObject]);
					if (tree && Random::Generate(4) == 0) {
						Game::Inst()->CreateNatureObject(Coordinate(x,y), "Scorched tree");
					}
					temperature += tree ? 500 : 100;
			}

		}
	}
}

void FireNode::save(OutputArchive& ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & temperature;
}

void FireNode::load(InputArchive& ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & temperature;
}
