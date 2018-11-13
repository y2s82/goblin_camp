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

#include "tileRenderer/Sprite.hpp"
#include "tileRenderer/TileSetTexture.hpp"
#include "tileRenderer/TileSetRenderer.hpp"
#include "tileRenderer/ConstructionSprite.hpp"
#include "tileRenderer/NPCSprite.hpp"

class ConstructionSpriteFactory
{
public:
	explicit ConstructionSpriteFactory();
	~ConstructionSpriteFactory();

	void Reset();
	ConstructionSprite Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture);

	template <typename IterT> void SetSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetUnderConstructionSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetUnreadyTrapSpriteIndices(IterT start, IterT end);

	void SetOpenDoorSprite(Sprite_ptr sprite);
	void SetWidth(int width);
	void SetFPS(int fps);
	void SetFrameCount(int frameCount);
	void SetConnectionMap(bool isConnectionMap);

private:
	std::vector<int> spriteIndices;
	std::vector<int> underConstructionSpriteIndices;
	std::vector<int> unreadyTrapSpriteIndices;
	Sprite_ptr openDoorSprite;
	int width;
	int frameRate;
	int frameCount;
	bool connectionMapped;
};

class NPCSpriteFactory
{
public:
	explicit NPCSpriteFactory();
	~NPCSpriteFactory();

	void Reset();
	NPCSprite Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture);

	void AddSpriteFrame(int frame);
	void SetFPS(int fps);
	void SetEquipmentMap(bool equipmentMap);
	void SetPaperdoll(bool paperDoll);
	void AddWeaponOverlay(int index);
	void AddArmourType(std::string armourType);
	void AddWeaponType(std::string weaponType);
	
private:
	std::vector<int> frames;
	std::vector<int> weaponOverlayIndices;
	std::vector<std::string> armourTypes;
	std::vector<std::string> weaponTypes;
	int frameRate;
	bool equipmentMap;
	bool paperdoll;
};

class StatusEffectSpriteFactory
{
public:
	explicit StatusEffectSpriteFactory();
	~StatusEffectSpriteFactory();

	void Reset();
	StatusEffectSprite Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture);

	void AddSpriteFrame(int frame);
	void SetFPS(int fps);
	void SetAlwaysOn(bool alwaysOn);
	void SetFlashRate(int flashRate);
	
private:
	std::vector<int> frames;
	int fps;
	bool alwaysOn;
	int flashRate;
};


class TerrainSpriteFactory
{
public:
	explicit TerrainSpriteFactory();
	~TerrainSpriteFactory();
	
	void Reset();
	TerrainSprite Build(boost::shared_ptr<TilesetRenderer> spriteFactory, boost::shared_ptr<TileSetTexture> currentTexture);

	template <typename IterT> void SetSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetSnowSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetHeightSplits(IterT start, IterT end);
	template <typename IterT> void SetEdgeSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetSnowEdgeSpriteIndices(IterT start, IterT end);
	void AddDetailSprite(Sprite_ptr sprite);
	void AddBurntDetailSprite(Sprite_ptr sprite);
	void AddSnowedDetailSprite(Sprite_ptr sprite);
	void AddCorruptedDetailSprite(Sprite_ptr sprite);
	void SetDetailsChance(float chance);
	void SetCorruptionSprite(Sprite_ptr sprite);
	void SetCorruptionOverlaySprite(Sprite_ptr sprite);
	void SetBurntSprite(Sprite_ptr sprite);
	void SetWang(bool wang);
	void SetSnowWang(bool wang);
private:
	std::vector<int> spriteIndices;
	std::vector<int> snowSpriteIndices;
	std::vector<float> heightSplits;
	std::vector<int> edgeIndices;
	std::vector<int> snowEdgeIndices;
	std::vector<Sprite_ptr> details;
	std::vector<Sprite_ptr> burntDetails;
	std::vector<Sprite_ptr> snowedDetails;
	std::vector<Sprite_ptr> corruptedDetails;
	int detailsChance;
	Sprite_ptr corruption;
	Sprite_ptr corruptionOverlay;
	Sprite_ptr burntOverlay;
	bool wang;
	bool snowWang;

};

template <typename IterT> void ConstructionSpriteFactory::SetSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		spriteIndices.push_back(*iter);
	}
}

template <typename IterT> void ConstructionSpriteFactory::SetUnderConstructionSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		underConstructionSpriteIndices.push_back(*iter);
	}
}

template <typename IterT> void ConstructionSpriteFactory::SetUnreadyTrapSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		unreadyTrapSpriteIndices.push_back(*iter);
	}
}

template <typename IterT> void TerrainSpriteFactory::SetSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		spriteIndices.push_back(*iter);
	}
}

template <typename IterT> void TerrainSpriteFactory::SetSnowSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		snowSpriteIndices.push_back(*iter);
	}
}


template <typename IterT> void TerrainSpriteFactory::SetHeightSplits(IterT iter, IterT end) {
   for (; iter != end; ++iter) {
     heightSplits.push_back( *reinterpret_cast<float*>(iter));
   }
}



template <typename IterT> void TerrainSpriteFactory::SetEdgeSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		edgeIndices.push_back(*iter);
	}
}

template <typename IterT> void TerrainSpriteFactory::SetSnowEdgeSpriteIndices(IterT iter, IterT end) {
	for (; iter != end; ++iter) {
		snowEdgeIndices.push_back(*iter);
	}
}