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
currentWeather(NORMALWEATHER) {
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
}

void Weather::ChangeWeather(WeatherType newWeather) {
	currentWeather = newWeather;
}