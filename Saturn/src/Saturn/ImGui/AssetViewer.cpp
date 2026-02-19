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
#include "AssetViewer.h"

namespace Saturn {

//	static std::unordered_map<AssetID, AssetViewer*> s_AssetViewers;

	AssetViewer::AssetViewer( AssetID ID )
		: m_AssetID( ID )
	{
//		s_AssetViewers[ ID ] = this;
	}

	AssetViewer::~AssetViewer()
	{
	}

	/*
	void AssetViewer::Draw()
	{
		SAT_PF_EVENT();

		for( auto Itr = s_AssetViewers.begin(); Itr != s_AssetViewers.end(); )
		{
			if( Itr->second->IsOpen() )
			{
				Itr->second->OnImGuiRender();
				++Itr;
			}
			else
			{
				delete Itr->second;
				Itr = s_AssetViewers.erase( Itr );
			}
		}
	}

	void AssetViewer::Update( Timestep ts )
	{
		for( auto&& [id, viewer] : s_AssetViewers )
		{
			viewer->OnUpdate( ts );
		}
	}

	void AssetViewer::ProcessEvent( RubyEvent& rEvent )
	{
		for( auto&& [id, viewer] : s_AssetViewers )
		{
			viewer->OnEvent( rEvent );
		}
	}

	void AssetViewer::DestroyViewer( AssetID ID )
	{
	}

	void AssetViewer::Terminate()
	{
		for( auto&& [id, viewer] : s_AssetViewers )
		{
			if( viewer )
				delete viewer;
		}

		s_AssetViewers.clear();
	}
	*/
}