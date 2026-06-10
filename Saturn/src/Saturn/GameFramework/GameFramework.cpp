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
#include "GameFramework.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/Input.h"

#include "Saturn/Audio/AudioSystem.h"
#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Scene/Scene.h"

#include "SharedGlobals.h"

namespace Saturn {

	bool IptIsMouseLocked()
	{
		return Input::Get().GetCursorMode() == RubyCursorMode::Locked;
	}

	bool IptIsMouseButtonPressed( RubyMouseButton btn )
	{
		return Input::Get().MouseButtonPressed( btn );
	}

	bool IptIsKeyPressed( RubyKey key )
	{
		return Input::Get().KeyPressed( key );
	}

	glm::vec2 IptGetMousePosition()
	{
		return Input::Get().MousePosition();
	}

	Ref<Sound> AsRequestSound2D( AssetID ID )
	{
		return AudioSystem::Get().RequestNewSound( ID, UUID(), false );
	}

	Ref<Sound> AsPlaySound2D( AssetID ID )
	{
		return AudioSystem::Get().RequestNewSound( ID, UUID() );
	}

	Ref<Sound> AsPlaySoundAtLocation( AssetID ID, const glm::vec3& rPosition, bool PlayNow /*= true */ )
	{
		return AudioSystem::Get().PlaySoundAtLocation( ID, UUID(), rPosition, PlayNow );
	}

	Ref<GraphSound> AsPlayGraphSound( AssetID ID )
	{
		return AudioSystem::Get().PlayGraphSound( ID, UUID() );
	}

	void AsFireAndForget( AssetID ID )
	{
		AudioSystem::Get().FireAndForget( ID );
	}

	Ref<Asset> AmFindAsset( AssetID ID )
	{
		return AssetManager::Get()->FindAsset( ID );
	}

	template<typename Ty>
	Ref<Ty> AmGetAsset( AssetID ID )
	{
		return AssetManager::Get()->GetAssetAs<Ty>( ID );
	}

	void RcCloseApplication()
	{
#if !defined(SAT_DIST)
		g_ActiveScene->OnRuntimeEnd();
#else
		Application::Get()->Close();
#endif
	}

}
