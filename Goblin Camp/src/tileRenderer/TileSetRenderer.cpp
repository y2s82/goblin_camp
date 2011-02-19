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

#include "tileRenderer/TileSetRenderer.hpp"
#include "MapMarker.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "data/Paths.hpp"
#include "MathEx.hpp"

#include "tileRenderer/DrawConstructionVisitor.hpp"

namespace {
	int NextPowerOfTwo(int val)
	{
		val--;
		val = (val >> 1) | val;
		val = (val >> 2) | val;
		val = (val >> 4) | val;
		val = (val >> 8) | val;
		val = (val >> 16) | val;
		val++;
		return val;
	}

	// Could generate this, but would want it constant as a game is saved and loaded.
	boost::array<int, 1024> permutationTable = {563, 546, 645, 295, 542, 9, 1, 182, 846, 471, 821, 44, 93, 805, 998, 595, 879, 898, 547, 762, 740, 815, 68, 311, 571, 120, 653, 85, 626, 557, 34, 344, 327, 213, 797, 49, 39, 492, 526, 291, 1008, 158, 212, 520, 436, 304, 305, 624, 600, 775, 377, 684, 176, 581, 983, 343, 334, 548, 743, 309, 715, 241, 410, 420, 412, 711, 754, 228, 816, 694, 770, 1003, 297, 432, 773, 963, 900, 374, 229, 739, 831, 399, 663, 812, 142, 474, 958, 699, 625, 196, 417, 118, 650, 680, 655, 80, 686, 56, 428, 224, 642, 405, 22, 54, 393, 926, 57, 139, 843, 267, 238, 422, 769, 99, 735, 424, 394, 461, 788, 149, 921, 521, 455, 636, 194, 912, 927, 479, 553, 109, 30, 966, 447, 755, 160, 457, 615, 568, 443, 862, 14, 67, 222, 169, 62, 667, 793, 397, 772, 931, 883, 530, 84, 859, 510, 42, 37, 157, 984, 161, 271, 651, 605, 573, 555, 830, 298, 441, 868, 807, 236, 1022, 354, 668, 757, 589, 867, 935, 180, 91, 452, 53, 691, 449, 819, 330, 364, 913, 86, 1010, 538, 692, 156, 386, 353, 914, 466, 874, 799, 768, 802, 226, 81, 817, 965, 543, 536, 373, 607, 554, 123, 719, 411, 493, 720, 602, 499, 506, 910, 253, 522, 301, 329, 633, 365, 511, 174, 360, 300, 290, 647, 844, 16, 366, 944, 384, 371, 168, 634, 707, 378, 361, 307, 901, 952, 609, 376, 616, 960, 1004, 92, 128, 170, 574, 153, 389, 904, 481, 896, 197, 207, 133, 968, 923, 970, 106, 209, 533, 138, 281, 710, 604, 273, 368, 456, 813, 877, 69, 515, 349, 630, 688, 459, 494, 64, 71, 516, 10, 206, 972, 383, 943, 732, 332, 178, 35, 717, 66, 167, 897, 463, 152, 725, 177, 646, 476, 623, 529, 880, 824, 239, 268, 687, 947, 586, 736, 89, 928, 598, 131, 242, 677, 783, 730, 671, 489, 670, 437, 464, 462, 164, 318, 551, 205, 369, 560, 480, 850, 532, 1016, 380, 544, 333, 784, 693, 129, 90, 596, 183, 531, 146, 603, 488, 220, 678, 559, 723, 969, 552, 857, 218, 696, 287, 408, 523, 995, 144, 907, 116, 427, 648, 737, 834, 11, 245, 517, 916, 302, 147, 32, 814, 362, 878, 73, 444, 509, 1001, 440, 644, 179, 496, 908, 442, 932, 996, 136, 113, 566, 130, 215, 52, 105, 549, 792, 201, 223, 1019, 172, 266, 999, 938, 665, 758, 312, 599, 864, 316, 629, 426, 997, 12, 433, 82, 237, 893, 954, 98, 933, 77, 88, 162, 18, 1012, 577, 870, 288, 485, 477, 856, 718, 256, 588, 322, 808, 733, 504, 491, 31, 920, 781, 911, 666, 847, 403, 979, 247, 3, 200, 981, 323, 826, 282, 800, 915, 611, 801, 435, 186, 319, 558, 125, 701, 448, 895, 132, 51, 714, 293, 570, 339, 727, 390, 285, 902, 881, 451, 582, 63, 47, 486, 885, 235, 541, 747, 890, 270, 565, 587, 584, 0, 326, 7, 631, 294, 617, 26, 50, 421, 20, 181, 257, 355, 992, 232, 258, 351, 876, 852, 484, 27, 74, 689, 409, 749, 690, 575, 137, 4, 25, 939, 974, 320, 8, 350, 21, 83, 519, 708, 827, 429, 500, 782, 95, 104, 356, 628, 122, 195, 219, 36, 777, 413, 751, 29, 278, 820, 259, 621, 345, 151, 922, 785, 742, 375, 204, 759, 387, 849, 942, 473, 283, 6, 869, 1021, 272, 810, 766, 791, 977, 795, 873, 439, 991, 134, 454, 572, 986, 338, 306, 505, 975, 303, 845, 886, 780, 803, 434, 415, 679, 635, 352, 248, 191, 790, 13, 601, 280, 225, 401, 233, 497, 728, 299, 836, 261, 796, 534, 620, 658, 950, 249, 382, 993, 115, 342, 251, 978, 450, 5, 508, 683, 528, 851, 764, 676, 702, 2, 490, 608, 835, 871, 392, 23, 614, 988, 507, 487, 124, 917, 367, 1011, 100, 503, 252, 798, 119, 1018, 1017, 243, 263, 418, 527, 45, 703, 594, 79, 760, 277, 540, 973, 175, 346, 348, 841, 962, 779, 884, 839, 335, 652, 934, 76, 722, 865, 675, 734, 337, 117, 78, 227, 97, 1002, 567, 924, 276, 203, 838, 40, 188, 861, 111, 682, 899, 919, 483, 65, 590, 315, 786, 458, 858, 704, 545, 561, 33, 103, 882, 171, 948, 190, 264, 657, 875, 951, 255, 698, 250, 246, 712, 1009, 891, 756, 716, 17, 578, 70, 721, 709, 918, 1014, 61, 936, 193, 406, 1020, 964, 664, 618, 60, 55, 143, 961, 591, 833, 700, 748, 579, 425, 674, 96, 284, 314, 673, 221, 1023, 230, 832, 632, 402, 771, 949, 244, 685, 460, 184, 613, 681, 324, 72, 695, 767, 155, 240, 189, 576, 825, 940, 823, 738, 388, 328, 467, 482, 192, 804, 1013, 654, 822, 539, 286, 929, 774, 746, 745, 729, 19, 840, 705, 419, 166, 126, 750, 154, 438, 211, 107, 478, 260, 512, 198, 94, 672, 776, 502, 606, 214, 640, 1007, 372, 108, 370, 127, 905, 43, 470, 28, 906, 697, 661, 148, 887, 641, 279, 930, 556, 980, 829, 744, 828, 855, 141, 231, 308, 889, 269, 199, 854, 860, 659, 894, 217, 580, 765, 985, 941, 789, 46, 275, 585, 296, 112, 564, 430, 806, 385, 866, 363, 1015, 404, 550, 400, 956, 468, 1005, 662, 569, 937, 925, 208, 110, 982, 325, 265, 453, 627, 216, 848, 59, 726, 976, 811, 888, 321, 446, 391, 407, 1006, 593, 909, 396, 135, 990, 535, 357, 763, 597, 637, 619, 498, 379, 274, 359, 254, 75, 753, 622, 187, 987, 358, 292, 87, 724, 649, 24, 101, 953, 612, 416, 472, 38, 592, 145, 341, 660, 741, 537, 114, 15, 863, 794, 955, 731, 842, 185, 610, 41, 656, 513, 48, 946, 957, 583, 262, 967, 971, 853, 310, 989, 959, 414, 495, 872, 518, 903, 706, 121, 469, 102, 465, 381, 778, 340, 331, 501, 234, 713, 1000, 210, 945, 165, 317, 289, 475, 524, 818, 638, 445, 173, 837, 140, 562, 159, 202, 994, 336, 514, 347, 431, 761, 163, 58, 313, 787, 639, 398, 752, 643, 809, 892, 423, 525, 395, 669, 150};

