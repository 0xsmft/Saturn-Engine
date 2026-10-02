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
#include "ShaderAssetViewer.h"

#include "ShaderViewerWindow.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/ShaderAsset.h"

namespace Saturn {

	ShaderAssetViewer::ShaderAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::ShaderPrototype;

		AddShader();
	}

	ShaderAssetViewer::~ShaderAssetViewer()
	{
	}

	void ShaderAssetViewer::OnImGuiRender()
	{
		m_ShaderViewer->OnImGuiRender();

		if( !m_ShaderViewer->IsOpen() )
		{
			CloseWindow();
		}
	}

	void ShaderAssetViewer::OnUpdate( Timestep ts )
	{
	}

	void ShaderAssetViewer::OnEvent( Event& rEvent )
	{
	}

	void ShaderAssetViewer::AddShader()
	{
		m_ShaderAsset = AssetManager::Get()->GetAssetAs<ShaderAsset>( m_AssetID );
		m_ShaderViewer = Ref<ShaderViewerWindow>::Create( Project::GetActiveProject()->FilepathAbs( m_ShaderAsset->Path ) );
		m_ShaderViewer->OpenWindow();

		m_Name = std::format( "{0}##{1}", m_ShaderAsset->Name, ( uint64_t ) m_AssetID );
		m_Open = true;
	}

}
