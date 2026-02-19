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
#include "AssetIDPin.h"

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AssetIDPin::AssetIDPin( const std::string& rName, PinKind kind, AssetType assetType )
		: Pin( rName, PinType::AssetID, kind ), m_AssetType( assetType )
	{
	}

	AssetIDPin::AssetIDPin( UUID ID, const std::string& rName, PinType type, UUID nodeid )
		: Pin( ID, rName, PinType::AssetID, nodeid )
	{
	}

	AssetIDPin::~AssetIDPin()
	{
	}

	void AssetIDPin::OnRenderOutput()
	{
		RenderInternal();
	}

	void AssetIDPin::OnRenderInput()
	{
		RenderInternal();
	}

	void AssetIDPin::RenderInternal()
	{
		// This function won't be called on Dist anyways
#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		bool openAssetIDPopup = false;

		std::string name = m_AssetID == 0 ? "Select Asset" : m_AssetName;
		if( ImGui::Button( name.c_str() ) )
		{
			openAssetIDPopup = true;
		}

		ed::Suspend();
		if( Auxiliary::DrawAssetFinder( m_AssetType, &openAssetIDPopup, m_AssetID ) ) 
		{
			m_AssetName = AssetManager::Get()->FindAsset( m_AssetID )->Name;

			NodeEditorBase* pOuter = dynamic_cast< NodeEditorBase* >( Node->GetParentObject() );

			if( pOuter && pOuter->GetAssetID() != 0 )
			{
				// NOTE: MaterialOutputNode::EvaluateNode does actaully do this as well but we keep this here for two reasons
				// One: AssetIDPin class is used by other classes that may not do what MaterialOutputNode::EvaluateNode does
				// Two: MaterialOutputNode only sets the dependencies if we evaluate.
				SAT_CORE_INFO( "[AssetIDPin] Registering Asset Dependency via AssetIDPin!" );
				AssetManager::Get()->RegisterAssetDependency( pOuter->GetAssetID(), m_AssetID );
			}
		}
		ed::Resume();
#endif
	}

	void AssetIDPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );

		RawSerialisation::WriteObject( m_AssetID, rStream );
	}

	void AssetIDPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadObject( m_AssetID, rStream );
#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		Ref<Asset> foundAsset = AssetManager::Get()->FindAsset( m_AssetID );
		m_AssetName = foundAsset == nullptr ? "No Asset" : foundAsset->Name;
#endif
	}

}
