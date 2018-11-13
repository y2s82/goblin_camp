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
#include "tileRenderer/SpriteSetFactories.hpp"
#include "Logger.hpp"
#include "MathEx.hpp"

ConstructionSpriteFactory::ConstructionSpriteFactory() 
	: spriteIndices(),
	  underConstructionSpriteIndices(),
	  unreadyTrapSpriteIndices(),
	  openDoorSprite(),
	  width(1),
	  frameRate(15),
	  frameCount(1),
	  connectionMapped(false)
{}

ConstructionSpriteFactory::~ConstructionSpriteFactory() {}

void ConstructionSpriteFactory::Reset() {
	spriteIndices.clear();
	underConstructionSpriteIndices.clear();
	unreadyTrapSpriteIndices.clear();
	openDoorSprite = Sprite_ptr();
	width = 1;
	frameRate = 15;
	frameCount = 1;
	connectionMapped = false;
}

ConstructionSprite ConstructionSpriteFactory::Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture) {
	ConstructionSprite spriteSet = ConstructionSprite();
	if (connectionMapped) {
		if (spriteIndices.size() > 0) {
			spriteSet.AddSprite(spriteFactory->CreateSprite(currentTexture, spriteIndices, true, frameRate, frameCount));
		}
		if (underConstructionSpriteIndices.size() > 0) {
			spriteSet.AddUnderConstructionSprite(spriteFactory->CreateSprite(currentTexture, underConstructionSpriteIndices, true));
		}
		if (unreadyTrapSpriteIndices.size() > 0) {
			spriteSet.AddUnreadyTrapSprite(spriteFactory->CreateSprite(currentTexture, unreadyTrapSpriteIndices, true));
		}
	} else {
		int numSprites = spriteIndices.size() / frameCount;
		for (int sprite = 0; sprite < numSprites; ++sprite)
		{
			std::vector<int> frames;
			for (int frame = 0; frame < frameCount; ++frame) {
				frames.push_back(spriteIndices.at(sprite + frame * numSprites));
			}
			spriteSet.AddSprite(spriteFactory->CreateSprite(currentTexture, frames, false, frameRate));
		}
		
		for (std::vector<int>::iterator iter = underConstructionSpriteIndices.begin(); iter != underConstructionSpriteIndices.end(); ++iter) {
			spriteSet.AddUnderConstructionSprite(spriteFactory->CreateSprite(currentTexture, *iter));
		}
		for (std::vector<int>::iterator iter = unreadyTrapSpriteIndices.begin(); iter != unreadyTrapSpriteIndices.end(); ++iter) {
			spriteSet.AddUnreadyTrapSprite(spriteFactory->CreateSprite(currentTexture, *iter));
		}
		spriteSet.SetWidth(width);
	}
	spriteSet.SetOpenSprite(openDoorSprite);
	return spriteSet;
}

void ConstructionSpriteFactory::SetOpenDoorSprite(Sprite_ptr sprite) {
	openDoorSprite = sprite;
}

void ConstructionSpriteFactory::SetWidth(int w) {
	width = w;
}

void ConstructionSpriteFactory::SetFPS(int fps) {
	frameRate = fps;
}

void ConstructionSpriteFactory::SetFrameCount(int frames) {
	frameCount = frames;
}

void ConstructionSpriteFactory::SetConnectionMap(bool isConnectionMap) {
	connectionMapped = isConnectionMap;
}


NPCSpriteFactory::NPCSpriteFactory()
: frames(),
  weaponOverlayIndices(),
  armourTypes(),
  weaponTypes(),
  frameRate(15),
  equipmentMap(false),
  paperdoll(false)
{
}

NPCSpriteFactory::~NPCSpriteFactory() {}

void NPCSpriteFactory::Reset() {
	frames.clear();
	weaponOverlayIndices.clear();
	armourTypes.clear();
	weaponTypes.clear();
	frameRate = 15;
	equipmentMap = false;
	paperdoll = false;
}

