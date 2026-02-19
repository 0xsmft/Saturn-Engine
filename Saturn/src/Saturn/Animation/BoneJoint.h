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

#include "Saturn/Core/Ref.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace Saturn {

//	class SkeletalMesh;

	class BoneJoint : public RefTarget
	{
	public:
		BoneJoint();
		BoneJoint( const std::string& rBoneName, const std::string& rName );
		virtual ~BoneJoint();

	public:
		[[nodiscard]] const glm::vec3 GetRelativePosition() const { return m_Position; }
		[[nodiscard]] const glm::quat GetRelativeRotation() const { return m_Rotation; }
		[[nodiscard]] const glm::vec3 GetRelativeScale()    const { return m_Scale; }
		[[nodiscard]] const std::string& GetBoneName()      const { return m_BoneName; }
		[[nodiscard]] const std::string& GetName()          const { return m_Name; }

		void SetRelativePosition( const glm::vec3& rPosition )    { m_Position = rPosition; }
		void SetRelativeRotation( const glm::vec3& rEulerAngles ) { m_Rotation = glm::quat( rEulerAngles ); }
		void SetRelativeScale( const glm::vec3& rScale )         { m_Scale = rScale; }

		glm::mat4 GetBoneMatrix( Ref<class Animator> animator ) const;
		glm::mat4 GetBoneMatrixPreview( Ref<class SkeletalMesh> animator ) const;

	private:
		std::string m_BoneName;
		std::string m_Name;

		glm::vec3 m_Position{};
		glm::quat m_Rotation{};
		glm::vec3 m_Scale{ 1.0f, 1.0f, 1.0f };
	
	private:
		friend class SkeletonBoneHierarchyPanel;
	};
	
}