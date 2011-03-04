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
#include "tileRenderer/TerrainSprite.hpp"

#include "MathEx.hpp"

TerrainSprite::TerrainSprite()
	: sprites(),
	  snowSprites(),
	  heightSplits(),
	  edge(),
	  snowEdge(),
	  details(),
	  burntDetails(),
	  snowedDetails(),
	  corruptedDetails(),
	  detailsChance(1),
	  corruption(),
	  corruptionOverlay(),
	  burntOverlay(),
	  numSprites(0)
{
}

TerrainSprite::TerrainSprite(const Sprite& sprite) 
  : sprites(),
	snowSprites(),
	heightSplits(),
	edge(sprite),
	snowEdge(),
	details(),
	burntDetails(),
	snowedDetails(),
	corruptedDetails(),
	detailsChance(1),
	corruption(),
	corruptionOverlay(),
	burntOverlay(),
	numSprites(0)
{
}

TerrainSprite::TerrainSprite(std::vector<Sprite> sprites,
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
							 Sprite burntOverlay)
: sprites(sprites),
  snowSprites(snowSprites),
  heightSplits(heightSplits),
  edge(edge),
  snowEdge(snowEdge),
  details(details),
  burntDetails(burntDetails),
  snowedDetails(snowedDetails),
  corruptedDetails(corruptedDetails),
  detailsChance(detailsChance),
  corruption(corruption),
  corruptionOverlay(corruptionOverlay),
  burntOverlay(burntOverlay),
  numSprites(sprites.size() / (heightSplits.size() + 1))
{
}

TerrainSprite::~TerrainSprite() {
}

bool TerrainSprite::Exists() const {
	return numSprites > 0 || edge.Exists();
}

// Connection map draw
void TerrainSprite::Draw(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected) const {
	if (Exists()) {
		DrawBaseLayer(dst, dstRect, sprites, heightSplits, numSprites, coords, permTable, height);
		edge.Draw(terrainConnected, dst, dstRect);
		DrawDetails(dst, dstRect, details, coords, permTable);
	}
}

void TerrainSprite::DrawCorrupted(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction corruptConnected) const {
	if (Exists()) {
		DrawBaseLayer(dst, dstRect, sprites, heightSplits, numSprites, coords, permTable, height);
		corruption.Draw(corruptConnected, dst, dstRect);
		edge.Draw(terrainConnected, dst, dstRect);
		if (!corruptedDetails.empty()) {
			DrawDetails(dst, dstRect, corruptedDetails, coords, permTable);
		} else {
			DrawDetails(dst, dstRect, details, coords, permTable);
		}
	}
}

void TerrainSprite::DrawBurnt(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction burntConnected) const {
	if (Exists()) {
		DrawBaseLayer(dst, dstRect, sprites, heightSplits, numSprites, coords, permTable, height);
		burntOverlay.Draw(burntConnected, dst, dstRect);
		edge.Draw(terrainConnected, dst, dstRect);
		if (!burntDetails.empty()) {
			DrawDetails(dst, dstRect, burntDetails, coords, permTable);
		} else {
			DrawDetails(dst, dstRect, details, coords, permTable);
		}
	}
}

void TerrainSprite::DrawSnowed(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected) const {
	if (Exists() && (snowSprites.size() > 0 || snowEdge.Exists())) {
		if (!snowEdge.Exists() || (snowConnected(NORTH) && snowConnected(EAST) && snowConnected(SOUTH) && snowConnected(WEST) && snowConnected(NORTHEAST) && snowConnected(NORTHWEST) && snowConnected(SOUTHEAST) && snowConnected(SOUTHWEST))) {
			if (snowSprites.size() > 1) {
				snowSprites.at(permTable.Hash(permTable.Hash(coords.X()) + coords.Y()) % snowSprites.size()).Draw(dst, dstRect);			
			} else if (snowSprites.size() == 1) {
				snowSprites.at(0).Draw(dst, dstRect);
			} else {
				snowEdge.Draw(snowConnected, dst, dstRect);
			}
		} else {
			DrawBaseLayer(dst, dstRect, sprites, heightSplits, numSprites, coords, permTable, height);
			snowEdge.Draw(snowConnected, dst, dstRect);
		}
		edge.Draw(terrainConnected, dst, dstRect);
		if (!snowedDetails.empty()) {
			DrawDetails(dst, dstRect, snowedDetails, coords, permTable);
		} else {
			DrawDetails(dst, dstRect, details, coords, permTable);
		}
	} else {
		Draw(dst, dstRect, coords, permTable, height, terrainConnected);
	}
}

