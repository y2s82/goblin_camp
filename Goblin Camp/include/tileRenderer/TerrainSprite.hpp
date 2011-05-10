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
#include "tileRenderer/TileSetTexture.hpp"

class TerrainSprite
{
public:
	explicit TerrainSprite();
	explicit TerrainSprite(Sprite_ptr sprite);
	explicit TerrainSprite(std::vector<Sprite_ptr > sprites,
						   std::vector<Sprite_ptr > snowSprites,
						   std::vector<float> heightSplits,
						   Sprite_ptr edge,
						   Sprite_ptr snowEdge,
						   std::vector<Sprite_ptr > details,
						   std::vector<Sprite_ptr > burntDetails,
						   std::vector<Sprite_ptr > snowedDetails,
						   std::vector<Sprite_ptr > corruptedDetails,
						   int detailsChance,
						   Sprite_ptr corruption,
						   Sprite_ptr corruptionOverlay,
						   Sprite_ptr burntOverlay);
	~TerrainSprite();

	bool Exists() const;

	void Draw(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected) const;
	void DrawCorrupted(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction corruptConnected) const;
	void DrawBurnt(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction burntConnected) const;
	void DrawSnowed(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected) const;
	void DrawSnowedAndCorrupted(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected, Sprite::ConnectedFunction corruptConnected) const;
	void DrawCorruptionOverlay(int screenX, int screenY, Sprite::ConnectedFunction) const;
	
	void SetCorruption(Sprite_ptr sprite);
	void SetCorruptionOverlay(Sprite_ptr sprite);

private:	
	std::vector<Sprite_ptr> sprites;
	std::vector<Sprite_ptr> snowSprites;
	std::vector<float> heightSplits;
	Sprite_ptr edge;
	Sprite_ptr snowEdge;
	std::vector<Sprite_ptr> details;
	std::vector<Sprite_ptr> burntDetails;
	std::vector<Sprite_ptr> snowedDetails;
	std::vector<Sprite_ptr> corruptedDetails;
	int detailsChance;
	Sprite_ptr corruption;
	Sprite_ptr corruptionOverlay;
	Sprite_ptr burntOverlay;
	
	// Just a little spice to separate details from normal sprites
	static const int detailPermOffset = 42;
	int numSprites;

	void DrawBaseLayer(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height) const;
	void DrawSnowLayer(int screenX, int screenY, Coordinate coords, const PermutationTable& permTable, float height, Sprite::ConnectedFunction terrainConnected, Sprite::ConnectedFunction snowConnected, bool corrupt = false, Sprite::ConnectedFunction corruptConnect = 0) const;
	void DrawDetails(int screenX, int screenY, const std::vector<Sprite_ptr>& detailSprites, Coordinate coords, const PermutationTable& permTable) const;

};