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
#include "GraphSound.h"

#include "Sound.h"
#include "SoundGraph/SoundGraphTaskHandler.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/NodeEditor/GlobalNodeEditorTaskCache.h"

namespace Saturn {

	GraphSound::GraphSound( AssetID id )
		: m_GraphAsset( AssetManager::Get()->FindAsset( id ) )
	{
	}

	GraphSound::~GraphSound()
	{
		m_SoundGroup = nullptr;
	}

	void GraphSound::Initialise()
	{
		m_SoundState = SoundState::Initialising;

		if( m_Loaded )
			return;

		m_TaskHandler = Ref<SoundGraphTaskHandler>::Create();

		// Try to load without touching the disk...
		auto& rCache = GlobalNodeEditorTaskCache::Get().GetOrCreateTaskCache( m_GraphAsset->ID );
		if( rCache.IsListEmpty() )
		{
			// ...otherwise load it from disk.
			const std::string filename = std::format( "{0}.gsnd", m_GraphAsset->Name );

			if( !NodeCacheEditor::ReadNodeTaskCacheOnly( rCache, m_GraphAsset->ID, filename ) )
			{
				m_SoundState = SoundState::NoDataSource;
				return;
			}
		}

		m_TaskHandler->Init( rCache );
	}

	void GraphSound::Play( uint64_t frameOffset )
	{
		if( m_SoundState != SoundState::Playing )
		{
			m_TaskHandler->PlaySounds();

			m_Loaded = true;
			m_SoundState = SoundState::Playing;
		}
	}

	void GraphSound::Stop()
	{
		m_TaskHandler->StopSounds();

		m_SoundState = SoundState::Stopped;
	}

	void GraphSound::Loop( bool loop )
	{
		m_TaskHandler->Loop( loop );
		m_Looping = loop;
	}

	void GraphSound::Load( uint32_t flags )
	{
		Initialise();
	}

	void GraphSound::Reset()
	{
		/*
		for( auto& rSound : m_Runtime->AliveSounds )
		{
			rSound->Reset();
		}
		*/
	}

	void GraphSound::OnSoundCompleted()
	{

	}

	void GraphSound::WaitUntilLoaded()
	{
		while( !m_Loaded )
		{
			std::this_thread::yield();
		}
	}

	void GraphSound::SetVolume( float volume )
	{
		m_TaskHandler->SetVolume( volume );
	}

	void GraphSound::SetPitch( float pitch )
	{
		m_TaskHandler->SetPitch( pitch );
	}

	void GraphSound::SetPosition( const glm::vec3& rPos )
	{
		m_TaskHandler->SetPosition( rPos );
	}

	void GraphSound::SetSpatialisation( bool value )
	{
		m_TaskHandler->SetSpatialisation( value );
	}

	void GraphSound::Unload()
	{
		m_TaskHandler->DestroyAliveSounds();

		m_Loaded = false;
		m_SoundState = SoundState::NoDataSource;
	}

}
