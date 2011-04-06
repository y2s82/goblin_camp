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
#include "tileRenderer/NPCSprite.hpp"

NPCSprite::NPCSprite() 
: sprites(),
  weaponOverlays(),
  equipmentAware(false),
  paperdoll(false),
  weaponTypeNames(),
  armourTypeNames()
{
	sprites.push_back(Sprite_ptr());
}

NPCSprite::NPCSprite(Sprite_ptr sprite)
: sprites(),
  weaponOverlays(),
  equipmentAware(false),
  paperdoll(false),
  weaponTypeNames(),
  armourTypeNames()
{
	sprites.push_back(sprite);
}

NPCSprite::NPCSprite(const std::vector<Sprite_ptr>& s, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes)
: sprites(s),
  weaponOverlays(),
  equipmentAware(true),
  paperdoll(false),
  weaponTypeNames(weaponTypes),
  armourTypeNames(armourTypes)
{
	if (sprites.empty()) {
		sprites.push_back(Sprite_ptr());
	}
	if ((weaponTypeNames.size() + 1) * (armourTypeNames.size() + 1) != sprites.size()) {
		equipmentAware = false;
	}
}

NPCSprite::NPCSprite(const std::vector<Sprite_ptr>& s, const std::vector<Sprite_ptr>& weaps, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes)
: sprites(s),
  weaponOverlays(weaps),
  equipmentAware(false),
  paperdoll(true),
  weaponTypeNames(weaponTypes),
  armourTypeNames(armourTypes)
{
	if (sprites.empty()) {
		sprites.push_back(Sprite_ptr());
	}
	if (sprites.size() != armourTypeNames.size() + 1 || weaponOverlays.size() != weaponTypeNames.size()) {
		paperdoll = false;
	}
}

NPCSprite::~NPCSprite() {}

bool NPCSprite::IsEquipmentAware() const {
	return equipmentAware;
}

bool NPCSprite::Exists() const {
	return sprites.at(0).Exists();
}

void NPCSprite::Draw(int screenX, int screenY) const {
	sprites.at(0).Draw(screenX, screenY);
}

namespace {
	int findIndex(const ItemPreset& itemPreset, const std::vector<std::string>& vector) {
		std::vector<std::string>::const_iterator namePos = std::find(vector.begin(), vector.end(), itemPreset.name);
		if (namePos != vector.end()) {
			return namePos - vector.begin();
		} else {
			for (std::set<ItemCategory>::const_iterator cati = itemPreset.specificCategories.begin(); cati != itemPreset.specificCategories.end(); ++cati) {
				ItemCategory catId = *cati;
				while (catId != -1) {
					ItemCat& cat = Item::Categories.at(catId);
					std::vector<std::string>::const_iterator catPos = std::find(vector.begin(), vector.end(), cat.GetName());
					if (catPos != vector.end())
					{
						return catPos - vector.begin();
					}
				}
			}
		}
		return -1;
	}
}

void NPCSprite::Draw(int screenX, int screenY, boost::shared_ptr<NPC> npc) const {
	if (equipmentAware || paperdoll) {
		int weaponIndex = -1;
		int armourIndex = -1;
		if (boost::shared_ptr<Item> weapon = npc->Wielding().lock()) {
			const ItemPreset& itemPreset = Item::Presets[weapon->Type()];
			weaponIndex = findIndex(itemPreset, weaponTypeNames);
		}
		if (boost::shared_ptr<Item> armour = npc->Wearing().lock()) {
			const ItemPreset& itemPreset = Item::Presets[armour->Type()];
			armourIndex = findIndex(itemPreset, armourTypeNames);
		}
		if (equipmentAware) {
			sprites.at((weaponIndex + 1) + ((armourIndex + 1) * (weaponTypeNames.size() + 1))).Draw(screenX, screenY);
		} else {
			sprites.at(armourIndex + 1).Draw(screenX, screenY);
			if (weaponIndex != -1) {
				weaponOverlays.at(weaponIndex).Draw(screenX, screenY);
			}
		}
	} else {
		sprites.at(0).Draw(screenX, screenY);
	}
}
