/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#pragma once

// --- Core
#include "Saturn/Core/App.h"
#include "Saturn/Core/Base.h"
#include "Saturn/Core/EnvironmentVariables.h"
#include "Saturn/Core/Input.h"
#include "Saturn/Core/Layer.h"
#include "Saturn/Core/Timer.h"
#include "Saturn/Core/Timestep.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Buffer.h"
#include "Saturn/Core/AABB/AABB.h"
#include "Saturn/Core/Ruby/RubyCore.h"
#include "Saturn/Core/Ruby/RubyEventType.h"

// --- Asset
#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/AssetManager.h"

// --- ImGui
#include "Saturn/ImGui/ContentBrowserPanel/ContentBrowserPanel.h"
#include "Saturn/ImGui/SceneHierarchyPanel.h"
#include "Saturn/ImGui/Styles.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

// --- Rendering
#include "Saturn/Vulkan/Base.h"
#include "Saturn/Vulkan/SceneRenderer.h"
#include "Saturn/Vulkan/Shader.h"
#include "Saturn/Vulkan/VulkanContext.h"

// --- Physics
#include "Saturn/Physics/PhysicsCooking.h"
#include "Saturn/Physics/PhysicsFoundation.h"
#include "Saturn/Physics/PhysicsShapes.h"

// --- GameFramework
#include "Saturn/GameFramework/ActionBinding.h"
#include "Saturn/GameFramework/PlayerInputController.h"
#include "Saturn/GameFramework/SClass.h"

// --- Scene
#include "Saturn/Scene/Components.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Scene.h"
