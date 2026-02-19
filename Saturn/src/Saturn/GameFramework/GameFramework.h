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

#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/GraphSound.h"

#include "Saturn/Core/Ruby/RubyEventType.h"

namespace Saturn {

	[[nodiscard]] extern bool IptIsMouseLocked();
	[[nodiscard]] extern bool IptIsMouseButtonPressed( RubyMouseButton btn );
	[[nodiscard]] extern bool IptIsKeyPressed( RubyKey key );
	[[nodiscard]] extern glm::vec2 IptGetMousePosition();

	extern Ref<Sound> AsPlaySound2D( AssetID ID, bool PlayNow = true );
	extern Ref<Sound> AsPlaySoundAtLocation( AssetID ID, const glm::vec3& rPosition, bool PlayNow = true );
	extern Ref<GraphSound> AsPlayGraphSound( AssetID ID );

	extern Ref<Asset> AmFindAsset( AssetID ID );
	
	template<typename Ty>
	extern Ref<Ty> AmGetAsset( AssetID ID );

	extern void RcCloseApplication();
}