NPCSprite NPCSpriteFactory::Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture) {
	if (equipmentMap) {
		if (frames.size() == (weaponTypes.size() + 1) * (armourTypes.size() + 1)) {
			std::vector<Sprite_ptr> sprites;
			for (std::vector<int>::iterator iter = frames.begin(); iter != frames.end(); ++iter)
				sprites.push_back(spriteFactory->CreateSprite(currentTexture, *iter));

			return NPCSprite(sprites, weaponTypes, armourTypes);
		} else if (frames.size() > 0) {
			return NPCSprite(spriteFactory->CreateSprite(currentTexture, frames[0]));
		} else {
			return NPCSprite();
		}
	} else if (paperdoll) {
		if (frames.size() == armourTypes.size() + 1 && weaponOverlayIndices.size() == weaponTypes.size()) {
			std::vector<Sprite_ptr> sprites;
			for (std::vector<int>::iterator iter = frames.begin(); iter != frames.end(); ++iter)
				sprites.push_back(spriteFactory->CreateSprite(currentTexture, *iter));

			std::vector<Sprite_ptr> weaponOverlays;
			for (std::vector<int>::iterator iter = weaponOverlayIndices.begin(); iter != weaponOverlayIndices.end(); ++iter)
				weaponOverlays.push_back(spriteFactory->CreateSprite(currentTexture, *iter));

			return NPCSprite(sprites, weaponOverlays, weaponTypes, armourTypes);
		} else if (frames.size() > 0) {
			return NPCSprite(spriteFactory->CreateSprite(currentTexture, frames[0]));
		} else {
			return NPCSprite();
		}
	} else {
		return NPCSprite(spriteFactory->CreateSprite(currentTexture, frames, false, frameRate));
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

void NPCSpriteFactory::SetPaperdoll(bool value) {
	paperdoll = value;
}

void NPCSpriteFactory::AddWeaponOverlay(int index) {
	weaponOverlayIndices.push_back(index);
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

StatusEffectSprite StatusEffectSpriteFactory::Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture) {
	StatusEffectSprite result(spriteFactory->CreateSprite(currentTexture, frames, false, fps), flashRate, alwaysOn);
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


TerrainSpriteFactory::TerrainSpriteFactory()
	: spriteIndices(),
	  snowSpriteIndices(),
	  heightSplits(),
	  edgeIndices(),
	  snowEdgeIndices(),
	  details(),
	  burntDetails(),
	  snowedDetails(),
	  corruptedDetails(),
	  detailsChance(1),
	  corruption(),
	  corruptionOverlay(),
	  burntOverlay(),
	  wang(false),
	  snowWang(false)
{
}


TerrainSpriteFactory::~TerrainSpriteFactory() {
} 

TerrainSprite TerrainSpriteFactory::Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture) {
	std::vector<Sprite_ptr> sprites;
	if (wang) {
		int indicesPerSprite = spriteIndices.size() / (heightSplits.size() + 1);
		for (int i = 0; i < static_cast<int>(heightSplits.size()) + 1; ++i) {
			sprites.push_back(TilesetRenderer::CreateSprite(spriteFactory, currentTexture, spriteIndices.begin() + i * indicesPerSprite, spriteIndices.begin() + (i + 1) * indicesPerSprite, true));
		}
	} else {
		for (std::vector<int>::iterator iter = spriteIndices.begin(); iter != spriteIndices.end(); ++iter) {
			sprites.push_back(spriteFactory->CreateSprite(currentTexture, *iter));
		}
	}
	
	std::vector<Sprite_ptr> snowSprites;
	if (snowWang) {
		snowSprites.push_back(spriteFactory->CreateSprite(currentTexture, snowSpriteIndices, true));
	} else {
		for (std::vector<int>::iterator iter = snowSpriteIndices.begin(); iter != snowSpriteIndices.end(); ++iter) {
			snowSprites.push_back(spriteFactory->CreateSprite(currentTexture, *iter));
		}
	}

	// Set the skipped edge sprite to an existing one
	Sprite_ptr edgeSprite = Sprite_ptr();
	if (!edgeIndices.empty()) {
		if (edgeIndices.size() == 4) {
			edgeIndices.push_back(currentTexture->Count());
		} else if (edgeIndices.size() == 15 || edgeIndices.size() == 46) {
			edgeIndices.insert(edgeIndices.begin() + 10, currentTexture->Count());
		} else if (edgeIndices.size() == 18) {
			edgeIndices.insert(edgeIndices.begin() + 4, currentTexture->Count());
		}
		edgeSprite = spriteFactory->CreateSprite(currentTexture, edgeIndices, true);
	}
	
	// Add extra sprite to snowSprite to complete connection map
	Sprite_ptr snowEdgeSprite = Sprite_ptr();
	if (!snowSpriteIndices.empty()) {
		if (snowSpriteIndices.size() > 0) {
			if (snowEdgeIndices.size() == 4) {
				snowEdgeIndices.push_back(snowSpriteIndices.at(0));
			} else if (snowEdgeIndices.size() == 15 || snowEdgeIndices.size() == 46) {
				snowEdgeIndices.insert(snowEdgeIndices.begin() + 10, currentTexture->Count());
			} else if (snowEdgeIndices.size() == 18) {
				snowEdgeIndices.insert(snowEdgeIndices.begin() + 4, snowSpriteIndices.at(0));
			}
		} 
		snowEdgeSprite = spriteFactory->CreateSprite(currentTexture, snowEdgeIndices, true);
	}
	return TerrainSprite(sprites, snowSprites, heightSplits, edgeSprite, snowEdgeSprite, details, burntDetails, snowedDetails, corruptedDetails, detailsChance, corruption, corruptionOverlay, burntOverlay);
}

void TerrainSpriteFactory::Reset() {
	spriteIndices.clear();
	snowSpriteIndices.clear();
	heightSplits.clear();
	edgeIndices.clear();
	snowEdgeIndices.clear();
	details.clear();
	burntDetails.clear();
	snowedDetails.clear();
	corruptedDetails.clear();
	detailsChance = 1;
	corruption = Sprite_ptr();
	corruptionOverlay = Sprite_ptr();
	burntOverlay = Sprite_ptr();
	wang = false;
	snowWang = false;
}

void TerrainSpriteFactory::AddDetailSprite(Sprite_ptr sprite) {
	details.push_back(sprite);
}

void TerrainSpriteFactory::AddBurntDetailSprite(Sprite_ptr sprite) {
	burntDetails.push_back(sprite);
}

void TerrainSpriteFactory::AddSnowedDetailSprite(Sprite_ptr sprite) {
	snowedDetails.push_back(sprite);
}

void TerrainSpriteFactory::AddCorruptedDetailSprite(Sprite_ptr sprite) {
	corruptedDetails.push_back(sprite);
}

void TerrainSpriteFactory::SetDetailsChance(float chance) {
	detailsChance = CeilToInt::convert(1.0f / chance);
}

void TerrainSpriteFactory::SetCorruptionSprite(Sprite_ptr sprite) {
	corruption = sprite;
}

void TerrainSpriteFactory::SetCorruptionOverlaySprite(Sprite_ptr sprite) {
	corruptionOverlay = sprite;
}

void TerrainSpriteFactory::SetBurntSprite(Sprite_ptr sprite) {
	burntOverlay = sprite;
}

void TerrainSpriteFactory::SetWang(bool value) {
	wang = value;
}

void TerrainSpriteFactory::SetSnowWang(bool value) {
	snowWang = value;
}
