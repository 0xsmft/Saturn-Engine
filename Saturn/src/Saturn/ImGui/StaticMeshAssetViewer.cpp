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
#include "StaticMeshAssetViewer.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include "Saturn/Core/Renderer/RenderThread.h"

// TOOD: #FixSceneRendererIncludes
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/AluraRenderer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Scene/Components.h"

#include "Saturn/Physics/PhysicsFoundation.h"

namespace Saturn {

	StaticMeshAssetViewer::StaticMeshAssetViewer( AssetID id )
		: AssetViewer( id ), SubSceneRendererWindow()
	{
		m_AssetType = AssetType::StaticMesh;

		Initialise();
		SetViewportWindowID( m_AssetID );

		AddMesh();
		m_Name = std::format( "{0}##{1}", m_Mesh->Name, std::to_string( m_AssetID ) );

	}

	StaticMeshAssetViewer::~StaticMeshAssetViewer()
	{
		m_Mesh = nullptr;
	}

	void StaticMeshAssetViewer::OnImGuiRender()
	{
		// Root Window.
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "StaticMeshDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		//////////////////////////////////////////////////////////////////////////

		if( ImGui::BeginMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Close" ) )
				{
					m_Open = false;
				}

				if( ImGui::MenuItem( "Save" ) )
				{
					StaticMeshAssetSerialiser sma;
					sma.Serialise( m_Mesh );
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		//////////////////////////////////////////////////////////////////////////

		RenderViewport();

		//////////////////////////////////////////////////////////////////////////

		ImGui::Begin( "Sidebar" );

		if( Auxiliary::TreeNode( "Physics" ) )
		{
			PhysicsShapeType type = m_Mesh->GetAttachedShape();
			
			constexpr const char* pItems[] = { "None", "Box", "Sphere", "Capsule", "Convex Mesh", "Triangle Mesh" };
			static PhysicsShapeType SelectedEnum = type;
			static const char* Selected = pItems[ ( int ) SelectedEnum ];

			ImGui::Text( "Select Physics Shape Type:" );
			ImGui::SameLine();

			if( ImGui::BeginCombo( "##setshape", Selected ) )
			{
				for( int i = 0; i < IM_ARRAYSIZE( pItems ); i++ )
				{
					bool IsSelected = ( Selected == pItems[ i ] );

					if( ImGui::Selectable( pItems[ i ], IsSelected ) ) 
					{
						SelectedEnum = ( PhysicsShapeType ) i;
						Selected = pItems[ i ];

						m_Mesh->SetAttachedShape( SelectedEnum );
					}

					if( IsSelected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if( SelectedEnum == PhysicsShapeType::TriangleMesh || SelectedEnum == PhysicsShapeType::ConvexMesh )
			{
				if( ImGui::Button( "Generate Mesh Collider" ) )
				{
					bool Result = PhysicsFoundation::Get()->GetCookingContext().CookMeshCollider( m_Mesh, SelectedEnum );

					// TODO: Show a dialog box of what failed.
				}
			}

			ImGui::Text( "Set Physics Material" );
			ImGui::SameLine();
			
			bool s_Open = false;

			if( ImGui::Button( "...##openmesh", ImVec2( 50.0f, 20.0f ) ) )
			{
				s_Open = !s_Open;
			}
			
			if( Auxiliary::DrawAssetFinder( AssetType::PhysicsMaterial, &s_Open, m_AssetFinderOutPhys, 0 ) )
			{
				m_Mesh->SetPhysicsMaterial( m_AssetFinderOutPhys );
			}

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Materials" ) )
		{
			int i = 0;
			for( auto& rMaterial : m_Mesh->GetMaterialAssets() )
			{
				ImGui::PushID( i );

				if( ImGui::TreeNodeEx( rMaterial->Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding ) )
				{
					ImGui::BeginHorizontal( i );

					ImGui::TextDisabled( "%s", rMaterial->Name.empty() ? "<NULL>" : rMaterial->Name.c_str() );

					bool open = false;

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
					{
						open = true;
					}

					ImGui::Spring();

					ImGui::EndHorizontal();

					if( Auxiliary::DrawAssetFinder( AssetType::Material, &open, m_AssetFinderOut, 0 ) )
					{
						Ref<MaterialAsset> newAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( m_AssetFinderOut );
						rMaterial->SetMaterial( newAsset->GetMaterial() );

						// Update Pure Dependencies & Update ADN Dependencies
						AssetManager::Get()->UnregisterAssetDependency( m_AssetID, rMaterial->ID );

						m_Mesh->GetMaterialRegistry()->SetMaterial( i, m_AssetFinderOut );

						AssetManager::Get()->RegisterAssetDependency( m_AssetID, m_AssetFinderOut );
					}

					Auxiliary::EndTreeNode();
				}

				ImGui::PopID();

				++i;
			}

			Auxiliary::EndTreeNode();
		}

		ImGui::End();
		ImGui::End(); // Root Window

		if( m_Open == false )
		{
			m_Open = false;

			RenderThread::Get().Queue( [ = ]()
			{
				m_SceneRenderer = nullptr;
			} );
		}
	}

	void StaticMeshAssetViewer::OnUpdate( Timestep ts )
	{
		OnUpdateRenderer( ts );
	}

	void StaticMeshAssetViewer::OnEvent( Event& rEvent )
	{
		OnCameraEvent( rEvent );
	}

	void StaticMeshAssetViewer::AddMesh()
	{
		m_Mesh = AssetManager::Get()->GetAssetAs<StaticMesh>( m_AssetID );
		m_Open = true;

		auto e = m_Scene->CreateEntity( "InternalViewerEntity" );
		e->AddComponent<StaticMeshComponent>( m_Mesh );
	}

}
