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

#include "Saturn/Asset/Asset.h"
#include "Saturn/Core/Ruby/RubyEvent.h"

#include <unordered_map>

namespace Saturn {

	class AssetViewer : public RefTarget
	{
	public:
		AssetViewer() = default;
		AssetViewer( AssetID ID );
		virtual ~AssetViewer();

		virtual void OnImGuiRender() = 0;

		// This function is to only be used when a asset viewer has a SceneRenderer as this will be called after the main renderer is complete.
		virtual void OnUpdate( Timestep ts ) = 0;

		virtual void OnEvent( RubyEvent& rEvent ) = 0;

		bool IsOpen() { return m_Open; }

		void MarkDirty() { m_Dirty = true; }
		void MarkClean() { m_Dirty = false; }

	public: 
		// Static API

		template<typename Ty, typename... Args>
		static Ty* Add( Args&&... rrArgs ) 
		{
			Ty* assetViewer = new Ty( std::forward<Args>( rrArgs )... );
			return assetViewer;
		}

		static void Draw();
		static void Update( Timestep ts );
		static void ProcessEvent( RubyEvent& rEvent );
		static void DestroyViewer( AssetID ID );
		static void Terminate();

	protected:
		AssetType m_AssetType = AssetType::Unknown;
		AssetID m_AssetID;
		bool m_Open = false;
		bool m_Dirty = false;
	};
}