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

#include "sppch.h"
#include "AnimGraphAnimationPin.h"

#include "AnimGraph.h"
#include "AnimGraphStateMachinePlayerNode.h"

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AnimGraphAnimationPin::AnimGraphAnimationPin( const std::string& rName, PinKind kind, AnimGraphAnimationPinFlags flags )
		: Pin( rName, PinType::AnimGraphAnimation, kind ), m_Flags( flags )
	{
	}

	AnimGraphAnimationPin::AnimGraphAnimationPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	AnimGraphAnimationPin::~AnimGraphAnimationPin()
	{
	}

	void AnimGraphAnimationPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		
		RawSerialisation::WriteObject( m_Flags, rStream );
		RawSerialisation::WriteUUID( m_AssetID, rStream );
	}

	void AnimGraphAnimationPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadObject( m_Flags, rStream );
		RawSerialisation::ReadUUID( m_AssetID, rStream );

#if !defined(SAT_DIST)
		Ref<Asset> asset = AssetManager::Get()->FindAsset( m_AssetID );
		if( asset )
		{
			m_AssetName = asset->Name;
		}
		else
		{
			m_AssetID = 0llu;
		}
#endif
	}

	void AnimGraphAnimationPin::OnRenderOutput()
	{
#if !defined(SAT_DIST)
		switch( m_Flags )
		{
			case AnimGraphAnimationPinFlags::StateMachine:
			{
				if( ImGui::Button( "Open State Machine" ) ) 
				{
					auto AG = dynamic_cast<AnimGraph*>( Node->GetParentObject() );
					if( AG )
					{
						auto playerNode = dynamic_cast< AnimGraphStateMachinePlayerNode* >( Node.Get() );
						AG->AddSubGraph( Node );
						AG->ChangeEditorNextFrame( Node );
					}
				}
			} break;

			case AnimGraphAnimationPinFlags::Animation: 
			{
				bool openAssetIDPopup = false;

				const std::string name = m_AssetID == 0 ? "Select Asset" : m_AssetName;
				if( ImGui::Button( name.c_str() ) )
				{
					openAssetIDPopup = true;
				}

				ed::Suspend();

				UUID tempID = m_AssetID;
				if( Auxiliary::DrawAssetFinder( AssetType::SkeletalAnimation, &openAssetIDPopup, tempID ) )
				{
					auto AG = dynamic_cast< AnimGraph* >( Node->GetParentObject() );

					AssetManager::Get()->UnregisterAssetDependency( AG->GetAssetID(), m_AssetID );

					m_AssetName = AssetManager::Get()->FindAsset( tempID )->Name;
					m_AssetID = tempID;

					AssetManager::Get()->RegisterAssetDependency( AG->GetAssetID(), m_AssetID );

					if( AG )
					{
						AG->MarkDirty();
					}
				}

				ed::Resume();
			} break;

			default: break;
		}
#endif
	}

}
