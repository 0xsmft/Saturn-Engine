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
#include "EditorIcons.h"

#if defined(SAT_DIST)
#include <stacktrace>
#endif

namespace Saturn {

#if !defined( SAT_DIST )
	static std::unordered_map<std::string, Ref<Texture2D>> s_Textures;
#endif

	Ref<Texture2D> EditorIcons::GetIcon( const std::string& rName )
	{
#if !defined( SAT_DIST )
		const auto Itr = s_Textures.find( rName );

		return Itr == s_Textures.end() ? nullptr : Itr->second;
#else
		std::string message = std::format( "EditorIcons::GetIcon should not be called in Dist! Please check the stacktrace below and debug!\n{0}", std::stacktrace::current() );
		SAT_CORE_VERIFY( false, message );
		return nullptr;
#endif
	}

	void EditorIcons::AddIcon( const Ref<Texture2D>& rTexture )
	{
#if !defined( SAT_DIST )
		const std::string name = rTexture->GetPath().stem().string();
		s_Textures[ name ] = rTexture;
#endif
	}

	void EditorIcons::Clear()
	{
#if !defined( SAT_DIST )
		s_Textures.clear();
#endif
	}
}