void TerrainSprite::DrawSnowedAndCorrupted(SDL_Surface * dst, SDL_Rect *dstRect, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected, Sprite::ConnectedFunction corruptConnected) const {
	if (Exists() && snowSprites.size() > 0) {
		if (snowConnected(NORTH) && snowConnected(EAST) && snowConnected(SOUTH) && snowConnected(WEST) && snowConnected(NORTHEAST) && snowConnected(NORTHWEST) && snowConnected(SOUTHEAST) && snowConnected(SOUTHWEST)) {
			if (snowSprites.size() > 1) {
				snowSprites.at(permTable.Hash(permTable.Hash(coords.X()) + coords.Y()) % snowSprites.size()).Draw(dst, dstRect);			
			} else if (snowSprites.size() == 1) {
				snowSprites.at(0).Draw(dst, dstRect);
			} else {
				snowEdge.Draw(snowConnected, dst, dstRect);
			}
		} else {
			DrawBaseLayer(dst, dstRect, sprites, heightSplits, numSprites, coords, permTable, height);
			snowEdge.Draw(snowConnected, dst, dstRect);
		}
		edge.Draw(terrainConnected, dst, dstRect);
		corruption.Draw(corruptConnected, dst, dstRect);
		if (!corruptedDetails.empty()) {
			DrawDetails(dst, dstRect, corruptedDetails, coords, permTable);
		} else if (!snowedDetails.empty()) {
			DrawDetails(dst, dstRect, snowedDetails, coords, permTable);
		} else {
			DrawDetails(dst, dstRect, details, coords, permTable);
		}
	} else {
		DrawCorrupted(dst, dstRect, coords, permTable, height, terrainConnected, corruptConnected);
	}
}

void TerrainSprite::DrawCorruptionOverlay(SDL_Surface * dst, SDL_Rect *dstRect, Sprite::ConnectedFunction connected) const {
	corruptionOverlay.Draw(connected, dst, dstRect);
}
	
void TerrainSprite::DrawBaseLayer(SDL_Surface * dst, SDL_Rect *dstRect, const std::vector<Sprite>& sprites, const std::vector<float>& heightSplits, int numSprites, Coordinate coords, const PermutationTable& permTable, float height) const {
	if (sprites.size() > 0) {
		sprites[0].Draw(dst, dstRect);
		// Calculate height layer
		int heightLayer = 0;
		for (int i = 0; i < heightSplits.size(); ++i) {
			if (height >= heightSplits[i]) {
				heightLayer++;
			} else {
				break;
			}
		}

		// Base Layer
		if (numSprites > 1) {
			sprites[heightLayer * numSprites + permTable.Hash(permTable.Hash(coords.X()) + coords.Y()) % numSprites].Draw(dst, dstRect);			
		} else {
			sprites[heightLayer].Draw(dst, dstRect);
		}
	}
}

void TerrainSprite::DrawDetails(SDL_Surface * dst, SDL_Rect *dstRect, const std::vector<Sprite>& detailSprites, Coordinate coords, const PermutationTable& permTable) const {
	if (detailsChance != 0 && !detailSprites.empty()) {
		int detailChoice = permTable.Hash(permTable.Hash(coords.X() + detailPermOffset) + coords.Y()) % (detailsChance * detailSprites.size());
		if (detailChoice < detailSprites.size()) {
			detailSprites[detailChoice].Draw(dst, dstRect);
		}
	}
}

void TerrainSprite::SetCorruption(Sprite sprite) {
	corruption = sprite;
}

void TerrainSprite::SetCorruptionOverlay(Sprite sprite) {
	corruptionOverlay = sprite;
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
	  burntOverlay()
{

}

TerrainSpriteFactory::~TerrainSpriteFactory() {
}

TerrainSprite TerrainSpriteFactory::Build(boost::shared_ptr<TileSetTexture> currentTexture) {
	std::vector<Sprite> sprites;
	for (std::vector<int>::iterator iter = spriteIndices.begin(); iter != spriteIndices.end(); ++iter) {
		sprites.push_back(Sprite(currentTexture, *iter));
	}
	
	std::vector<Sprite> snowSprites;
	for (std::vector<int>::iterator iter = snowSpriteIndices.begin(); iter != snowSpriteIndices.end(); ++iter) {
		snowSprites.push_back(Sprite(currentTexture, *iter));
	}

	Sprite edgeSprite;
	Sprite snowEdgeSprite;
	if (sprites.size() > 0) {
		// Set the skipped edge sprite to an existing one
		if (edgeIndices.size() == 4) {
			edgeIndices.push_back(currentTexture->Count());
		} else if (edgeIndices.size() == 15 || edgeIndices.size() == 46) {
			edgeIndices.insert(edgeIndices.begin() + 10, currentTexture->Count());
		} else if (edgeIndices.size() == 18) {
			edgeIndices.insert(edgeIndices.begin() + 4, currentTexture->Count());
		}
		edgeSprite = Sprite(currentTexture, edgeIndices.begin(), edgeIndices.end(), true);
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
	corruption = Sprite();
	corruptionOverlay = Sprite();
	burntOverlay = Sprite();
}

void TerrainSpriteFactory::AddDetailSprite(const Sprite& sprite) {
	details.push_back(sprite);
}

void TerrainSpriteFactory::AddBurntDetailSprite(const Sprite& sprite) {
	burntDetails.push_back(sprite);
}

void TerrainSpriteFactory::AddSnowedDetailSprite(const Sprite& sprite) {
	snowedDetails.push_back(sprite);
}

void TerrainSpriteFactory::AddCorruptedDetailSprite(const Sprite& sprite) {
	corruptedDetails.push_back(sprite);
}

void TerrainSpriteFactory::SetDetailsChance(float chance) {
	detailsChance = CeilToInt::convert(1.0f / chance);
}

void TerrainSpriteFactory::SetCorruptionSprite(const Sprite& sprite) {
	corruption = sprite;
}

void TerrainSpriteFactory::SetCorruptionOverlaySprite(const Sprite& sprite) {
	corruptionOverlay = sprite;
}

void TerrainSpriteFactory::SetBurntSprite(const Sprite& sprite) {
	burntOverlay = sprite;
}