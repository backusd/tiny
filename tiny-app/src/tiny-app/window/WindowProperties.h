#pragma once
#include "tiny-app-pch.h"


namespace tiny
{
	struct WindowProperties
	{
		std::string title;
		unsigned int width;
		unsigned int height;

		WindowProperties(const std::string& title = "Tiny App",
			unsigned int width = 1280,
			unsigned int height = 720) noexcept :
			title(title), width(width), height(height)
		{}
	};





}