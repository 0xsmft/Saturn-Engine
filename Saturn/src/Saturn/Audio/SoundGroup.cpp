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

#include "sppch.h"
#include "SoundGroup.h"

#include "AudioSystem.h"

namespace Saturn {

	SoundGroup::SoundGroup( const std::string& rName )
		: m_Name( rName )
	{
	}

	SoundGroup::SoundGroup()
	{
	}

	SoundGroup::~SoundGroup()
	{
		Destroy();
	}

	void SoundGroup::Destroy()
	{
		if( m_Group )
		{
			ma_sound_group_uninit( m_Group );
			delete m_Group;

			m_Group = nullptr;
		}
	}

	float SoundGroup::GetVolume() const
	{
		if( m_Group )
			return ma_sound_group_get_volume( m_Group );

		return 0.0f;
	}

	float SoundGroup::GetPitch() const
	{
		if( m_Group )
			return ma_sound_group_get_pitch( m_Group );

		return 0.0f;
	}

	void SoundGroup::SetVolume( float volume )
	{
		if( m_Group )
			ma_sound_group_set_volume( m_Group, volume );
	}

	void SoundGroup::SetPitch( float pitch )
	{
		if( m_Group )
			ma_sound_group_set_pitch( m_Group, pitch );
	}

	void SoundGroup::Start()
	{
		if( m_Group )
			ma_sound_group_start( m_Group );
	}

	void SoundGroup::Stop()
	{
		if( m_Group )
			ma_sound_group_stop( m_Group );
	}

	void SoundGroup::Init( bool isMaster )
	{
		m_Group = new ma_sound();

		ma_sound_group* pParentSoundGroup = isMaster ? nullptr : AudioSystem::Get().GetMasterSoundGroup()->GetInternal();

		MA_CHECK( ma_sound_group_init( AudioSystem::Get().GetAudioEngine(), 0, pParentSoundGroup, m_Group ) );
	}

}