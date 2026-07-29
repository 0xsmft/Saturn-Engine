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

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Saturn {

	struct AnimatorLocalTransfrom
	{
		glm::vec3 Position = glm::zero<glm::vec3>();
		glm::quat Rotation = glm::identity<glm::quat>();
		glm::vec3 Scale = glm::one<glm::vec3>();
	};

	// Animator Pose
	// ~~
	// A pose is simply a collection of transforms that the bones are currently in.
	// It is used in conjunction with a current Animation. In some cases a Pose may be used to represent the resting position of a skeleton.
	// Poses are allocated on the heap and again are allocated per animation.
	struct Pose
	{
		AnimatorLocalTransfrom RootMotion;
		std::array<AnimatorLocalTransfrom, SK_MAX_BONES> LocalTransforms;
		float Duration = 0.0f;
		float Timestamp = 0.0f;
		// Number of bones
		uint64_t BonesUsed = 0llu;
	};
	
}
