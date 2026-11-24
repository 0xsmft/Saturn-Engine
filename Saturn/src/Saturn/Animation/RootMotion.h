/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Saturn {

	//
	// Controls which parts of the root bone's motion are taken out of the animation and given to the entity.
	// Anything that is not extracted stays in the pose.
	//
	struct RootMotionSettings
	{
		// Extract movement along the ground plane (X/Z).
		bool ExtractTranslation = true;

		// Extract movement along the up axis (Y). 
		// Off by default so gravity/character controllers keep control of height.
		bool ExtractVerticalTranslation = false;

		// Extract rotation around the up axis (yaw).
		bool ExtractRotation = true;

		[[nodiscard]] bool ExtractsAnything() const { return ExtractTranslation || ExtractVerticalTranslation || ExtractRotation; }
	};

	//
	// A rigid transform (translation + rotation) used for root motion.
	//
	// Root motion is always expressed in the space of the mesh (i.e. the entity's local space),
	// so to apply it to an entity the translation must be scaled and rotated by the entity's transform.
	//
	struct RootMotionTransform
	{
		glm::vec3 Translation = glm::vec3( 0.0f );
		glm::quat Rotation = glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );

		// Apply rOther after this transform.
		RootMotionTransform operator*( const RootMotionTransform& rOther ) const
		{
			RootMotionTransform result;
			result.Translation = Translation + Rotation * rOther.Translation;
			result.Rotation = glm::normalize( Rotation * rOther.Rotation );
			return result;
		}

		RootMotionTransform& operator*=( const RootMotionTransform& rOther )
		{
			*this = *this * rOther;
			return *this;
		}

		RootMotionTransform Inverse() const
		{
			RootMotionTransform result;
			result.Rotation = glm::inverse( Rotation );
			result.Translation = result.Rotation * -Translation;
			return result;
		}

		bool IsIdentity() const
		{
			return Translation == glm::vec3( 0.0f ) && Rotation == glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );
		}
	};

	//
	// Works out the root motion of one animation from its root bone.
	//
	// Everything is worked out in mesh space (the root bone multiplied by the skeleton's transform).
	//
	// For any time in the animation we build a "root motion frame" F(t) from the root bone, made from the
	// extracted parts of its position and its yaw relative to the start of the animation, so F(0) is identity.
	//
	// - The motion between two times a and b is F(a)^-1 * F(b), this is what the entity is moved by.
	// - The in-place pose is F(t)^-1 * Root(t), this is what gets rendered.
	//
	// So entity * in-place pose always matches where the root bone would have been without root motion.
	//
	class RootMotionExtractor
	{
	public:
		//
		// @param rSettings          -- what to extract.
		// @param rSkeletonTransform -- the transform the root motion bone is local to (the skeleton's transform * its parent bones).
		// @param rStartPosition, rStartRotation -- the root motion bone (local) at the start of the animation.
		// @param rEndPosition, rEndRotation     -- the root motion bone (local) at the end of the animation.
		//
		void Initialise(
			const RootMotionSettings& rSettings, const glm::mat4& rSkeletonTransform,
			const glm::vec3& rStartPosition, const glm::quat& rStartRotation,
			const glm::vec3& rEndPosition, const glm::quat& rEndRotation )
		{
			*this = {};
			m_Settings = rSettings;

			const glm::mat3 basis( rSkeletonTransform );
			if( glm::abs( glm::determinant( basis ) ) > 1e-12f )
			{
				m_SkeletonTransform = rSkeletonTransform;
				m_InverseSkeletonTransform = glm::inverse( rSkeletonTransform );

				// Remove the scale so we are left with the rotation.
				const glm::mat3 rotation( glm::normalize( basis[ 0 ] ), glm::normalize( basis[ 1 ] ), glm::normalize( basis[ 2 ] ) );
				m_SkeletonRotation = glm::normalize( glm::quat_cast( rotation ) );
				m_InverseSkeletonRotation = glm::inverse( m_SkeletonRotation );
			}

			m_StartPosition = Mask( ToMeshSpace( rStartPosition ) );
			m_InverseStartRotation = glm::inverse( ToMeshSpace( rStartRotation ) );

			m_FullCycle = CalculateFrame( rEndPosition, rEndRotation );
		}

		//
		// @returns F(t) for the root bone (local) at some time t.
		//
		RootMotionTransform CalculateFrame( const glm::vec3& rRootPosition, const glm::quat& rRootRotation ) const
		{
			RootMotionTransform frame;

			if( m_Settings.ExtractRotation )
				frame.Rotation = ExtractYaw( ToMeshSpace( rRootRotation ) * m_InverseStartRotation );

			// F = Translate( position ) * Yaw * Translate( -start ), so we rotate around the root bone and not the mesh origin.
			frame.Translation = Mask( ToMeshSpace( rRootPosition ) ) - frame.Rotation * m_StartPosition;

			return frame;
		}

		//
		// @returns the motion from rPrevious to rCurrent, after looping around the animation loopCount times.
		//
		RootMotionTransform CalculateDelta( const RootMotionTransform& rPrevious, const RootMotionTransform& rCurrent, uint32_t loopCount ) const
		{
			// Motion until the end of the animation, then any full loops, then the motion from the start until now.
			RootMotionTransform delta = rPrevious.Inverse();

			for( uint32_t i = 0; i < loopCount; ++i )
				delta *= m_FullCycle;

			delta *= rCurrent;

			return delta;
		}

		//
		// Takes the frame out of the root bone (local) so the mesh plays in place.
		//
		void RemoveFromRoot( const RootMotionTransform& rFrame, glm::vec3& rRootPosition, glm::quat& rRootRotation ) const
		{
			const RootMotionTransform inverseFrame = rFrame.Inverse();

			const glm::vec3 position = inverseFrame.Translation + inverseFrame.Rotation * ToMeshSpace( rRootPosition );
			const glm::quat rotation = inverseFrame.Rotation * ToMeshSpace( rRootRotation );

			rRootPosition = glm::vec3( m_InverseSkeletonTransform * glm::vec4( position, 1.0f ) );
			rRootRotation = glm::normalize( m_InverseSkeletonRotation * rotation );
		}

		// The total motion of one complete play through of the animation.
		const RootMotionTransform& GetFullCycle() const { return m_FullCycle; }
		const RootMotionSettings& GetSettings() const { return m_Settings; }

	private:
		glm::vec3 ToMeshSpace( const glm::vec3& rPosition ) const { return glm::vec3( m_SkeletonTransform * glm::vec4( rPosition, 1.0f ) ); }
		glm::quat ToMeshSpace( const glm::quat& rRotation ) const { return glm::normalize( m_SkeletonRotation * rRotation ); }

		// Only keep the components of the position that are being extracted.
		glm::vec3 Mask( const glm::vec3& rPosition ) const
		{
			glm::vec3 result( 0.0f );

			if( m_Settings.ExtractTranslation )
			{
				result.x = rPosition.x;
				result.z = rPosition.z;
			}

			if( m_Settings.ExtractVerticalTranslation )
				result.y = rPosition.y;

			return result;
		}

		// Returns the rotation around the up (Y) axis of the quaternion (swing-twist decomposition).
		static glm::quat ExtractYaw( const glm::quat& rRotation )
		{
			const glm::quat twist( rRotation.w, 0.0f, rRotation.y, 0.0f );
			const float length = glm::length( twist );

			// Rotation is a 180 degree swing, the twist is undefined.
			if( length < 1e-6f )
				return glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );

			return twist / length;
		}

	private:
		RootMotionSettings m_Settings;

		glm::mat4 m_SkeletonTransform = glm::mat4( 1.0f );
		glm::mat4 m_InverseSkeletonTransform = glm::mat4( 1.0f );
		glm::quat m_SkeletonRotation = glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );
		glm::quat m_InverseSkeletonRotation = glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );

		// Root bone at the start of the animation (mesh space), 
		// only the extracted components of the position are kept.
		glm::vec3 m_StartPosition = glm::vec3( 0.0f );
		glm::quat m_InverseStartRotation = glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );

		RootMotionTransform m_FullCycle;
	};
}
