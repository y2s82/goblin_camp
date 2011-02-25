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
  equipmentAware(false),
  weaponTypeNames(),
  armourTypeNames()
{
	sprites.push_back(Sprite());
}

NPCSprite::NPCSprite(const Sprite& sprite)
: sprites(),
  equipmentAware(false),
  weaponTypeNames(),
  armourTypeNames()
{
	sprites.push_back(sprite);
}

NPCSprite::NPCSprite(const std::vector<Sprite>& s, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes)
: sprites(s),
  equipmentAware(true),
  weaponTypeNames(weaponTypes),
  armourTypeNames(armourTypes)
{
	if (sprites.empty()) {
		sprites.push_back(Sprite());
	}
	if ((weaponTypeNames.size() + 1) * (armourTypeNames.size() + 1) != sprites.size()) {
		equipmentAware = false;
	}
}

NPCSprite::~NPCSprite() {}

bool NPCSprite::IsEquipmentAware() const {
	return equipmentAware;
}

bool NPCSprite::Exists() const {
	return sprites.at(0).Exists();
}

void NPCSprite::Draw(SDL_Surface * dst, SDL_Rect * dstRect) const {
	sprites.at(0).Draw(dst, dstRect);
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

void NPCSprite::Draw(boost::shared_ptr<NPC> npc, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (equipmentAware) {
		int weaponIndex = 0;
		int armourIndex = 0;
		if (boost::shared_ptr<Item> weapon = npc->Wielding().lock()) {
			const ItemPreset& itemPreset = Item::Presets[weapon->Type()];
			weaponIndex = findIndex(itemPreset, weaponTypeNames) + 1;
		}
		if (boost::shared_ptr<Item> armour = npc->Wearing().lock()) {
			const ItemPreset& itemPreset = Item::Presets[armour->Type()];
			armourIndex = findIndex(itemPreset, armourTypeNames) + 1;
		}
		sprites.at(weaponIndex + (armourIndex * (weaponTypeNames.size() + 1))).Draw(dst, dstRect);
	} else {
		sprites.at(0).Draw(dst, dstRect);
	}
}

NPCSpriteFactory::NPCSpriteFactory()
: frames(),
  armourTypes(),
  weaponTypes(),
  frameRate(15),
  equipmentMap(false)
{
}

NPCSpriteFactory::~NPCSpriteFactory() {}

void NPCSpriteFactory::Reset() {
	frames.clear();
	armourTypes.clear();
	weaponTypes.clear();
	frameRate = 15;
	equipmentMap = false;
}

NPCSprite NPCSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	if (equipmentMap) {
		if (frames.size() == (weaponTypes.size() + 1) * (armourTypes.size() + 1)) {
			std::vector<Sprite> sprites;
			for (std::vector<int>::iterator iter = frames.begin(); iter != frames.end(); ++iter)
				sprites.push_back(Sprite(currentTexture, *iter));

			return NPCSprite(sprites, weaponTypes, armourTypes);
		} else if (frames.size() > 0) {
			return NPCSprite(Sprite(currentTexture, frames[0]));
		} else {
			return NPCSprite();
		}
	} else {
		return NPCSprite(Sprite(currentTexture, frames.begin(), frames.end(), false, frameRate));
	}
}

void NPCSpriteFactory::AddSpriteFrame(int frame) {
	frames.push_back(frame);
}

void NPCSpriteFactory::SetFPS(int fps) {
	frameRate = fps;
}

void NPCSpriteFactory::SetEquipmentMap(bool equipMap) {
	equipmentMap = equipMap;
}

void NPCSpriteFactory::AddArmourType(std::string armourType) {
	armourTypes.push_back(armourType);
}

void NPCSpriteFactory::AddWeaponType(std::string weaponType) {
	weaponTypes.push_back(weaponType);
}