	inline int hash(int val)
	{
		return permutationTable[val & 0x3FF];
	}

	inline int hash(int x, int y)
	{
		return permutationTable[(permutationTable[x & 0x3FF] + y) & 0x3FF];
	}
}


 
TileSetRenderer::TileSetRenderer(int resolutionX, int resolutionY, boost::shared_ptr<TileSet> ts, TCODConsole * mapConsole) 
: tcodConsole(mapConsole),
  screenWidth(resolutionX), 
  screenHeight(resolutionY),
  keyColor(TCODColor::magenta),
  mapSurface(),
  tileSet(ts),
  mapOffsetX(0),
  mapOffsetY(0),
  startTileX(0),
  startTileY(0),
  cursorMode(Cursor_None),
  cursorHint(-1) {
   Uint32 rmask, gmask, bmask, amask;
   #if SDL_BYTEORDER == SDL_BIG_ENDIAN
       rmask = 0xff000000;
       gmask = 0x00ff0000;
       bmask = 0x0000ff00;
       amask = 0x000000ff;
   #else
       rmask = 0x000000ff;
       gmask = 0x0000ff00;
       bmask = 0x00ff0000;
       amask = 0xff000000;
   #endif
   SDL_Surface * temp = SDL_CreateRGBSurface(0, NextPowerOfTwo(screenWidth), NextPowerOfTwo(screenHeight), 32, rmask, gmask, bmask, amask);
   SDL_SetAlpha(temp, 0, SDL_ALPHA_OPAQUE);
   mapSurface = boost::shared_ptr<SDL_Surface>(SDL_DisplayFormat(temp), SDL_FreeSurface);
   SDL_FreeSurface(temp);

   // Make this a future option:
   //SDL_SetAlpha(tempBuffer.get(), SDL_SRCALPHA, 196);
   if (!mapSurface)
   {
	   LOG(SDL_GetError());
   }
   TCODSystem::registerSDLRenderer(this);
}

