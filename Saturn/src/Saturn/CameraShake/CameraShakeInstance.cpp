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

#include "sppch.h"
#include "CameraShakeInstance.h"

#include "CameraShakeDescriptorAsset.h"

#include "Saturn/Core/Random.h"

namespace Saturn {

	CameraShakeInstance::CameraShakeInstance( Ref<CameraShakeDescriptorAsset> asset )
	{
		SAT_CORE_ASSERT( asset, "Null CameraShakeDescriptorAsset specified into CameraShakeInstance!" );

		m_Trauma = asset->GetTrauma();
		m_CurrentTrauma = m_Trauma;
		m_DecaySpeed = asset->GetDecaySpeed();

		m_MaxPitchRad = asset->GetMaxPitchInRad();
		m_MaxYawRad   = asset->GetMaxYawInRad();
		m_MaxRollRad  = asset->GetMaxRollInRad();
	}

	CameraShakeInstance::CameraShakeInstance( float trauma, float decaySpeed, float maxPitchDeg, float maxYawDeg, float maxRollDeg )
	{
		m_Trauma = trauma;
		m_CurrentTrauma = m_Trauma;
		m_DecaySpeed = decaySpeed;

		m_MaxPitchRad = glm::radians( maxPitchDeg );
		m_MaxYawRad   = glm::radians( maxYawDeg );
		m_MaxRollRad  = glm::radians( maxRollDeg );
	}

	CameraShakeInstance::~CameraShakeInstance()
	{
	}

	// TODO: This is not great and should be replaced with a perlin noise function.
	static float RandValue()
	{
		return Random::RandomFloatInRange( -1.0f, 1.0f );
	}

	glm::quat CameraShakeInstance::Tick( Timestep ts )
	{
		m_CurrentTrauma = glm::max( 0.0f, m_CurrentTrauma - m_DecaySpeed * ts.Seconds() );

		const float shake = m_CurrentTrauma * m_CurrentTrauma;
		const float pitchOffset = m_MaxPitchRad * shake * RandValue();
		const float yawOffset = m_MaxYawRad * shake * RandValue();
		const float rollOffset = m_MaxRollRad * shake * RandValue();

		m_Completed = ( m_CurrentTrauma == 0.0f );

		const glm::vec3 angles( glm::radians( pitchOffset ), glm::radians( yawOffset ), glm::radians( rollOffset ) );
		return glm::quat( angles );
	}

	void CameraShakeInstance::Reset()
	{
		m_CurrentTrauma = m_Trauma;
	}

}
