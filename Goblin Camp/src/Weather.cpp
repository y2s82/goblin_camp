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
	prevailingWindDirection(NORTH), currentWeather(NORMALWEATHER), 
	tileChange(false),
	changeAll(false), tileChangeRate(0), changePosition(0),
	currentSeason(-1) {
}

Direction Weather::GetWindDirection() { return windDirection; }

void Weather::RandomizeWind() {
	prevailingWindDirection = static_cast<Direction>(Random::Generate(7));
	windDirection = prevailingWindDirection;
}

void Weather::ShiftWind() {
	if (Random::Generate(2) == 0) {
		windDirection = static_cast<Direction>(windDirection + Random::Generate(-1, 1));
		if (windDirection < 0) windDirection = NORTHWEST;
		if (windDirection > NORTHWEST) windDirection = NORTH;

		int distanceFromPrevailing = windDirection - prevailingWindDirection;

		if (distanceFromPrevailing == 3 || distanceFromPrevailing == 4 || distanceFromPrevailing == -5) 
			windDirection = static_cast<Direction>(windDirection - 1);
		else if (distanceFromPrevailing == -3 || distanceFromPrevailing == -4 || distanceFromPrevailing == 5) 
			windDirection = static_cast<Direction>(windDirection + 1);
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
	if (!tileChange && currentTemperature >= 0 && currentWeather == RAIN) Game::Inst()->CreateWater(Coordinate(Random::Generate(map->Width()-1),Random::Generate(map->Height()-1)),1);

	if (Game::Inst()->CurrentSeason() != static_cast<Season>(currentSeason)) {
		currentSeason = static_cast<int>(Game::Inst()->CurrentSeason());
		SeasonChange();
	}

	if (tileChange) {
		TileType tile = currentTemperature > 0 ? TILEGRASS : TILESNOW;
		if (!changeAll) {
			for (int i = 0; i < tileChangeRate; ++i) {
				Coordinate r = Random::ChooseInExtent(map->Extent());
				if (map->GetType(r) == TILEGRASS || map->GetType(r) == TILESNOW) {
					map->ChangeType(r, static_cast<TileType>(tile), map->heightMap->getValue(r.X(), r.Y()));
				}
				if (currentTemperature < 0) {
					if (map->GetWater(r).lock() && map->GetWater(r).lock()->IsCoastal()) {
						Game::Inst()->CreateNatureObject(Coordinate(r), "Ice");
					}
				} else if (currentTemperature > 0) {
					if (map->GetNatureObject(r) >= 0) {
						if (Game::Inst()->natureList[map->GetNatureObject(r)]->IsIce()) {
							Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[map->GetNatureObject(r)]);
						}
					}
				}
			}
			if (tileChangeRate < 300 && Random::Generate(200) == 0) ++tileChangeRate;
		} else {
			for (int i = 0; i < tileChangeRate; ++i) {
				for (int x = 0; x < 500; ++x) {
					Coordinate p(x, changePosition);
					if (map->GetType(p) == TILEGRASS || map->GetType(p) == TILESNOW) {
						map->ChangeType(p, static_cast<TileType>(tile), map->heightMap->getValue(p.X(),p.Y()));
					}

					if (currentTemperature < 0) {
						if (map->GetWater(p).lock() && map->GetWater(p).lock()->IsCoastal()) {
							Game::Inst()->CreateNatureObject(p, "Ice");
						}
					} else if (currentTemperature > 0) {
						if (map->GetNatureObject(p) >= 0) {
							if (Game::Inst()->natureList[map->GetNatureObject(p)]->IsIce()) {
								Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[map->GetNatureObject(p)]);
							}
						}
					}

				}
				++changePosition;
				if (changePosition >= map->Height()) {
					changeAll = false;
					tileChangeRate = 1;
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
		tileChangeRate = 500;
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
		tileChangeRate = 500;
		changePosition = 0;
		currentTemperature = 1;
		break;
	}
}

void Weather::ApplySeasonalEffects() {
	SeasonChange();
}

void Weather::save(OutputArchive& ar, const unsigned int version) const {
	ar & map;
	ar & windDirection;
	ar & prevailingWindDirection;
	ar & currentWeather;
	ar & tileChange;
	ar & changeAll;
	ar & tileChangeRate;
	ar & changePosition;
	ar & currentTemperature;
}

void Weather::load(InputArchive& ar, const unsigned int version) {
	ar & map;
	ar & windDirection;
	ar & prevailingWindDirection;
	ar & currentWeather;
	ar & tileChange;
	ar & changeAll;
	ar & tileChangeRate;
	ar & changePosition;
	ar & currentTemperature;
}
