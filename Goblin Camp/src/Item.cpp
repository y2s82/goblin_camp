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

#include <libtcod.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#ifdef DEBUG
#include <iostream>
#include <boost/format.hpp>
#endif

#include "Random.hpp"
#include "Item.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"
#include "StockManager.hpp"
#include "Attack.hpp"

std::vector<ItemPreset> Item::Presets = std::vector<ItemPreset>();
std::vector<ItemCat> Item::Categories = std::vector<ItemCat>();
std::map<std::string, ItemType> Item::itemTypeNames = std::map<std::string, ItemType>();
std::map<std::string, ItemType> Item::itemCategoryNames = std::map<std::string, ItemType>();

Item::Item(Coordinate pos, ItemType typeval, int owner, std::vector<boost::weak_ptr<Item> > components) : Entity(),
	type(typeval),
	flammable(false),
	attemptedStore(false),
	decayCounter(-1),
	container(boost::weak_ptr<Item>()),
	internal(false)
{
	SetFaction(owner);
	//Remember that the components are destroyed after this constructor!
	x = pos.X();
	y = pos.Y();

	name = Item::Presets[type].name;
	categories = Item::Presets[type].categories;
	graphic = Item::Presets[type].graphic;
	color = Item::Presets[type].color;
	if (Item::Presets[type].decays) decayCounter = Item::Presets[type].decaySpeed;

	for (int i = 0; i < (signed int)components.size(); ++i) {
		if (components[i].lock()) 
			color = TCODColor::lerp(color, components[i].lock()->Color(), 0.35f);
	}

	attack = Item::Presets[type].attack;

	for (int i = 0; i < RES_COUNT; ++i) {
		resistances[i] = Item::Presets[type].resistances[i];
	}
}

Item::~Item() {
#ifdef DEBUG
	std::cout<<name<<"("<<uid<<") destroyed\n";
#endif
	if (faction == 0) {
		StockManager::Inst()->UpdateQuantity(type, -1);
	}
}

void Item::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
	if (!container.lock() && screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, Map::Inst()->BackColor(x,y));
	}
}

ItemType Item::Type() {return type;}
bool Item::IsCategory(ItemCategory category) { return (categories.find(category) != categories.end());}
TCODColor Item::Color() {return color;}
void Item::Color(TCODColor col) {color = col;}

void Item::Position(Coordinate pos) {
	if (!internal) Map::Inst()->ItemList(x,y)->erase(uid);
	x = pos.X(); y = pos.Y();
	if (!internal) Map::Inst()->ItemList(x,y)->insert(uid);
}
Coordinate Item::Position() {
	if (container.lock()) return container.lock()->Position();
	return this->Entity::Position();
}

void Item::Reserve(bool value) {
	reserved = value;
	if (!reserved && !container.lock() && !attemptedStore) {
		Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
		attemptedStore = true;
	}
}

void Item::PutInContainer(boost::weak_ptr<Item> con) {
	container = con;
	attemptedStore = false;

	if (container.lock()) Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), true);
	else Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), false);

	if (!container.lock() && !reserved) {
		Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
		attemptedStore = true;
	}
}
boost::weak_ptr<Item> Item::ContainedIn() {return container;}

int Item::Graphic() {return graphic;}

Attack Item::GetAttack() const {return attack;}

std::string Item::ItemTypeToString(ItemType type) {
	return Item::Presets[type].name;
}

ItemType Item::StringToItemType(std::string str) {
	boost::to_upper(str);
	return itemTypeNames[str];
}

std::string Item::ItemCategoryToString(ItemCategory category) {
	return category == -1 ? "None" : Item::Categories[category].name;
}

ItemCategory Item::StringToItemCategory(std::string str) {
	boost::to_upper(str);
	return Item::itemCategoryNames[str];
}

std::vector<ItemCategory> Item::Components(ItemType type) {
	return Item::Presets[type].components;
}

ItemCategory Item::Components(ItemType type, int index) {
	return Item::Presets[type].components[index];
}

