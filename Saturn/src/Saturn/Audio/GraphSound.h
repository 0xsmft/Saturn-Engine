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

#include "SoundGroup.h"
#include "SoundBase.h"
#include "Saturn/Asset/Asset.h"

namespace Saturn {

	class NodeEditor;
	class NodeEditorBase;
	class SoundEditorEvaluator;

	class GraphSound : public SoundBase
	{
	public:
		GraphSound( AssetID id );
		~GraphSound();

		void Initialise();
		virtual void Play( int frameOffset = 0 ) override;

		void Stop() override;
		void Loop( bool loop = true ) override;
		void Load( uint32_t flags ) override;
		void Reset() override;

		// Public non-static version of OnSoundEnd
		// OnSoundCompleted is called by the Audio System
		void OnSoundCompleted() override;

		void WaitUntilLoaded();

		void SetVolume( float volume );
		void SetPitch( float pitch );

		void SetPosition( const glm::vec3& rPos );
		void SetSpatialisation( bool value );

#if !defined(SAT_DIST)
		SharedPtr<NodeEditor> GetNodeEditor() const;
#endif

	private:
		void Unload() override;

	private:
		// The "GraphSound" class is not an asset however GraphSounds are an asset
		Ref<Asset> m_GraphAsset;
		Ref<SoundGroup> m_SoundGroup;
		Ref<SoundEditorEvaluator> m_Runtime;

#if !defined(SAT_DIST)
		SharedPtr<NodeEditor> m_NodeEditor;
#else
		SharedPtr<NodeEditorBase> m_NodeEditor;
#endif
		UUID m_OutputNodeID = 0;
	};
}
