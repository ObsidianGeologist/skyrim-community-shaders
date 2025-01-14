#pragma once

#include "../Widget.h"

class WorldSpaceWidget : public Widget
{
public:
	RE::TESWorldSpace* worldSpace = nullptr;

	WorldSpaceWidget(RE::TESWorldSpace* a_worldspace)
	{
		form = a_worldspace;
		worldSpace = a_worldspace;
	}

	~WorldSpaceWidget();

	virtual void DrawWidget() override;
};