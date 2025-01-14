#pragma once

#include "../Widget.h"

class WeatherWidget : public Widget
{
public:
	WeatherWidget* parent = nullptr;
	char currentParentBuffer[256] = "None";

	RE::TESWeather* weather = nullptr;

	WeatherWidget(RE::TESWeather* a_weather)
	{
		form = a_weather;
		weather = a_weather;
	}

	~WeatherWidget();

	virtual void DrawWidget() override;
};