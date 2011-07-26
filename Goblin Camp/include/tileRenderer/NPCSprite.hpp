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
	explicit NPCSprite(Sprite_ptr sprite);
	explicit NPCSprite(const std::vector<Sprite_ptr>& sprites, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes);
	explicit NPCSprite(const std::vector<Sprite_ptr>& sprites, const std::vector<Sprite_ptr>& weaponOverlays, const std::vector<std::string>& weaponTypes, const std::vector<std::string>& armourTypes);
	~NPCSprite();

	bool IsEquipmentAware() const;
	bool Exists() const;

	void Draw(int screenX, int screenY) const;
	void Draw(int screenX, int screenY, boost::shared_ptr<NPC> npc) const;

private:
	std::vector<Sprite_ptr> sprites;
	std::vector<Sprite_ptr> weaponOverlays;
	bool equipmentAware;
	bool paperdoll;
	std::vector<std::string> weaponTypeNames;
	std::vector<std::string> armourTypeNames;
};

