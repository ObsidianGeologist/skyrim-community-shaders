#pragma once

#include "Buffer.h"

#include "Weather/CloudsWidget.h"
#include "Weather/WeatherWidget.h"
#include "Weather/WorldSpaceWidget.h"
#include "Widget.h"

class EditorWindow
{
public:
	static EditorWindow* GetSingleton()
	{
		static EditorWindow singleton;
		return &singleton;
	}

	Texture2D* tempTexture;

	std::vector<Widget*> weatherWidgets;
	std::vector<Widget*> worldSpaceWidgets;
	std::vector<Widget*> cloudsWidgets;

	void ShowObjectsWindow();

	void ShowViewportWindow();

	void ShowWidgetWindow();

	void RenderUI();

	void SetupResources();

	void Draw();
};