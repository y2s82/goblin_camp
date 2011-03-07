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

#pragma once

#include "NPC.hpp"
#include "tileRenderer/Sprite.hpp"

class NPCSprite
{
public:
	explicit NPCSprite();
	explicit NPCSprite(const Sprite& sprite);
	explicit NPCSprite(const std::vector<Sprite>& sprites, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes);
	~NPCSprite();

	bool IsEquipmentAware() const;
	bool Exists() const;

	void Draw(SDL_Surface * dst, SDL_Rect * dstRect) const;
	void Draw(boost::shared_ptr<NPC> npc, SDL_Surface * dst, SDL_Rect * dstRect) const;

private:
	std::vector<Sprite> sprites;
	bool equipmentAware;
	std::vector<std::string> weaponTypeNames;
	std::vector<std::string> armourTypeNames;
};

class NPCSpriteFactory
{
public:
	explicit NPCSpriteFactory();
	~NPCSpriteFactory();

	void Reset();
	NPCSprite Build(boost::shared_ptr<TileSetTexture> currentTexture);

	void AddSpriteFrame(int frame);
	void SetFPS(int fps);
	void SetEquipmentMap(bool equipmentMap);
	void AddArmourType(std::string armourType);
	void AddWeaponType(std::string weaponType);
	
private:
	std::vector<int> frames;
	std::vector<std::string> armourTypes;
	std::vector<std::string> weaponTypes;
	int frameRate;
	bool equipmentMap;
};
