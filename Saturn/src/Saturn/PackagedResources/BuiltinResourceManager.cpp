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
#include "BuiltinResourceManager.h"

#include "Saturn/Asset/Asset.h"

#include "Saturn/Vulkan/Texture.h"

namespace Saturn {

	BuiltinResourceManager::BuiltinResourceManager()
	{
		SAT_CORE_ASSERT( !SingletonStorage::GetSingleton<BuiltinResourceManager>(), "A BuiltinResourceManager already exists!" );

		SingletonStorage::AddSingleton( this );
	}

	BuiltinResourceManager::~BuiltinResourceManager()
	{
		SingletonStorage::RemoveSingleton( this );
	}

	Ref<Asset> BuiltinResourceManager::GetResource( PackagedResourceType type )
	{
		return nullptr;
	}

	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType( PackagedResourceType type )
	{
#if defined(SAT_DIST)
		return CreateResourceFromType_Dist( type );
#else
		return CreateResourceFromType_Development( type );
#endif
	}

#if defined(SAT_DIST)
	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType_Dist( PackagedResourceType type )
	{

	}
#else
	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType_Development( PackagedResourceType type )
	{
		switch( type )
		{
			case PackagedResourceType::DefaultAluraFont: 
			{
				return AluraCanvas::LoadEditorFont_ForEngineAB( Passkey<BuiltinResourceManager>() );
			}

			case PackagedResourceType::SMAA_AreaTexture: 
			{
				constexpr TextureLoadFlags loadFlags = TextureLoadFlags( TextureLoadFlags_LoadOnMainThread | TextureLoadFlags_NoMips );

				return Ref<Texture2D>::Create( "content/textures/SMAA_AreaTex.tga", AddressingMode::Repeat, loadFlags );
			}

			case PackagedResourceType::BRDFLutTexture:
				return Ref<Texture2D>::Create( "content/textures/BRDF_LUT.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			case PackagedResourceType::DefaultBloomDirtTexture:
				break;

			case PackagedResourceType::WeatherMapTexture:
				return Ref<Texture2D>::Create( "content/textures/CloudsWeatherMap.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			case PackagedResourceType::CurlTexture:
				return Ref<Texture2D>::Create( "content/textures/CloudsCurlNoise.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			case PackagedResourceType::Unknown:
			default:
				break;
		}

		SAT_CORE_VERIFY( false );
		return nullptr;
	}
#endif

}
