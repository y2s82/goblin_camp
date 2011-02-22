/* Copyright 2010-2011 Ilkka Halila
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

#include "Weather.hpp"
#include "Random.hpp"
#include "Game.hpp"
#include "GCamp.hpp"

Weather::Weather(Map* vmap) : map(vmap), windDirection(NORTH),
currentWeather(NORMALWEATHER), tileChange(false),
changeAll(false), tileChangeRate(0), changePosition(0),
currentSeason(-1) {
}

Direction Weather::GetWindDirection() { return windDirection; }

void Weather::RandomizeWind() {
	windDirection = (Direction)Random::Generate(7);
}

void Weather::ShiftWind() {
	if (Random::Generate(2) == 0) {
		windDirection = (Direction)(windDirection + Random::Generate(-1, 1));
		if (windDirection < 0) windDirection = NORTHWEST;
		if (windDirection > NORTHWEST) windDirection = NORTH;
	}
}

std::string Weather::GetWindAbbreviation() {
	switch (windDirection) {
	case NORTH:
		return "N";
	case NORTHEAST:
		return "NE";
	case EAST:
		return "E";
	case SOUTHEAST:
		return "SE";
	case SOUTH:
		return "S";
	case SOUTHWEST:
		return "SW";
	case WEST:
		return "W";
	case NORTHWEST:
		return "NW";
	default:
		return "?";
	}
}

void Weather::Update() {
	if (Random::Generate(MONTH_LENGTH) == 0) ShiftWind();
	if (Random::Generate(MONTH_LENGTH) == 0) {
		if (Random::Generate(2) < 2) {
			currentWeather = NORMALWEATHER;
		} else currentWeather = RAIN;
	}
	if (currentWeather == RAIN) Game::Inst()->CreateWater(Coordinate(Random::Generate(map->Width()-1),Random::Generate(map->Height()-1)),1);

	if (Game::Inst()->CurrentSeason() != static_cast<Season>(currentSeason)) {
		currentSeason = static_cast<int>(Game::Inst()->CurrentSeason());
		SeasonChange();
	}

	if (tileChange) {
		TileType tile = currentTemperature > 0 ? TILEGRASS : TILESNOW;
		if (!changeAll) {
			for (int i = 0; i < tileChangeRate; ++i) {
				int rx = Random::Generate(map->Width()-1);
				int ry = Random::Generate(map->Height()-1);
				if (map->GetType(rx,ry) == TILEGRASS || map->GetType(rx,ry) == TILESNOW) {
					map->ChangeType(rx, ry, static_cast<TileType>(tile), map->heightMap->getValue(rx,ry));
				}
			}
			if (Random::Generate(99) == 0) ++tileChangeRate;
		} else {
			for (int i = 0; i < tileChangeRate; ++i) {
				for (int x = 0; x < 500; ++x) {
					if (map->GetType(x,changePosition) == TILEGRASS || map->GetType(x,changePosition) == TILESNOW) {
						map->ChangeType(x, changePosition, static_cast<TileType>(tile), map->heightMap->getValue(x,changePosition));
					}
				}
				++changePosition;
				if (changePosition >= map->Height()) {
					tileChange = false;
					break;
				}
			}
		}
	}
}

void Weather::ChangeWeather(WeatherType newWeather) {
	currentWeather = newWeather;
}

void Weather::SeasonChange() {
	tileChange = false;
	changeAll = false;

	switch (Game::Inst()->CurrentSeason()) {
	case EarlySummer:
	case Summer:
	case LateSummer:
	case EarlyFall:
	case Fall:
		currentTemperature = 1;
		break;

	case LateWinter:
		tileChange = true;
		tileChangeRate = 1;
		currentTemperature = 1;
		break;

	case LateFall:
		tileChange = true;
		tileChangeRate = 1;
		currentTemperature = -1;
		break;

	case EarlyWinter:
		tileChange = true;
		tileChangeRate = 200;
		currentTemperature = -1;
		break;

	case Winter:
		tileChange = true;
		changeAll = true;
		tileChangeRate = 10;
		changePosition = 0;
		currentTemperature = -1;
		break;

	case EarlySpring:
		tileChange = true;
		tileChangeRate = 100;
		currentTemperature = 1;
		break;

	case Spring:
		tileChange = true;
		tileChangeRate = 200;
		currentTemperature = 1;
		break;

	case LateSpring:
		tileChange = true;
		changeAll = true;
		tileChangeRate = 10;
		changePosition = 0;
		currentTemperature = 1;
		break;
	}
}