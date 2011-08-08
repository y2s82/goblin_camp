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
#include "tileRenderer/StatusEffectSprite.hpp"

StatusEffectSprite::StatusEffectSprite()
	: sprite(),
	  alwaysOn(false),
	  flashRate(0)
{}

StatusEffectSprite::StatusEffectSprite(Sprite_ptr sprite, int flashRate, bool alwaysOn)
	: sprite(sprite),
	  alwaysOn(alwaysOn),
	  flashRate(flashRate == 0 ? 0 : 1000/(flashRate + 1))
{}
	
StatusEffectSprite::~StatusEffectSprite() {}

void StatusEffectSprite::Draw(int screenX, int screenY, bool forceOn) const {
	if (forceOn || flashRate == 0 || (TCODSystem::getElapsedMilli() / flashRate) % 2 == 0)
	{
		sprite.Draw(screenX, screenY);
	}
}

bool StatusEffectSprite::IsAlwaysVisible() const {
	return alwaysOn;
}

bool StatusEffectSprite::Exists() const {
	return sprite.Exists();
}