TileSetRenderer::~TileSetRenderer() {}

void TileSetRenderer::PreparePrefabs() 
{
	for (std::vector<NPCPreset>::iterator npci = NPC::Presets.begin(); npci != NPC::Presets.end(); ++npci) {
		npci->graphicsHint = tileSet->GetGraphicsHintFor(*npci);
	}
	for (std::vector<NatureObjectPreset>::iterator nopi = NatureObject::Presets.begin(); nopi != NatureObject::Presets.end(); ++nopi) {
		nopi->graphicsHint = tileSet->GetGraphicsHintFor(*nopi);
	}
	for (std::vector<ItemPreset>::iterator itemi = Item::Presets.begin(); itemi != Item::Presets.end(); ++itemi) {
		itemi->graphicsHint = tileSet->GetGraphicsHintFor(*itemi);
	}
	for (std::vector<ConstructionPreset>::iterator constructi = Construction::Presets.begin(); constructi != Construction::Presets.end(); ++constructi) {
		constructi->graphicsHint = tileSet->GetGraphicsHintFor(*constructi);
	}
	for (std::vector<SpellPreset>::iterator spelli = Spell::Presets.begin(); spelli != Spell::Presets.end(); ++spelli) {
		spelli->graphicsHint = tileSet->GetGraphicsHintFor(*spelli);
	}
}

Coordinate TileSetRenderer::TileAt(int x, int y, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) const {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	float left = focusX * tileSet->TileWidth() - viewportW * 0.5f;
	float up = focusY * tileSet->TileHeight() - viewportH * 0.5f;
	
	return Coordinate(FloorToInt::convert((left + x) / tileSet->TileWidth()), FloorToInt::convert((up + y) / tileSet->TileHeight()));
}

