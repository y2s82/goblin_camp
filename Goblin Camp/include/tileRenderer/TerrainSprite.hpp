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
#include "tileRenderer/PermutationTable.hpp"
#include <boost/shared_ptr.hpp>

class TerrainSprite
{
public:
	explicit TerrainSprite();
	explicit TerrainSprite(const Sprite& sprite);
	explicit TerrainSprite(std::vector<Sprite> sprites,
						   std::vector<Sprite> snowSprites,
						   std::vector<float> heightSplits,
						   Sprite edge,
						   Sprite snowEdge,
						   std::vector<Sprite> details,
						   std::vector<Sprite> burntDetails,
						   std::vector<Sprite> snowedDetails,
						   std::vector<Sprite> corruptedDetails,
						   int detailsChance,
						   Sprite corruption,
						   Sprite corruptionOverlay,
						   Sprite burntOverlay);
	~TerrainSprite();

	bool Exists() const;

	// Connection map draw
	void Draw(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected) const;
	void DrawCorrupted(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction corruptConnected) const;
	void DrawBurnt(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction burntConnected) const;
	void DrawSnowed(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected) const;
	void DrawSnowedAndCorrupted(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected, Sprite::ConnectedFunction corruptConnected) const;
	void DrawCorruptionOverlay(SDL_Surface * dst, SDL_Rect *dstRect, Sprite::ConnectedFunction) const;
	
	void SetCorruption(Sprite sprite);
	void SetCorruptionOverlay(Sprite sprite);

private:	
	std::vector<Sprite> sprites;
	std::vector<Sprite> snowSprites;
	std::vector<float> heightSplits;
	Sprite edge;
	Sprite snowEdge;
	std::vector<Sprite> details;
	std::vector<Sprite> burntDetails;
	std::vector<Sprite> snowedDetails;
	std::vector<Sprite> corruptedDetails;
	int detailsChance;
	Sprite corruption;
	Sprite corruptionOverlay;
	Sprite burntOverlay;
	
	// Just a little spice to separate details from normal sprites
	static const int detailPermOffset = 42;
	int numSprites;

	void DrawBaseLayer(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height) const;
	void DrawSnowLayer(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected) const;
	void DrawDetails(SDL_Surface * dst, SDL_Rect *dstRect, const std::vector<Sprite>& detailSprites, Coordinate coords, const PermutationTable& permTable) const;

};

class TerrainSpriteFactory
{
public:
	explicit TerrainSpriteFactory();
	~TerrainSpriteFactory();
	
	void Reset();
	TerrainSprite Build(boost::shared_ptr<TileSetTexture> currentTexture);

	template <typename IterT> void SetSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetSnowSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetHeightSplits(IterT start, IterT end);
	template <typename IterT> void SetEdgeSpriteIndices(IterT start, IterT end);
	template <typename IterT> void SetSnowEdgeSpriteIndices(IterT start, IterT end);
	void AddDetailSprite(const Sprite& sprite);
	void AddBurntDetailSprite(const Sprite& sprite);
	void AddSnowedDetailSprite(const Sprite& sprite);
	void AddCorruptedDetailSprite(const Sprite& sprite);
	void SetDetailsChance(float chance);
	void SetCorruptionSprite(const Sprite& sprite);
	void SetCorruptionOverlaySprite(const Sprite& sprite);
	void SetBurntSprite(const Sprite& sprite);
	void SetWang(bool wang);
	void SetSnowWang(bool wang);
private:
	std::vector<int> spriteIndices;
	std::vector<int> snowSpriteIndices;
	std::vector<float> heightSplits;
	std::vector<int> edgeIndices;
	std::vector<int> snowEdgeIndices;
	std::vector<Sprite> details;
	std::vector<Sprite> burntDetails;
	std::vector<Sprite> snowedDetails;
	std::vector<Sprite> corruptedDetails;
	int detailsChance;
	Sprite corruption;
	Sprite corruptionOverlay;
	Sprite burntOverlay;
	bool wang;
	bool snowWang;

};

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
		heightSplits.push_back(*iter);
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