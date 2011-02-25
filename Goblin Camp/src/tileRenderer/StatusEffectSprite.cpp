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

StatusEffectSprite::StatusEffectSprite(Sprite sprite, int flashRate, bool alwaysOn)
	: sprite(sprite),
	  flashRate(flashRate == 0 ? 0 : 1000/(flashRate + 1)),
	  alwaysOn(alwaysOn)
{}
	
StatusEffectSprite::~StatusEffectSprite() {}

void StatusEffectSprite::Draw(bool forceOn, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (forceOn || flashRate == 0 || (TCODSystem::getElapsedMilli() / flashRate) % 2 == 0)
	{
		sprite.Draw(dst, dstRect);
	}
}

bool StatusEffectSprite::IsAlwaysVisible() const {
	return alwaysOn;
}

bool StatusEffectSprite::Exists() const {
	return sprite.Exists();
}

StatusEffectSpriteFactory::StatusEffectSpriteFactory() 
: frames(),
  fps(10),
  alwaysOn(false),
  flashRate(1)
{}

StatusEffectSpriteFactory::~StatusEffectSpriteFactory() {}

void StatusEffectSpriteFactory::Reset() {
	frames.clear();
	fps = 10;
	alwaysOn = false;
	flashRate = 1;
}

StatusEffectSprite StatusEffectSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	StatusEffectSprite result(Sprite(currentTexture,frames.begin(), frames.end(), false, fps), flashRate, alwaysOn);
	Reset();
	return result;
}

void StatusEffectSpriteFactory::AddSpriteFrame(int frame) {
	frames.push_back(frame);
}

void StatusEffectSpriteFactory::SetFPS(int framesPerSecond) {
	fps = framesPerSecond;
}

void StatusEffectSpriteFactory::SetAlwaysOn(bool on) {
	alwaysOn = on;
}

void StatusEffectSpriteFactory::SetFlashRate(int rate) {
	flashRate = rate;
}