void TileSetRenderer::DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) {
	if (viewportW == -1) viewportW = screenWidth;
	if (viewportH == -1) viewportH = screenHeight;

	// Merge viewport to console
	if (tcodConsole != 0) {
		int charX, charY;
		TCODSystem::getCharSize(&charX, &charY);
		int offsetX = FloorToInt::convert(float(viewportX) / charX);
		int offsetY = FloorToInt::convert(float(viewportY) / charY);
		int sizeX = CeilToInt::convert(float(viewportW + offsetX * charX) / charX) - offsetX;
		int sizeY = CeilToInt::convert(float(viewportH + offsetY * charY) / charY) - offsetY;
		viewportX = offsetX * charX;
		viewportY = offsetY * charY;
		viewportW = sizeX * charX;
		viewportH = sizeY * charY;

		for (int x = offsetX; x < offsetX + sizeX; x++) {
			for (int y = offsetY; y < offsetY + sizeY; y++) {
				tcodConsole->putCharEx(x, y, ' ', TCODColor::black, keyColor);
				//tcodConsole->setDirty(x,y,1,1);
			}
		}
	}

	SDL_Rect viewportRect;
	viewportRect.x = viewportX;
	viewportRect.y = viewportY;
	viewportRect.w = viewportW;
	viewportRect.h = viewportH;
	
	SDL_SetClipRect(mapSurface.get(), &viewportRect);
	
	// These are over-estimates, sometimes fewer tiles are needed.
	startTileX = int((focusX * tileSet->TileWidth() - viewportRect.w / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - viewportRect.h / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + viewportRect.w / 2) + viewportX;
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + viewportRect.h / 2) + viewportY;
	int tilesX = CeilToInt::convert(boost::numeric_cast<float>(viewportRect.w) / tileSet->TileWidth()) + 2;
	int tilesY = CeilToInt::convert(boost::numeric_cast<float>(viewportRect.h) / tileSet->TileHeight()) + 2;
	
    // And then render to map
	for (int y = 0; y < tilesY; ++y) {
		for (int x = 0; x <= tilesX; ++x) {
			int tileX = x + startTileX;
			int tileY = y + startTileY;
			
			// Draw Terrain
			SDL_Rect dstRect(CalcDest(tileX, tileY));
			if (tileX >= 0 && tileX < map->Width() && tileY >= 0 && tileY < map->Height()) {
				DrawTerrain(map, tileX, tileY, &dstRect);			
				
				if (boost::shared_ptr<Construction> construction = (Game::Inst()->GetConstruction(map->GetConstruction(tileX,tileY))).lock()) {
					DrawConstructionVisitor visitor(this, tileSet.get(), map, mapSurface.get(), &dstRect, Coordinate(tileX,tileY));
					construction->AcceptVisitor(visitor);
				} else  {
					DrawFilth(map, tileX, tileY, &dstRect);
				}

				int natNum = map->GetNatureObject(tileX, tileY);
				if (natNum >= 0) {
					boost::shared_ptr<NatureObject> natureObj = Game::Inst()->natureList[natNum];
					if (natureObj->Marked())
					{
						tileSet->DrawMarkedOverlay(mapSurface.get(), &dstRect);
					}
					tileSet->DrawNatureObject(natureObj, mapSurface.get(), &dstRect);
				}

				if (map->GetOverlayFlags() & TERRITORY_OVERLAY) {
					DrawTerritoryOverlay(map, tileX, tileY, &dstRect);
				}
			}
			else {
				// Out of world
				SDL_FillRect(mapSurface.get(), &dstRect, 0);
			}
		}
	}

	DrawMarkers(map, startTileX, startTileY, tilesX, tilesY);

	DrawItems(startTileX, startTileY, tilesX, tilesY);
	DrawNPCs(startTileX, startTileY, tilesX, tilesY);
	DrawFires(startTileX, startTileY, tilesX, tilesY);
	DrawSpells(startTileX, startTileY, tilesX, tilesY);

	SDL_SetClipRect(mapSurface.get(), NULL);
}

float TileSetRenderer::ScrollRate() const {
	return 16.0f / (tileSet->TileWidth() + tileSet->TileHeight());
}

void TileSetRenderer::SetCursorMode(CursorType mode) {
	cursorMode = mode;
	cursorHint = -1;
}

void TileSetRenderer::SetCursorMode(const NPCPreset& preset) {
	cursorMode = Cursor_NPC_Mode;
	cursorHint = preset.graphicsHint;
}

void TileSetRenderer::SetCursorMode(const ItemPreset& preset) {
	cursorMode = Cursor_Item_Mode;
	cursorHint = preset.graphicsHint;
}

void TileSetRenderer::SetCursorMode(int other) {
	cursorMode = Cursor_None;
	cursorHint = -1;
}
	
void TileSetRenderer::DrawCursor(const Coordinate& pos, float focusX, float focusY, bool placeable) {
	startTileX = int((focusX * tileSet->TileWidth() - screenWidth / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - screenHeight / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + screenWidth / 2);
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + screenHeight / 2);

	SDL_Rect dstRect(CalcDest(pos.X(), pos.Y()));
	tileSet->DrawCursor(cursorMode, cursorHint, placeable, mapSurface.get(), &dstRect);
}

