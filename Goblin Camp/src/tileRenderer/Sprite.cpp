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

#include "tileRenderer/Sprite.hpp"

Sprite::Sprite() : tiles(), texture(), type(SPRITE_Single), frameTime(15) {}
Sprite::Sprite(boost::shared_ptr<TileSetTexture> tilesetTexture, int tile)
	: tiles(),
	  texture(tilesetTexture),
	  type(SPRITE_Single),
	  frameTime(15)
{
	tiles.push_back(tile);
}



Sprite::~Sprite() {}

bool Sprite::Exists() const
{
	return tiles.size() > 0;
}

bool Sprite::IsConnectionMap() const {
	return type & (SPRITE_SimpleConnectionMap | SPRITE_NormalConnectionMap | SPRITE_ExtendedConnectionMap);
}

bool Sprite::IsAnimated() const {
	return type & SPRITE_Animated;
}

namespace {
	inline int ConnectionIndex(bool connectNorth, bool connectEast, bool connectSouth, bool connectWest) {
		int index = 0;
		if (!connectSouth) index += 8;
		if (connectSouth == connectNorth) index +=4;
		if (connectWest) index += 2;
		if (connectEast != connectWest) index += 1;
		return index;
	}

	static boost::array<short, 256> BuildExtendedConnectionLookupTable()
	{
		boost::array<short,15> lookupTable4Sides =
		{
			34, // None
			42, // NE
			44, // SE
			32, // NE + SE
			43, // SW
			46, // NE + SW
			24, // SE + SW
			22, // NE + SE + SW
			41, // NW
			29, // NE + NW
			45, // SE + NW
			27, // NE + SE + NW
			33, // SW + NW
			28, // NE + SW + NW
			23  // SE + SW + NW
		};
		boost::array<short,12> lookupTable3Sides =
		{
			19, // !N, !SE + !SW
			17, // !N, !SW
			18, // !N, !SE
			35, // !E, !SW + !NW
			25, // !E, !NW
			30, // !E, !SW
			39, // !S, !NE + !NW
			37, // !S, !NW
			38, // !S, !NE
			31, // !W, !NE + !SE
			26, // !W, !SE
			21  // !W, !NE		
		};

		boost::array<short, 256> result;
		for (int i = 0; i < 256; i++)
		{
			bool connectN = i & 0x1;
			bool connectE = i & 0x2;
			bool connectS = i & 0x4;
			bool connectW = i & 0x8;
			bool connectNE = i & 0x10;
			bool connectSE = i & 0x20;
			bool connectSW = i & 0x40;
			bool connectNW = i & 0x80;

			if ((connectN && connectE && !connectNE) || 
				(connectE && connectS && !connectSE) ||
				(connectS && connectW && !connectSW) ||
				(connectW && connectN && !connectNW))
			{
				int sides = ((connectN) ? 1 : 0) + ((connectS) ? 1 : 0) + ((connectE) ? 1 : 0) + ((connectW) ? 1 : 0);

				if (sides == 4) {
					int cornerScore = ((connectNE) ? 1 : 0) + ((connectSE) ? 2 : 0) + ((connectSW) ? 4 : 0) + ((connectNW) ? 8 : 0);
					result[i] = lookupTable4Sides[cornerScore];
				} else if (sides == 3) {
					if (!connectN) {
						result[i] = lookupTable3Sides[(connectSE ? 1 : 0) + (connectSW ? 2 : 0)];
					} else if (!connectE) {
						result[i] = lookupTable3Sides[3 + (connectSW ? 1 : 0) + (connectNW ? 2 : 0)];
					} else if (!connectS) {
						result[i] = lookupTable3Sides[6 + (connectNE ? 1 : 0) + (connectNW ? 2 : 0)];
					} else {
						result[i] = lookupTable3Sides[9 + (connectNE ? 1 : 0) + (connectSE ? 2 : 0)];
					}
				} else {
					int index = 16;
					if (connectW) index += 4;
					if (connectN) index += 20;
					result[i] = index;
				}
			}
			else 
			{
				result[i] = ConnectionIndex(connectN, connectE, connectS, connectW);
			}
		}
		return result;
	}

	inline int ExtConnectionIndex(Sprite::ConnectedFunction connected) {
		static boost::array<short,256> lookupTable(BuildExtendedConnectionLookupTable());
		int index = (connected(NORTH) << 0) +
					(connected(EAST) << 1) +
					(connected(SOUTH) << 2) +
					(connected(WEST) << 3) +
					(connected(NORTHEAST) << 4) +
					(connected(SOUTHEAST) << 5) +
					(connected(SOUTHWEST) << 6) +
					(connected(NORTHWEST) << 7);
		return lookupTable[index];
	}
}

void Sprite::Draw(SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (Exists()) {
		if (IsAnimated()) {
			int frame = (TCODSystem::getElapsedMilli() / frameTime) % tiles.size();
			texture->DrawTile(tiles[frame], dst, dstRect);
		} else {
			texture->DrawTile(tiles[0], dst, dstRect);
		}
	}
}
	
void Sprite::Draw(ConnectedFunction connected, SDL_Surface * dst, SDL_Rect * dstRect) const {
	if (IsConnectionMap()) {
		if (type & SPRITE_ExtendedConnectionMap) {
			int index = ExtConnectionIndex(connected);
			texture->DrawTile(tiles.at(index), dst, dstRect);
		} else if (type & SPRITE_NormalConnectionMap) {
			int index = ConnectionIndex(connected(NORTH), connected(EAST), connected(SOUTH), connected(WEST));
			texture->DrawTile(tiles.at(index), dst, dstRect);
		} else {
			DrawSimpleConnected(connected, dst, dstRect);
		}
	} else {
		Draw(dst, dstRect);
	}

}

void Sprite::DrawSimpleConnected(ConnectedFunction connected, SDL_Surface * dst, SDL_Rect * dstRect) const {
	boost::array<bool, 2> vertConnected = {connected(NORTH), connected(SOUTH)};
	boost::array<bool, 2> horizConnected = {connected(WEST), connected(EAST)};
	boost::array<bool, 4> cornerConnected = {connected(NORTHWEST), connected(NORTHEAST), connected(SOUTHWEST), connected(SOUTHEAST)};
	
	for (int vertDirection = 0; vertDirection < 2; ++vertDirection) {
		for (int horizDirection = 0; horizDirection < 2; ++horizDirection) {
			TileSetTexture::Corner corner = static_cast<TileSetTexture::Corner>(horizDirection + 2 * vertDirection);
			int index = (vertConnected[vertDirection] ? 1 : 0) + (horizConnected[horizDirection] ? 2 : 0);
			if (index == 3 && cornerConnected[corner]) {
				index ++;
			} 
			texture->DrawTileCorner(tiles[index], corner, dst, dstRect);
		}
	}
}




