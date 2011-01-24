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
#include "Construction.hpp"
#include "ConstructionVisitor.hpp"
#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>

class ConstructionSpriteSet
{
public:
	ConstructionSpriteSet();
	~ConstructionSpriteSet();

	void AddSprite(const Sprite& sprite);
	void AddUnderConstructionSprite(const Sprite& sprite);
	void SetWidth(int width);
	void SetOpenSprite(const Sprite& sprite);

	bool IsValid() const;
	bool HasUnderConstructionSprites() const;
		
	void Draw(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawUnderConstruction(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect *dstRect) const;
	void DrawOpen(const Coordinate& internalPos, SDL_Surface * dst, SDL_Rect * dstRecT) const;
private:	
	std::vector<Sprite> sprites;
	std::vector<Sprite> underconstructionSprites;
	Sprite openSprite;
	int width;
};