void TileSetRenderer::DrawCursor(const Coordinate& start, const Coordinate& end, float focusX, float focusY, bool placeable) {
	startTileX = int((focusX * tileSet->TileWidth() - screenWidth / 2) / tileSet->TileWidth()) - 1;
	startTileY = int((focusY * tileSet->TileHeight() - screenHeight / 2) / tileSet->TileHeight()) - 1;
	mapOffsetX = int(startTileX * tileSet->TileWidth() - focusX * tileSet->TileWidth() + screenWidth / 2);
	mapOffsetY = int(startTileY * tileSet->TileHeight() - focusY * tileSet->TileHeight() + screenHeight / 2);

	for (int x = start.X(); x <= end.X(); ++x) {
		for (int y = start.Y(); y <= end.Y(); ++y) {
			SDL_Rect dstRect(CalcDest(x, y));
			tileSet->DrawCursor(cursorMode, cursorHint, placeable, mapSurface.get(), &dstRect);
		}
	}
	
}

void TileSetRenderer::DrawItems(int startX, int startY, int sizeX, int sizeY) const {
	for (std::map<int,boost::shared_ptr<Item> >::iterator itemi = Game::Inst()->itemList.begin(); itemi != Game::Inst()->itemList.end(); ++itemi) {
		if (!itemi->second->ContainedIn().lock()) {
			Coordinate itemPos = itemi->second->Position();
			if (itemPos.X() >= startX && itemPos.X() < startX + sizeX
				&& itemPos.Y() >= startY && itemPos.Y() < startY + sizeY)
			{
				SDL_Rect dstRect(CalcDest(itemPos.X(), itemPos.Y()));
				tileSet->DrawItem(itemi->second, mapSurface.get(), &dstRect);
			}
		}
	}
}

void TileSetRenderer::DrawNPCs(int startX, int startY, int sizeX, int sizeY) const {
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
		Coordinate npcPos = npci->second->Position();
		if (npcPos.X() >= startX && npcPos.X() < startX + sizeX
				&& npcPos.Y() >= startY && npcPos.Y() < startY + sizeY)
		{
			SDL_Rect dstRect(CalcDest(npcPos.X(), npcPos.Y()));
			tileSet->DrawNPC(npci->second, mapSurface.get(), &dstRect);
		}
	}
}

void TileSetRenderer::DrawSpells(int startX, int startY, int sizeX, int sizeY) const {
	for (std::list<boost::shared_ptr<Spell> >::iterator spelli = Game::Inst()->spellList.begin(); spelli != Game::Inst()->spellList.end(); ++spelli) {
		Coordinate spellPos = (*spelli)->Position();
		if (spellPos.X() >= startX && spellPos.X() < startX + sizeX
				&& spellPos.Y() >= startY && spellPos.Y() < startY + sizeY)
		{
			SDL_Rect dstRect(CalcDest(spellPos.X(), spellPos.Y()));
			tileSet->DrawSpell(*spelli, mapSurface.get(), &dstRect);
		}
	}
}

void TileSetRenderer::DrawFires(int startX, int startY, int sizeX, int sizeY) const {
	for (std::list<boost::weak_ptr<FireNode> >::iterator firei = Game::Inst()->fireList.begin(); firei != Game::Inst()->fireList.end(); ++firei) {
		if (boost::shared_ptr<FireNode> fire = firei->lock())
		{
			Coordinate firePos = fire->GetPosition();
			if (firePos.X() >= startX && firePos.X() < startX + sizeX
					&& firePos.Y() >= startY && firePos.Y() < startY + sizeY)
			{
				SDL_Rect dstRect(CalcDest(firePos.X(), firePos.Y()));
				tileSet->DrawFire(fire, mapSurface.get(), &dstRect);
			}
		}
	}
}

void TileSetRenderer::DrawMarkers(Map * map, int startX, int startY, int sizeX, int sizeY) const {
	for (Map::MarkerIterator markeri = map->MarkerBegin(); markeri != map->MarkerEnd(); ++markeri) {
		int markerX = markeri->second.X();
		int markerY = markeri->second.Y();
		if (markerX >= startX && markerX < startX + sizeX
			&& markerY >= startY && markerY < startY + sizeY) {
				SDL_Rect dstRect( CalcDest(markerX, markerY));
				tileSet->DrawMarker(mapSurface.get(), &dstRect);
		}
	}
}