void Item::SetFaction(int val) {
	if (val == 0 && faction != 0) { //Transferred to player
		StockManager::Inst()->UpdateQuantity(type, 1);
	} else if (val != 0 && faction == 0) { //Transferred from player
		StockManager::Inst()->UpdateQuantity(type, -1);
	}
	faction = val;
}

int Item::GetFaction() const { return faction; }

int Item::RelativeValue() {
	TCOD_dice_t amount = attack.Amount();
	int minDamage = (int)(amount.nb_dices + amount.addsub);
	int maxDamage = (int)((amount.nb_dices * amount.nb_faces) + amount.addsub);
	return (minDamage + maxDamage) / 2;
}

int Item::Resistance(int i) const { return resistances[i]; }

void Item::SetVelocity(int speed) {
	velocity = speed;
	if (speed > 0) {
		Game::Inst()->flyingItems.insert(boost::static_pointer_cast<Item>(shared_from_this()));
	} else {
		Game::Inst()->stoppedItems.push_back(boost::static_pointer_cast<Item>(shared_from_this()));
		if (!Map::Inst()->Walkable(x, y)) {
			for (int radius = 1; radius < 10; ++radius) {
				for (int xi = x - radius; xi <= x + radius; ++xi) {
					for (int yi = y - radius; yi <= y + radius; ++yi) {
						if (Map::Inst()->Walkable(xi, yi)) {
							Position(Coordinate(xi, yi));
							return;
						}
					}
				}
			}
		}
	}
}

void Item::UpdateVelocity() {
	if (velocity > 0) {
		nextVelocityMove += velocity;
		while (nextVelocityMove > 100) {
			nextVelocityMove -= 100;
			if (flightPath.size() > 0) {
				if (flightPath.back().height < ENTITYHEIGHT) { //We're flying low enough to hit things
					int tx = flightPath.back().coord.X();
					int ty = flightPath.back().coord.Y();
					if (Map::Inst()->BlocksWater(tx,ty)) { //We've hit an obstacle
						Attack attack = GetAttack();
						if (Map::Inst()->GetConstruction(tx,ty) > -1) {
							if (boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(Map::Inst()->GetConstruction(tx,ty)).lock()) {
								construct->Damage(&attack);
							}
						}
						SetVelocity(0);
						flightPath.clear();
						return;
					}
					if (Map::Inst()->NPCList(tx,ty)->size() > 0) {
						if (Random::Generate(0, std::max(1, flightPath.back().height) - 1) < (signed int)(2 + Map::Inst()->NPCList(tx,ty)->size())) {

							Attack attack = GetAttack();
							boost::shared_ptr<NPC> npc = Game::Inst()->npcList[*Map::Inst()->NPCList(tx,ty)->begin()];
							npc->Damage(&attack);

							Position(flightPath.back().coord);
							SetVelocity(0);
							flightPath.clear();
							return;
						}
					}
				}
				Position(flightPath.back().coord);
				if (flightPath.back().height == 0) {
					SetVelocity(0);
				}
				flightPath.pop_back();
			} else SetVelocity(0);
		}
	} 
}

void Item::SetInternal() { internal = true; }

ItemCat::ItemCat() : flammable(false),
	name("Category schmategory"),
	parent(0)
{}

std::string ItemCat::GetName() {
	return name;
}

ItemPreset::ItemPreset() : graphic('?'),
	color(TCODColor::pink),
	name("Preset default"),
	categories(std::set<ItemCategory>()),
	nutrition(-1),
	growth(-1),
	fruits(std::list<ItemType>()),
	organic(false),
	container(0),
	multiplier(1),
	fitsin(-1),
	containIn(-1),
	decays(false),
	decaySpeed(0),
	decayList(std::vector<ItemType>()),
	attack(Attack())
{
	for (int i = 0; i < RES_COUNT; ++i) {
		resistances[i] = 0;
	}
}

OrganicItem::OrganicItem(Coordinate pos, ItemType typeVal) : Item(pos, typeVal),
	nutrition(-1),
	growth(-1)
{}

int OrganicItem::Nutrition() { return nutrition; }
void OrganicItem::Nutrition(int val) { nutrition = val; }

ItemType OrganicItem::Growth() { return growth; }
void OrganicItem::Growth(ItemType val) { growth = val; }
