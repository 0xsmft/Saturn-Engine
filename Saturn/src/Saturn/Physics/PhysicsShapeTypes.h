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

#include <stdint.h>

namespace Saturn {

	enum class PhysicsShapeType : uint8_t
	{
		Unknown,
		Box,
		Sphere,
		Capusle,
		ConvexMesh,
		TriangleMesh
	};

	enum class ForceMode : uint8_t
	{
		Force,
		Impulse,
		ForceAndTorque,
		Torque
	};

	enum RigidbodyLockFlags : uint8_t
	{
		RigidbodyLock_PositionX = BIT( 0 ),
		RigidbodyLock_PositionY = BIT( 1 ),
		RigidbodyLock_PositionZ = BIT( 2 ),
		RigidbodyLock_RotationX = BIT( 3 ),
		RigidbodyLock_RotationY = BIT( 4 ),
		RigidbodyLock_RotationZ = BIT( 5 ),

		RigidbodyLock_AllPosition = RigidbodyLock_PositionX | RigidbodyLock_PositionY | RigidbodyLock_PositionZ,
		RigidbodyLock_AllRotation = RigidbodyLock_RotationX | RigidbodyLock_RotationY | RigidbodyLock_RotationZ,

		RigidbodyLock_AllAxes = RigidbodyLock_AllPosition | RigidbodyLock_AllRotation,
	};
}
