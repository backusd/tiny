#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <array>
#include <format>
#include <exception>
#include <optional>
#include <filesystem>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <cstdint>
#include <ppl.h>
#include <utility>
#include <stdexcept>
#include <map>
#include <string_view>
#include <set>

#include <dxgidebug.h> // For DxgiInfoManager
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include "tiny/utils/Constants.h"
#include "tiny/utils/d3dx12.h"
#include "tiny/utils/DDSTextureLoader.h"
#include "tiny/utils/MathHelper.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "dxguid.lib")