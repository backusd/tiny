#pragma once
#include "tiny-pch.h"

#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/Engine.h"

#include "tiny/rendering/MeshGeometry.h"

#include "tiny/utils/Timer.h"

// TEMPORARY: We use TheApp class to implement app-specific stuff in the library itself.
//            Eventually this should be removed, but it serves as an easy way to run the code on Win32 and UWP right now
#include "tiny/TheApp.h"