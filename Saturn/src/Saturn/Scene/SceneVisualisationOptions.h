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

namespace Saturn {

	enum class PhysicsColliderVisualisationOptions : uint8_t
	{
		// Do not show at all
		Disabled,

		// Show all
		All,
		
		// Show selected only
		SelectedOnly,
	};
	
	enum AIVisualisationOptions : uint8_t
	{
		// Do not show at all
		AIVisualisationOptions_Disabled,

		// Show behaviour tree information
		// shows the current task, the asset etc
		AIVisualisationOptions_BehaviourTreeInfo = 1 << 0,

		// Show all navigation paths (default, fallback value)
		AIVisualisationOptions_NavPaths = 1 << 1,
	};

	// Visualisation options for skeletal meshes
	enum SkeletonVisualisationOptions : uint8_t 
	{
		SkeletonVisualisationOptions_Disabled,

		// Draw lines between each bone.
		SkeletonVisualisationOptions_BoneLines = 1 << 0,

		// Draw the bone name at the joint.
		SkeletonVisualisationOptions_Names = 1 << 1
	};

	struct SceneVisualisationOptions
	{
		bool ShowGrid = true;
		bool ShowGridOnRuntime = false;
		PhysicsColliderVisualisationOptions PhysColliderOptions = PhysicsColliderVisualisationOptions::SelectedOnly;
		uint8_t AIVisualisationOptions = AIVisualisationOptions_Disabled;
		uint8_t SkeletonVisualisationOptions = SkeletonVisualisationOptions_Disabled;
	};
}
