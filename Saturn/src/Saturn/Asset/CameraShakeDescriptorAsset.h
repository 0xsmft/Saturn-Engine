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

#include "Asset.h"

namespace Saturn {

	class CameraShakeDescriptorAsset : public Asset
	{
	public:
		CameraShakeDescriptorAsset() = default;
		CameraShakeDescriptorAsset( const Ref<Asset>& rOther ) 
			: Asset( rOther )
		{
		}

		virtual ~CameraShakeDescriptorAsset() = default;

	public:
		float GetTrauma() const { return m_Trauma; }
		float GetDecaySpeed() const { return m_DecaySpeed; }

		float GetMaxPitchInDeg() const { return m_MaxPitchDeg; }
		float GetMaxRollInDeg() const { return m_MaxRollDeg; }
		float GetMaxYawInDeg() const { return m_MaxYawDeg; }

		float GetMaxPitchInRad() const { return glm::radians( m_MaxPitchDeg ); }
		float GetMaxRollInRad() const { return glm::radians( m_MaxRollDeg ); }
		float GetMaxYawInRad() const { return glm::radians( m_MaxYawDeg ); }

	private:
		float m_Trauma = 0.0f;
		float m_DecaySpeed = 1.0f;
		float m_MaxPitchDeg = 5.0f;
		float m_MaxYawDeg = 5.0f;
		float m_MaxRollDeg = 8.0f;

	private:
		friend class CameraShakeDescriptorAssetSerialiser;
	};
	
}
