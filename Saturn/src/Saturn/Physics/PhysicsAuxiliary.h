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

#include "PxPhysicsAPI.h"

#include "Saturn/Core/Maths.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#define PHYSX_TERMINATE_ITEM( x ) if(x) x->release(); x = nullptr

namespace Saturn::Auxiliary {

	inline physx::PxVec3 GLMToPx( const glm::vec3& vec ) 
	{
		return *( physx::PxVec3* )&vec;
	}

	inline physx::PxQuat QGLMToPx( const glm::quat& quat )
	{
		// X, Y, Z, W
		return physx::PxQuat( quat.x, quat.y, quat.z, quat.w );
	}

	inline glm::vec3 PxToGLM( physx::PxVec3 vec )
	{
		return *( glm::vec3* ) &vec;
	}

	inline glm::quat QPxToGLM( physx::PxQuat quat )
	{
		// We can't cast it to a glm::quat because PhysX's quats are XYZW however GLM's is WXYZ so it would return the wrong values.
		// We could define GLM_FORCE_QUAT_DATA_XYZW. However that does not seem to work.
		return glm::quat( quat.w, quat.x, quat.y, quat.z );
	}

	inline physx::PxTransform GLMTransformToPx( const glm::mat4& mat )
	{
		glm::vec3 pos{}, scale{};
		glm::quat rot{};

		Maths::DecomposeTransform( mat, pos, rot, scale );

		physx::PxQuat r = QGLMToPx( rot );
		physx::PxVec3 p = GLMToPx( pos );

		return physx::PxTransform( p, r );
	}

	inline glm::mat4 PxTransformToGLM( const physx::PxTransform& mat )
	{
		glm::quat q( QPxToGLM( mat.q ) );
		glm::vec3 p( PxToGLM( mat.p ) );

		return glm::translate( glm::mat4( 1.0f ), p ) * glm::toMat4( q );
	}

	inline physx::PxTransform GLMTransformToPx( const glm::vec3& position, const glm::vec3& rotation )
	{
		return physx::PxTransform( GLMToPx( position ), QGLMToPx( glm::quat( rotation ) ) );
	}
}
