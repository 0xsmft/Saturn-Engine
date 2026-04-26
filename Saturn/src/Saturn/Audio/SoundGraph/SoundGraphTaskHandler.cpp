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
#include "SoundGraphTaskHandler.h"

#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/AudioSystem.h"

namespace Saturn {

	SoundGraphTaskHandler::SoundGraphTaskHandler()
	{
	}

	SoundGraphTaskHandler::~SoundGraphTaskHandler()
	{
		DestroyAliveSounds();
	}

	void SoundGraphTaskHandler::Tick( Timestep ts )
	{
		NodeEditorTaskHandler::Tick( ts );
	}

	void SoundGraphTaskHandler::PlaySounds()
	{
		size_t index = 0;
		for( auto itr = m_AliveSounds.begin(); itr != m_AliveSounds.end(); )
		{
			Ref<Sound> sound = *( itr );

			if( m_SoundsPlaying.count( index ) > 0 )
			{
				sound->Play();
				++itr;
			}
			else
			{
				sound->Unload();

				itr = m_AliveSounds.erase( itr );
			}

			++index;
		}
	}

	void SoundGraphTaskHandler::StopSounds()
	{
		for( auto& rSound : m_AliveSounds )
		{
			rSound->Stop();
		}
	}

	UUID SoundGraphTaskHandler::AddNewSound( UUID assetID, bool spatialisation /*= false */ )
	{
		Ref<Sound> snd;
		if( spatialisation )
		{
			snd = AudioSystem::Get().PlaySoundAtLocation( assetID, UUID(), { 0.0f, 0.0f, 0.0f }, false, nullptr );
		}
		else
		{
			snd = AudioSystem::Get().RequestNewSound( assetID, UUID(), false, nullptr );
		}

		snd->AddOnCompleteFunction( SAT_BIND_EVENT_FN( SoundGraphTaskHandler::OnSoundCompleted ) );
//		snd->WaitUntilLoaded();

		m_AliveSounds.push_back( snd );

		return m_AliveSounds.size() - 1;
	}

	void SoundGraphTaskHandler::OnSoundCompleted( UUID PlayerID )
	{
		auto Itr = std::find_if( m_AliveSounds.begin(), m_AliveSounds.end(),
			[ PlayerID ]( const auto& rSound )
		{
			return rSound->GetPlayerID() == PlayerID;
		} );

		if( Itr != m_AliveSounds.end() )
		{
			m_AliveSounds.erase( Itr );
		}

		m_Completed = m_AliveSounds.empty();
		if( m_Completed && m_Looping )
		{
			m_Completed = false;

			m_SoundsPlaying.clear();

			ResetAllTasks();
		}
	}

	void SoundGraphTaskHandler::DestroyAliveSounds()
	{
		for( auto& rSound : m_AliveSounds )
		{
			rSound->Stop();
			AudioSystem::Get().UnloadSound( rSound );
		}

		m_AliveSounds.clear();
		m_SoundsPlaying.clear();
	}

	void SoundGraphTaskHandler::RegisterSound( size_t index )
	{
		m_SoundsPlaying.insert( index );
	}

	void SoundGraphTaskHandler::UnregisterSound( size_t index )
	{
		m_SoundsPlaying.erase( index );
	}

	Ref<Sound> SoundGraphTaskHandler::GetSoundFromIndex( size_t index )
	{
		if( index >= m_AliveSounds.size() )
			return nullptr;
	
		return m_AliveSounds[ index ];
	}

}
