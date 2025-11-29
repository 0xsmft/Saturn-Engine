/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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

#include "Pose.h"

#include <acl/core/sample_rounding_policy.h>
#include <acl/core/track_writer.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	class PoseWriter : public acl::track_writer
	{
	public:
		PoseWriter() = default;
		PoseWriter( Pose* pPose ) : m_pPose( pPose ) {}

		constexpr bool skip_track_scale( uint32_t index ) const { return index == 0; }

		void RTM_SIMD_CALL write_rotation( uint32_t index, rtm::quatf_arg0 value )
		{
			m_pPose->LocalTransforms[ index ].Rotation = glm::quat(
				rtm::quat_get_w( value ),
				rtm::quat_get_x( value ),
				rtm::quat_get_y( value ),
				rtm::quat_get_z( value )
			);
		}

		void RTM_SIMD_CALL write_translation( uint32_t index, rtm::vector4f_arg0 value )
		{
			rtm::vector_store3( value, glm::value_ptr( m_pPose->LocalTransforms[ index ].Position ) );
		}

		void RTM_SIMD_CALL write_scale( uint32_t index, rtm::vector4f_arg0 value ) 
		{
			rtm::vector_store3( value, glm::value_ptr( m_pPose->LocalTransforms[ index ].Scale ) );
		}

	private:
		Pose* m_pPose = nullptr;
	};
	
}
