#pragma once
#include "pch.h"
#include "Core.h"


namespace tiny
{
	class TINY_APP_API Application
	{
	public:
		Application() {}
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		virtual ~Application() {};

		void Run();
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}