namespace {
	bool TerrainConnectionTest(Map* map, Coordinate origin, TileType type, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->Type(coord.X(), coord.Y()) == type;
	}

	bool CorruptionConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height())
		{
			return true;
		}
		return map->GetCorruption(coord.X(), coord.Y()) >= 100;
	}

	bool WaterConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (coord.X() < 0 || coord.Y() < 0 || coord.X() >= map->Width() || coord.Y() >= map->Height()) {
			return true;
		}
		else if (boost::shared_ptr<WaterNode> water = map->GetWater(coord.X(), coord.Y()).lock()) {
			return water->Depth() > 0;
		}
		return false;
	}

	bool MajorFilthConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<FilthNode> filth = map->GetFilth(coord.X(), coord.Y()).lock()) {
			return filth->Depth() > 4;
		}
		return false;
	}

	bool BloodConnectionTest(Map* map, Coordinate origin, Direction dir) {
		Coordinate coord = origin + Coordinate::DirectionToCoordinate(dir);
		if (boost::shared_ptr<BloodNode> blood = map->GetBlood(coord.X(), coord.Y()).lock()) {
			return blood->Depth() > 0;
		}
		return false;
	}
}

void TileSetRenderer::DrawTerrain(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	TileType type(map->Type(tileX, tileY));
	Coordinate pos(tileX, tileY);

	tileSet->DrawTerrain(type, boost::bind(&TerrainConnectionTest, map, pos, type, _1), mapSurface.get(), dstRect);
	
	int detailIndex = hash(tileX, tileY) % tileSet->GetDetailRange(); 
	tileSet->DrawDetail(detailIndex, mapSurface.get(), dstRect);

	// Corruption
	if (map->GetCorruption(tileX, tileY) >= 100) {
		tileSet->DrawCorruption(boost::bind(&CorruptionConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
	}


	// Water
	boost::weak_ptr<WaterNode> waterPtr = map->GetWater(tileX,tileY);
	if (boost::shared_ptr<WaterNode> water = waterPtr.lock()) {
		if (water->Depth() > 0)
		{
			tileSet->DrawWater(boost::bind(&WaterConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
		}
	}
	if (boost::shared_ptr<BloodNode> blood = map->GetBlood(tileX, tileY).lock()) {
		if (blood->Depth() > 0) {
			tileSet->DrawBlood(boost::bind(&BloodConnectionTest, map, pos, _1), mapSurface.get(), dstRect);
		}
	}
	if (map->GroundMarked(tileX, tileY)) {
		tileSet->DrawMarkedOverlay(mapSurface.get(), dstRect);
	}
	
}

void TileSetRenderer::DrawFilth(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	if (boost::shared_ptr<FilthNode> filth = map->GetFilth(tileX, tileY).lock()) {
		if (filth->Depth() > 4) {
			tileSet->DrawFilthMajor(boost::bind(&MajorFilthConnectionTest, map, Coordinate(tileX, tileY), _1), mapSurface.get(), dstRect);
		} else if (filth->Depth() > 0) {
			tileSet->DrawFilthMinor(mapSurface.get(), dstRect);
		}
	}
}

void TileSetRenderer::DrawTerritoryOverlay(Map* map, int tileX, int tileY, SDL_Rect * dstRect) const {
	tileSet->DrawTerritoryOverlay(map->IsTerritory(tileX,tileY), mapSurface.get(), dstRect);
}

void TileSetRenderer::render(void * surf,void * sdl_screen) {
	  SDL_Surface *tcod = (SDL_Surface *)surf;
	  SDL_Surface *screen = (SDL_Surface *)sdl_screen;

      SDL_Rect srcRect={0,0,screenWidth,screenHeight};
      SDL_Rect dstRect={0,0,screenWidth,screenHeight};
	  SDL_BlitSurface(mapSurface.get(), &srcRect, screen, &dstRect);
	  SDL_SetColorKey(tcod,SDL_SRCCOLORKEY, SDL_MapRGBA(tcod->format, keyColor.r, keyColor.g, keyColor.b, 255));
	  // TODO: Make this an option
	  //SDL_SetAlpha(tcod, SDL_SRCALPHA, 196);
	  SDL_BlitSurface(tcod, &srcRect, screen, &dstRect);
}
