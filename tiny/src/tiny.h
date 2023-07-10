#pragma once
#include "tiny-pch.h"

#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/Engine.h"

#include "tiny/scene/Camera.h"

#include "tiny/rendering/BlendState.h"
#include "tiny/rendering/ConstantBuffer.h"
#include "tiny/rendering/DepthStencilState.h"
#include "tiny/rendering/DescriptorVector.h"
#include "tiny/rendering/GeometryGenerator.h"
#include "tiny/rendering/InputLayout.h"
#include "tiny/rendering/MeshGroup.h"
#include "tiny/rendering/RasterizerState.h"
#include "tiny/rendering/RenderItem.h"
#include "tiny/rendering/RenderPass.h"
#include "tiny/rendering/RenderPassLayer.h"
#include "tiny/rendering/RootConstantBufferView.h"
#include "tiny/rendering/RootDescriptorTable.h"
#include "tiny/rendering/RootSignature.h"
#include "tiny/rendering/Shader.h"
#include "tiny/rendering/Texture.h"

#include "tiny/utils/Constants.h"
#include "tiny/utils/Timer.h"
#include "tiny/utils/Profile.h"
#include "tiny/utils/MathHelper.h"

// TEMPORARY: We use TheApp class to implement app-specific stuff in the library itself.
//            Eventually this should be removed, but it serves as an easy way to run the code on Win32 and UWP right now
//#include "tiny/TheApp.h"