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
#pragma once

#include <vector>

#include "Coordinate.hpp"
#include "data/Serialization.hpp"

class Map;

enum WeatherType {
	NORMALWEATHER,
	RAIN
};

class Weather {
	GC_SERIALIZABLE_CLASS
	
	Map* map;
	Direction windDirection;
	Direction prevailingWindDirection;
	WeatherType currentWeather;
	bool tileChange;
	bool changeAll;
	int tileChangeRate;
	int changePosition;
	int currentTemperature;
	int currentSeason;
	void SeasonChange();

public:
	Weather(Map* map = 0);
	Direction GetWindDirection();
	void RandomizeWind();
	void ShiftWind();
	std::string GetWindAbbreviation();
	void Update();
	void ChangeWeather(WeatherType);
	void ApplySeasonalEffects();
};

BOOST_CLASS_VERSION(Weather, 0)
