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
#include "SceneHierarchyPanel.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include "Saturn/Core/App.h"

#include "Saturn/Vulkan/Mesh.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/Scene/Entity.h"

#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/Asset/Prefab.h"

#include "Saturn/Physics/PhysicsRigidBody.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Vulkan/VulkanContext.h"

#include "UndoRedo/GlobalUndoRedoGroup.h"
#include "UndoRedo/EntityUndoRedoActions.h"

#include "Saturn/AI/Navigation/NavBoundsEntity.h"
#include "Saturn/AI/AIAgentEntity.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

//#include <imgui.h>
#include <imgui_internal.h>

namespace Saturn {

	SceneHierarchyPanel::SceneHierarchyPanel() 
		: ImGuiWindow( "Scene Hierarchy Panel" ), m_EditIcon( Ref<Texture2D>::Create( "content/textures/editor/EditIcon.png", AddressingMode::Repeat, false ) )
	{
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
		m_EditIcon = nullptr;
		m_CopyComponentData.Buffer.Free();
	}

	void SceneHierarchyPanel::SetContext( const Ref<Scene>& scene )
	{
		m_Context = scene;
		m_SelectionContexts.clear();
	}

	void SceneHierarchyPanel::SetSelected( Ref<Entity> entity )
	{
		if( !m_IsMultiSelecting )
			ClearSelection();

		m_SelectionContexts.push_back( entity );
		m_Context->AddSelectedEntity( entity );
	}

	void SceneHierarchyPanel::DrawEntities()
	{
		m_Context->Each( [&]( Ref<Entity> entity )
			{
				if( !entity->HasParent() )
					DrawEntityNode( entity );
			} );
	}

	void SceneHierarchyPanel::ClearSelection()
	{
		m_SelectionContexts.clear();
		m_Context->ClearSelectedEntities();
	}

	template<typename Ty>
	void SceneHierarchyPanel::DrawAddComponents( const char* pName, Ref<Entity> entity )
	{
		if( !m_SelectionContexts[ 0 ]->HasComponent<Ty>() )
		{
			if( ImGui::Button( pName ) )
			{
				m_SelectionContexts[ 0 ]->AddComponent<Ty>();

				Ref<UndoRedoActionAddComponent<Ty>> action = Ref<UndoRedoActionAddComponent<Ty>>::Create( entity );
				GlobalUndoRedoGroup::Get().AddAction( action, ( uint64_t ) entity->GetHandle() );

				ImGui::CloseCurrentPopup();
			}
		}
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::PushID( static_cast<int>( m_CustomID == 0 ? m_Context->ID : m_CustomID ) );

		if( !m_IsPrefabScene )
			ImGui::Begin( m_WindowName.c_str(), &m_Open );

		if( m_Context )
		{	
			if( m_EntityTextFilter.DrawWithHint( "##schpanelfinder", "Search...", ImGui::GetContentRegionAvail().x ) )
				m_Searching = m_EntityTextFilter.IsActive();

			ImGui::Separator();

			DrawEntities();

			if( ImGui::IsMouseDown( 0 ) && ImGui::IsWindowHovered() && !m_IsMultiSelecting )
			{
				ClearSelection();
			}

			if( ImGui::BeginPopupContextWindow( 0, ImGuiPopupFlags_MouseButtonRight ) )
			{
				if( ImGui::MenuItem( "Create Empty Entity" ) )
				{
					SetSelected( m_Context->CreateEntity( "Unnamed Entity" ) );
					m_Context->MarkDirty();
				}

				auto directionalLights = m_Context->m_Registry.view<DirectionalLightComponent>();
				if( directionalLights.empty() )
				{
					if( ImGui::MenuItem( "Directional Light" ) )
					{
						Ref<Entity> entity = m_Context->CreateEntity( "Directional Light" );

						entity->AddComponent<DirectionalLightComponent>();
						entity->GetComponent<TransformComponent>().SetRotation( glm::radians( glm::vec3( 80.0f, 10.0f, 0.0f ) ) );

						SetSelected( entity );
						m_Context->MarkDirty();
					}
				}

				auto SkylightComponents = m_Context->m_Registry.view<SkylightComponent>();
				if( SkylightComponents.empty() )
				{
					if( ImGui::MenuItem( "Skylight" ) )
					{
						auto entity = m_Context->CreateEntity( "Skylight" );
						entity->AddComponent<SkylightComponent>();
						
						// Defaults
						Application::Get().PrimarySceneRenderer().SetDynamicSky( 2.0f, 0.0f, 0.0f );

						SetSelected( entity );
						m_Context->MarkDirty();
					}
				}

				auto navBounds = m_Context->m_Registry.view<NavigationMeshSpecificationComponent>();
				if( navBounds.empty() )
				{
					if( ImGui::MenuItem( "Create Navigation Bounds" ) )
					{
						auto navEntity = m_Context->CreateEntityScript<NavBoundsEntity>( "NavBoundsEntity", "Navigation Bounds" );
						navEntity->GatherGeometryAndBuild();

						SetSelected( navEntity );
						m_Context->MarkDirty();
					}
				}

				if( ImGui::MenuItem( "Create AI Agent" ) )
				{
					auto agent = m_Context->CreateEntityScript<AIAgentEntity>( "AIAgentEntity", "AI Agent" );
					
					SetSelected( agent );
					m_Context->MarkDirty();
				}
				
				ImGui::EndPopup();
			}

			std::string name = "Inspector##" + m_WindowName;
			ImGui::Begin( name.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse );

			if( m_SelectionContexts.size() )
			{
				DrawComponents( m_SelectionContexts.front() );
			}

			ImGui::End();
		}

		if( !m_IsPrefabScene )
			ImGui::End();

		if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) || Input::Get().KeyPressed( RubyKey_RightCtrl ) )
		{
			m_IsMultiSelecting = true;
		}
		else
		{
			m_IsMultiSelecting = false;
		}

		ImGui::PopID();
	}

	void SceneHierarchyPanel::DrawComponents( Ref<Entity> entity )
	{
		DrawEntityComponents( m_SelectionContexts[ 0 ] );

		if( ImGui::Button( "Add Component" ) )
			ImGui::OpenPopup( "AddComponentPanel" );

		if( ImGui::BeginPopup( "AddComponentPanel" ) )
		{
			DrawAddComponents<StaticMeshComponent>( "Static Mesh", m_SelectionContexts[ 0 ] );

			DrawAddComponents<CameraComponent>( "Camera", m_SelectionContexts[ 0 ] );

			DrawAddComponents<PointLightComponent>( "Point Light", m_SelectionContexts[ 0 ] );

			DrawAddComponents<DirectionalLightComponent>( "Directional Light", m_SelectionContexts[ 0 ] );

			DrawAddComponents<BoxColliderComponent>( "Box Collider", m_SelectionContexts[ 0 ] );

			DrawAddComponents<SphereColliderComponent>( "Sphere Collider", m_SelectionContexts[ 0 ] );

			DrawAddComponents<CapsuleColliderComponent>( "Capsule Collider", m_SelectionContexts[ 0 ] );

			DrawAddComponents<RigidbodyComponent>( "Rigidbody", m_SelectionContexts[ 0 ] );

			DrawAddComponents<BillboardComponent>( "Billboard", m_SelectionContexts[ 0 ] );

			DrawAddComponents<AudioPlayerComponent>( "Audio Player", m_SelectionContexts[ 0 ] );
			
			DrawAddComponents<AudioListenerComponent>( "Audio Listener", m_SelectionContexts[ 0 ] );

			ImGui::EndPopup();
		}
	}

	bool SceneHierarchyPanel::IsEntitySelected( Ref<Entity> entity )
	{
		return std::find( m_SelectionContexts.begin(), m_SelectionContexts.end(), entity ) != m_SelectionContexts.end();
	}

	void SceneHierarchyPanel::DrawEntityNode( Ref<Entity> entity )
	{
		if( entity->HasComponent<TagComponent>() )
		{
			const auto& rTag = entity->GetComponent<TagComponent>().Tag;
			const bool isPrefab = entity->HasComponent<PrefabComponent>();

			if( m_Searching && !m_EntityTextFilter.PassFilter( rTag.c_str() ) )
				return;

			ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			IsEntitySelected( entity ) ? Flags |= ImGuiTreeNodeFlags_Selected : 0;

			bool Clicked;

			if( isPrefab )
				ImGui::PushStyleColor( ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4( IM_COL32( 255, 179, 0, 255 ) ) );

			Clicked = ImGui::TreeNodeEx( (void*)entity.Get(), Flags, rTag.c_str() );

			if( isPrefab )
				ImGui::PopStyleColor();

			if( ImGui::IsItemClicked( ImGuiMouseButton_Left ) || ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
			{
				SetSelected( entity );

				if( m_SelectionChangedCallback )
					m_SelectionChangedCallback( entity );
			}

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::BeginHorizontal( (void*)entity.Get() );
				ImGui::Text( "%s | %s", rTag.c_str(), entity->GetClass()->GetName().c_str() );
				ImGui::Spring();
				ImGui::Text( "ECS Handle: %i", entity->GetHandle() );
				ImGui::Spring();
				ImGui::EndHorizontal();

				ImGui::EndTooltip();
			}

			if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID ) ) 
			{
				ImGui::Text( rTag.c_str() );

				ImGui::SetDragDropPayload( "ENTITY_PARENT_SCHPANEL", entity.Get(), sizeof( Entity ), ImGuiCond_Once );

				ImGui::EndDragDropSource();
			}

			if( ImGui::BeginDragDropTarget() )
			{
				const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload( "ENTITY_PARENT_SCHPANEL" );
				if( pPayload )
				{
					Ref<Entity> e = (Entity*)pPayload->Data;
					Ref<Entity> previousParent = m_Context->FindEntityByID( e->GetParent() );

					// If a child is trying to parent it's parent.
					bool ParentToParent = false;
					for( auto& child : e->GetChildren() )
					{
						if( child == entity->GetUUID() )
						{
							ParentToParent = true;
							break;
						}
					}

					if( !ParentToParent )
					{
						if( previousParent )
						{
							auto& children = previousParent->GetChildren();
							children.erase( std::remove( children.begin(), children.end(), e->GetComponent<IdComponent>().ID ), children.end() );
						}

						e->SetParent( entity->GetComponent<IdComponent>().ID );

						auto& children = entity->GetChildren();
						children.push_back( e->GetComponent<IdComponent>().ID );
					}
				}

				ImGui::EndDragDropTarget();
			}

			if( Clicked ) 
			{
				for ( auto& child : entity->GetChildren() )
				{
					Ref<Entity> e = m_Context->FindEntityByID( child );
					if( e )
						DrawEntityNode( e );
				}

				ImGui::TreePop();
			}
		}
	}

	void SceneHierarchyPanel::DrawEntityProperties( Ref<Entity> entity ) 
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_AllowItemOverlap;

		const bool hasScript = entity->HasComponent<DScriptComponent>();

		if( hasScript )
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		// Draw properties
		if( ImGui::TreeNodeEx( ( void* ) ( ( uint32_t ) entity ), flags, "Properties" ) )
		{
			if( hasScript )
			{
				auto propCount = entity->GetClass()->GetPropertyCount();
				auto properties = entity->GetClass()->GetProperties();

				for( int i = 0; i < propCount; i++ )
				{
					SProperty* pProperty = ( SProperty* ) properties[ i ];

					std::string name = pProperty->GetName();

					if( pProperty->IsDirty() )
					{
						name += "*";
					}

					switch( pProperty->GetType() )
					{
						case SPropertyType::Float:
						{
							float temporaryValue = pProperty->Read<SPropertyType::Float>( entity.Get() );

							if( Auxiliary::DrawFloatControl( name, temporaryValue ) )
								pProperty->SetProperty( entity.Get(), temporaryValue );
						} break;

						case SPropertyType::Int:
						{
							auto temporaryValue = pProperty->Read<SPropertyType::Int>( entity.Get() );
							if( Auxiliary::DrawIntControl( name, temporaryValue ) )
								pProperty->SetProperty( entity.Get(), temporaryValue );
						} break;

						case SPropertyType::Double:
						{
							auto temporaryValue = pProperty->Read<SPropertyType::Double>( entity.Get() );
							if( Auxiliary::DrawDoubleControl( name, temporaryValue ) )
								pProperty->SetProperty( entity.Get(), temporaryValue );
						} break;

						case SPropertyType::Vector2:
						{
							glm::vec2 value = pProperty->Read<SPropertyType::Vector2>( entity.Get() );

							if( Auxiliary::DrawVec2Control( name, value, 0.0f, true, 125.0f ) )
								pProperty->SetProperty( entity.Get(), value );
						} break;

						case SPropertyType::Vector3:
						{
							glm::vec3 value = pProperty->Read<SPropertyType::Vector3>( entity.Get() );

							if( Auxiliary::DrawVec3Control( name, value, 0.0f, true, 125.0f ) )
								pProperty->SetProperty( entity.Get(), value );
						} break;

						case SPropertyType::EntityType:
						{
							Ref<Entity>& entityFromProp = pProperty->Read<SPropertyType::EntityType>( entity.Get() );

							ImGui::PushID( name.c_str() );

							ImGui::Columns( 2 );
							ImGui::SetColumnWidth( 0, 125.0f );
							ImGui::Text( name.c_str() );
							ImGui::NextColumn();

							ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

							if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
							{
								m_OpenEntityFinderPopup = true;
							}

							ImGui::SameLine();
							ImGui::Text( !entityFromProp ? " <no class set>" : entityFromProp->GetName().c_str() );

							ImGui::PopItemWidth();
							ImGui::PopStyleVar();

							ImGui::Columns( 1 );

							ImGui::PopID();

							if( m_OpenEntityFinderPopup )
							{
								ImGui::OpenPopup( "EntityFinderPopup" );

								ImGui::SetNextWindowSize( { 250.0f, 0.0f } );
								if( ImGui::BeginPopup( "EntityFinderPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
								{
									bool PopupModified = false;

									if( ImGui::BeginListBox( "##ENTITYLIST", ImVec2( -FLT_MIN, 0.0f ) ) )
									{
										bool Selected = false;

										m_Context->Each( [ & ]( Ref<Entity>& rEntity )
										{
											if( ImGui::Selectable( rEntity->GetComponent<TagComponent>().Tag.c_str(), &Selected ) )
											{
												pProperty->SetProperty( entity.Get(), rEntity );
												PopupModified = true;
											}
										} );

										if( Selected )
											ImGui::SetItemDefaultFocus();

										ImGui::EndListBox();
									}

									if( PopupModified )
									{
										ImGui::CloseCurrentPopup();
										m_OpenEntityFinderPopup = false;
									}

									ImGui::EndPopup();
								}
							}
						} break;

						case SPropertyType::Asset:
						{
							static bool open = false;

							auto& rRef = pProperty->Read<SPropertyType::Asset>( entity.Get() );

							ImGui::PushID( name.c_str() );

							ImGui::Columns( 2 );
							ImGui::SetColumnWidth( 0, 125.0f );
							ImGui::Text( name.c_str() );
							ImGui::NextColumn();

							ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

							if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
							{
								open ^= 1;

								m_CurrentFinderType = AssetType::Unknown;

								m_CurrentAssetID = rRef.ID;
							}

							ImGui::SameLine();
							std::string assetName = " <no asset set>";

							if( rRef.ID != 0 )
							{
								if( Ref<Asset> asset = AssetManager::Get().FindAsset( rRef.ID ); asset != nullptr )
								{
									assetName = " " + asset->Name;
								}
								else
								{
									assetName = " <asset missing!>";
								}
							}

							ImGui::Text( assetName.c_str() );

							ImGui::PopItemWidth();
							ImGui::PopStyleVar();

							ImGui::Columns( 1 );

							if( Auxiliary::DrawAssetFinder( m_CurrentFinderType, &open, m_CurrentAssetID ) )
							{
								pProperty->SetProperty( entity.Get(), m_CurrentAssetID );
							}

							ImGui::PopID();
						} break;
					}
				}
			}

			ImGui::TreePop();
		}

		ImGui::Separator();
	} 

	void SceneHierarchyPanel::DrawEntityComponents( Ref<Entity> entity )
	{
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
		const bool isPrefab = entity->HasComponent<PrefabComponent>();
		const auto& id = entity->GetComponent<IdComponent>().ID;

		ImGui::Image( m_EditIcon->GetDescriptorSet(), ImVec2( 30.0f, 30.0f ) );

		ImGui::SameLine();

		// TODO: We really don't need to check this as entities will always have a tag.
		if( entity->HasComponent<TagComponent>() )
		{
			auto& tag = entity->GetComponent<TagComponent>().Tag;
			char buffer[ 256 ];
			memset( buffer, 0, 256 );
			memcpy( buffer, tag.c_str(), tag.length() );

			ImGui::PushItemWidth( contentRegionAvailable.x - ImGui::GetStyle().FramePadding.x );
			if( ImGui::InputText( "##Tag", buffer, 256 ) )
			{
				std::string newChar( buffer );
				tag = newChar;

				Ref<UndoRedoActionModifyString> action = Ref<UndoRedoActionModifyString>::Create( "Modify Entity Tag", &tag, tag, newChar );

				GlobalUndoRedoGroup::Get().AddAction( action, ( uint64_t ) entity->GetHandle() );
				m_Context->MarkDirty();
			}
			ImGui::PopItemWidth();
		}

		// Draw ID and entity class type.
		{
			// ID
			ImGui::TextDisabled( "%llu", id );

			ImGui::SameLine();
			ImGui::TextDisabled( "%s", entity->GetClass()->GetName().c_str() );

			if( entity->HasComponent<PrefabComponent>() )
			{
				ImGui::SameLine();
				ImGui::TextDisabled( "Class Instance (Prefab)" );
			}
		}

		DrawEntityProperties( entity );

		//////////////////////////////////////////////////////////////////////////
		// Components

		DrawComponent<TransformComponent>( "Transform", entity, [&]( auto& tc )
		{
			bool modified = false;

			auto& translation = tc.Position;
			glm::vec3 rotation = glm::degrees( tc.GetRotationEuler() );
			auto& scale = tc.Scale;

			modified = Auxiliary::DrawVec3Control( "Translation", tc.Position );
		
			if( Auxiliary::DrawVec3Control( "Rotation", rotation ) ) 
			{
				tc.SetRotation( glm::radians( rotation ) );
				
				modified |= true;
			}

			modified |= Auxiliary::DrawVec3Control( "Scale", tc.Scale, 1.0f );

			if( modified )
			{
				m_Context->MarkDirty();
			
				if( m_Context->GetNavBoundsEntity() == entity )
				{
					m_Context->GetNavBoundsEntity()->SetAABB( tc.Position, tc.Scale );
					m_Context->GetNavBoundsEntity()->MarkDirty();
				}
			}
		} );

		DrawComponent<StaticMeshComponent>( "Static Mesh", entity, [&]( StaticMeshComponent& mc )
		{
			bool modified = false;
			bool open = false;
			static uint32_t s_CurrentIndex = 0;

			ImGui::Columns( 3 );
			ImGui::SetColumnWidth( 0, 100.0f );
			ImGui::SetColumnWidth( 1, 300.0f );
			ImGui::SetColumnWidth( 2, 40.0f );
			ImGui::Text( "File Path" );
			ImGui::NextColumn();
			ImGui::PushItemWidth( -1.0f );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				open = !open;
				m_CurrentFinderType = AssetType::StaticMesh;

				if( mc.Mesh )
					m_CurrentAssetID = mc.Mesh->ID;
			}

			ImGui::SameLine();

			if( mc.Mesh )
				ImGui::InputText( "##meshfilepath", ( char* ) mc.Mesh->Name.c_str(), 256, ImGuiInputTextFlags_ReadOnly );
			else
				ImGui::InputText( "##meshfilepath", ( char* ) "", 256, ImGuiInputTextFlags_ReadOnly );

			if( mc.Mesh ) 
			{
				if( Auxiliary::TreeNode( "Materials" ) )
				{
					int i = 0;
					for( auto& rAsset : mc.MaterialRegistry->GetMaterialAssets() )
					{
						std::string name = rAsset->Name.empty() ? rAsset->GetMaterialName() : rAsset->Name;

						if( ImGui::Button( name.c_str() ) )
						{
							m_CurrentFinderType = AssetType::Material;
							open = !open;
							s_CurrentIndex = i;
						}

						if( mc.MaterialRegistry->HasOverrides( i ) )
						{
							ImGui::SameLine();

							if( ImGui::SmallButton( "x" ) )
							{
								mc.MaterialRegistry->ResetMaterial( i, mc.Mesh->GetMaterialRegistry() );
								modified |= true;
							}
						}

						i++;
					}

					Auxiliary::EndTreeNode();
				}
			}

			if( Auxiliary::DrawAssetFinder( m_CurrentFinderType, &open, m_CurrentAssetID ) )
			{
				if( m_CurrentFinderType == AssetType::StaticMesh )
				{
					mc.Mesh = AssetManager::Get().GetAssetAs<StaticMesh>( m_CurrentAssetID );

					mc.MaterialRegistry = nullptr;
					mc.MaterialRegistry = Ref<MaterialRegistry>::Create( mc.Mesh );
				}
				else if( m_CurrentFinderType == AssetType::Material )
				{
					// Don't update pure dependencies here because pure dependencies are only for assets and this change is local to this material registry
					mc.MaterialRegistry->SetMaterial( s_CurrentIndex, m_CurrentAssetID );
				}

				modified = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<CameraComponent>( "Camera", entity, [&]( auto& cc )
		{
			bool modified = false;

			modified =  Auxiliary::DrawBoolControl( "Main Camera", cc.MainCamera );
			modified |= Auxiliary::DrawFloatControl( "Field of View", cc.Fov, 10.0f, 100.0f );

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<PointLightComponent>( "Point Light", entity, [&]( auto& plc )
		{
			bool modified = false;

			modified =  Auxiliary::DrawColorVec3Control( "Light Color", plc.Radiance, 150.0f );
			modified |= Auxiliary::DrawFloatControl( "Light Intensity", plc.Multiplier, 0.0f, 500.0f );
			modified |= Auxiliary::DrawFloatControl( "Radius", plc.Radius, 0.0f, FLT_MAX );
			modified |= Auxiliary::DrawFloatControl( "Falloff", plc.Falloff, 0.0f, 1.0f );

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<DirectionalLightComponent>( "Directional Light", entity, [&]( auto& dlc )
		{
			bool modified = false;

			modified =  Auxiliary::DrawFloatControl( "Intensity", dlc.Intensity, 0.0f, 110.0f );
			modified |= Auxiliary::DrawBoolControl( "Cast shadows", dlc.CastShadows );
			modified |= Auxiliary::DrawColorVec3Control( "Radiance", dlc.Radiance, 1.0f );

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<SkylightComponent>( "Skylight", entity, [&]( auto& skl )
		{
			if( Auxiliary::DrawBoolControl( "Dynamic Sky", skl.DynamicSky ) || skl.DynamicSky )
			{
				bool changed = false;

				changed = Auxiliary::DrawFloatControl( "Turbidity", skl.Turbidity );
				changed |= Auxiliary::DrawFloatControl( "Azimuth", skl.Azimuth );
				changed |= Auxiliary::DrawFloatControl( "Inclination", skl.Inclination );

				if( changed ) 
				{
					Application::Get().PrimarySceneRenderer().SetDynamicSky( skl.Turbidity, skl.Azimuth, skl.Inclination );

					m_Context->MarkDirty();
				}
			}
		} );

		DrawComponent<BoxColliderComponent>( "Box Collider", entity, [&]( auto& bc )
		{
			bool modified = false;

			{
				Auxiliary::ScopedItemFlag disabledFlag( ImGuiItemFlags_Disabled, bc.AutoAdjustExtent );
				Auxiliary::ScopedStyleVar<float> styleVar( ImGuiStyleVar_Alpha, bc.AutoAdjustExtent ? 0.5f : 1.0f );

				modified = Auxiliary::DrawVec3Control( "Extent", bc.Extents );
			}
			
			modified |= Auxiliary::DrawVec3Control( "Offset", bc.Offset );
			modified |= Auxiliary::DrawBoolControl( "Is Trigger", bc.IsTrigger );

			if( Auxiliary::DrawBoolControl( "Auto Adjust Extent", bc.AutoAdjustExtent ) || bc.AutoAdjustExtent )
			{
				auto& transform = entity->GetComponent<TransformComponent>();
				bc.Extents = transform.Scale;

				modified |= true;
			}

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<SphereColliderComponent>( "Sphere Collider", entity, [&]( auto& sc )
		{
			bool modified = false;

			modified =  Auxiliary::DrawFloatControl( "Radius", sc.Radius );
			modified |= Auxiliary::DrawVec3Control( "Offset", sc.Offset );
			modified |= Auxiliary::DrawBoolControl( "Is Trigger", sc.IsTrigger );

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<CapsuleColliderComponent>( "Capsule Collider", entity, [&]( auto& cc )
		{
			bool modified = false;

			modified =  Auxiliary::DrawFloatControl( "Radius", cc.Radius );
			modified |= Auxiliary::DrawFloatControl( "Height", cc.Height );
			modified |= Auxiliary::DrawVec3Control( "Offset", cc.Offset );
			modified |= Auxiliary::DrawBoolControl( "Is Trigger", cc.IsTrigger );

			if( modified ) m_Context->MarkDirty();
		} );

		/*
		DrawComponent<MeshColliderComponent>( "Mesh Collider", entity, []( auto& mcc )
		{
			Auxiliary::DrawBoolControl( "Is Trigger", mcc.IsTrigger );
		} );
		*/

		DrawComponent<RigidbodyComponent>( "Rigidbody", entity, [&]( auto& rb )
		{
			bool modified = false;

			if( !m_Context->IsRuntimeRunning() )
			{
				modified = Auxiliary::DrawBoolControl( "Kinematic Body", rb.IsKinematic );
				modified |= Auxiliary::DrawBoolControl( "Use CCD", rb.UseCCD );

				modified |= Auxiliary::DrawFloatControl( "Mass", rb.Mass );
				modified |= Auxiliary::DrawFloatControl( "Linear Drag", rb.LinearDrag );
			}
			else
			{
				{
					Auxiliary::ScopedDisabledFlag disabled( true );
					
					Auxiliary::DrawBoolControl( "Kinematic Body", rb.IsKinematic );
					Auxiliary::DrawBoolControl( "Use CCD", rb.UseCCD );
				}
				
				if( Auxiliary::DrawFloatControl( "Mass", rb.Mass ) )
					rb.Rigidbody->SetMass( rb.Mass );

				if( Auxiliary::DrawFloatControl( "Linear Drag", rb.LinearDrag ) )
					rb.Rigidbody->SetLinearDrag( rb.LinearDrag );
			}

			//////////////////////////////////////////////////////////////////////////

			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, 125.0f );

			ImGui::BeginHorizontal( "rbMaterial" );
			{
				ImGui::Text( "Physics Material" );

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "This will override the meshes physics material to an asset of your choice." );
					ImGui::Text( "If there is no mesh then the engine will automatically use the project default physics material. If there is no project default then it will create a internal material for it." );
					ImGui::Text( "You do not need to change this if you wish to keep using the meshes physics material." );

					ImGui::EndTooltip();
				}

				ImGui::NextColumn();

				if( rb.MaterialAssetID != 0 )
				{
					ImGui::InputText( "##physMaterial", ( char* ) std::to_string( rb.MaterialAssetID ).c_str(), 256, ImGuiInputTextFlags_ReadOnly );
				
					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Overridden." );
						ImGui::EndTooltip();
					}
				}
				else if( entity->HasComponent<StaticMeshComponent>() )
				{
					if( auto& rStaticMesh = entity->GetComponent<StaticMeshComponent>().Mesh; rStaticMesh != nullptr )
					{
						ImGui::InputText( "##physMaterial", ( char* ) std::to_string( rStaticMesh->GetPhysicsMaterial() ).c_str(), 256, ImGuiInputTextFlags_ReadOnly );

						if( ImGui::BeginItemTooltip() )
						{
							ImGui::Text( "Inherited from static mesh." );
							ImGui::EndTooltip();
						}
					}
				}
				else 
				{
					ImGui::InputText( "##physMaterial", ( char* ) "No Asset", 256, ImGuiInputTextFlags_ReadOnly );
				}

				bool openAssetFinder = false;
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
				{
					openAssetFinder = !openAssetFinder;
					m_CurrentFinderType = AssetType::PhysicsMaterial;

					if( rb.MaterialAssetID != 0 ) 
					{
						m_CurrentAssetID = rb.MaterialAssetID;
					}
				}

				// TODD: Remove double check (condition was already checked when we render the input text)
				if( rb.MaterialAssetID != 0 )
				{
					if( ImGui::Button( "Reset", ImVec2( 24.0f, 24.0f ) ) )
					{
						rb.MaterialAssetID = 0;
						modified = true;
					}
				}

				if( Auxiliary::DrawAssetFinder( m_CurrentFinderType, &openAssetFinder, m_CurrentAssetID ) )
				{
					rb.MaterialAssetID = m_CurrentAssetID;
					modified |= true;
				}
			}
			ImGui::EndHorizontal();

			ImGui::Columns( 1 );

			//////////////////////////////////////////////////////////////////////////

			ImGui::PushID( "rbPos" );

			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, 125.0f );

			ImGui::BeginHorizontal( "rbPos" );

			ImGui::Text( "Position Lock" );

			ImGui::NextColumn();

			bool posX = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_PositionX;
			bool posY = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_PositionY;
			bool posZ = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_PositionZ;

			ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 1.0f, 0.0f } );

			if( ImGui::Checkbox( "##posX", &posX ) )
			{
				if( posX )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_PositionX;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_PositionX;

				modified |= true;
			}

			ImGui::PopItemWidth();

			if( ImGui::Checkbox( "##posY", &posY ) )
			{
				if( posY )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_PositionY;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_PositionY;

				modified |= true;
			}

			ImGui::PopItemWidth();

			if( ImGui::Checkbox( "##posZ", &posZ ) ) 
			{
				if( posZ )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_PositionZ;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_PositionZ;
			
				modified |= true;
			}

			ImGui::PopItemWidth();

			ImGui::PopStyleVar();

			ImGui::EndHorizontal();

			ImGui::Columns( 1 );

			ImGui::PopID();

			//////////////////////////////////////////////////////////////////////////

			ImGui::PushID( "rbRot" );

			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, 125.0f );

			ImGui::BeginHorizontal( "rbRot" );

			ImGui::Text( "Rotation Lock" );

			ImGui::NextColumn();

			ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 1.0f, 0 } );

			bool rotX = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationX;
			bool rotY = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationY;
			bool rotZ = rb.LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationZ;

			if( ImGui::Checkbox( "##rotX", &rotX ) )
			{
				if( rotX )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_RotationX;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_RotationX;
		
				modified |= true;
			}

			ImGui::PopItemWidth();

			if( ImGui::Checkbox( "##rotY", &rotY ) )
			{
				if( rotY )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_RotationY;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_RotationY;
			
				modified |= true;
			}

			ImGui::PopItemWidth();

			if( ImGui::Checkbox( "##rotZ", &rotZ ) )
			{
				if( rotZ )
					rb.LockFlags |= RigidbodyLockFlags::RigidbodyLock_RotationZ;
				else
					rb.LockFlags &= ~RigidbodyLockFlags::RigidbodyLock_RotationZ;
			
				modified |= true;
			}

			ImGui::PopStyleVar();

			ImGui::EndHorizontal();

			ImGui::Columns( 1 );

			ImGui::PopID();
			
			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<BillboardComponent>( "Billboard", entity, [&](auto& bc) 
		{
			bool open = false;

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24, 24 ) ) )
			{
				m_CurrentFinderType = AssetType::Texture;
				open = true;

				if( bc.AssetID != 0 )
					m_CurrentAssetID = bc.AssetID;
			}

			ImGui::SameLine();

			if( Auxiliary::DrawAssetFinder( m_CurrentFinderType, &open, m_CurrentAssetID ) )
			{
				bc.AssetID = m_CurrentAssetID;
				
				m_Context->MarkDirty();
			}
		} );

		DrawComponent<AudioPlayerComponent>( "Audio Player", entity, [&]( AudioPlayerComponent& ap )
		{
			bool modified = false;

			{
				bool open = false;			
				
				// Push disabled flag if runtime running
				Auxiliary::ScopedDisabledFlag disabledFlag( m_Context->IsRuntimeRunning() );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24, 24 ) ) )
				{
					m_CurrentFinderType = AssetType::Sound;
					open = true;

					if( ap.SpecAssetID != 0 )
						m_CurrentAssetID = ap.SpecAssetID;
				}

				ImGui::SameLine();

				if( Auxiliary::DrawAssetFinder( { AssetType::GraphSound, AssetType::Sound }, &open, m_CurrentAssetID ) )
				{
					ap.SpecAssetID = m_CurrentAssetID;
					modified = true;
				}

				ImGui::PushID( ( int ) ap.UniqueID );

				if( ap.SpecAssetID != 0 )
					ImGui::InputText( "##2dplayerid", ( char* ) std::to_string( ap.SpecAssetID ).c_str(), 256, ImGuiInputTextFlags_ReadOnly );
				else
					ImGui::InputText( "##2dplayerid", ( char* ) "", 256, ImGuiInputTextFlags_ReadOnly );

				ImGui::PopID();
			}

			if( m_Context->IsRuntimeRunning() )
			{
				Ref<Sound> sound = AudioSystem::Get().FindSound( ap.UniqueID );
				if( sound )
				{
					if( Auxiliary::DrawBoolControl( "Loop", ap.Loop ) ) 
						sound->Loop( ap.Loop );

					Auxiliary::DrawBoolControl( "Mute", ap.Mute );
					
					if( Auxiliary::DrawBoolControl( "Spatialization", ap.Spatialization ) )
						sound->SetSpatialisation( ap.Spatialization );

					if( Auxiliary::DrawFloatControl( "Volume", ap.Volume, 0.0f, 100.0f ) )
						sound->SetVolume( ap.Volume );

					if( Auxiliary::DrawFloatControl( "Pitch", ap.Pitch, 0.0f, 100.0f ) )
						sound->SetPitch( ap.Pitch );
				}
				else
				{
					ImGui::Text( "Sound could not be found in active scene. This should not happen and may indicate a bug in the application." );
					ImGui::Text( "Looking for: %llu (ASSET/%llu). Was it marked for destruction?", ap.UniqueID, ap.SpecAssetID );

					Auxiliary::DrawDisabledBoolControl( "Loop", ap.Loop );
					Auxiliary::DrawDisabledBoolControl( "Mute", ap.Mute );
					Auxiliary::DrawDisabledBoolControl( "Spatialization", ap.Spatialization );
					Auxiliary::DrawDisabledFloatControl( "Volume", ap.Volume );
					Auxiliary::DrawDisabledFloatControl( "Pitch", ap.Pitch );
				}
			}
			else
			{
				bool open = false;

				modified |= Auxiliary::DrawBoolControl( "Loop", ap.Loop );
				modified |= Auxiliary::DrawBoolControl( "Mute", ap.Mute );
				modified |= Auxiliary::DrawBoolControl( "Spatialization", ap.Spatialization );

				modified |= Auxiliary::DrawFloatControl( "Volume", ap.Volume, 0.0f, 100.0f );
				modified |= Auxiliary::DrawFloatControl( "Pitch", ap.Pitch, 0.0f, 100.0f );

				ImGui::PushID( "##select_sound_grp" );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24, 24 ) ) )
				{
					open = true;
				}

				ImGui::PopID();

				if( open == true && !ImGui::IsPopupOpen( "SoundGroupPopup" ) )
				{
					ImGui::OpenPopup( "SoundGroupPopup" );
					open = false;
				}

				ImGui::SetNextWindowSize( { 250.0f, 0.0f } );
				if( ImGui::BeginPopup( "SoundGroupPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
				{
					bool PopupModified = false;

					if( ImGui::BeginListBox( "##ASSETLIST", ImVec2( -FLT_MIN, 0.0f ) ) )
					{
						for( const auto& rSoundGroup : Project::GetActiveProject()->GetSoundGroups() )
						{
							ImGui::PushID( rSoundGroup->GetName().c_str() );

							if( ImGui::Selectable( rSoundGroup->GetName().c_str(), false ) )
							{
								ap.SoundGroup = rSoundGroup;

								PopupModified = true;
							}

							ImGui::PopID();

							//if( Selected )
							//	ImGui::SetItemDefaultFocus();
						}

						ImGui::EndListBox();
					}

					if( PopupModified )
					{
						ImGui::CloseCurrentPopup();

						modified |= true;
						open = false;
					}

					ImGui::EndPopup();
				}
			}

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<AudioListenerComponent>( "Audio Listener", entity, [ & ]( auto& al )
		{
			bool modified = false;

			modified = Auxiliary::DrawBoolControl( "Primary", al.Primary );
			modified |= Auxiliary::DrawVec3Control( "Direction", al.Direction );
			modified |= Auxiliary::DrawFloatControl( "ConeInnerAngle", al.ConeInnerAngle );
			modified |= Auxiliary::DrawFloatControl( "ConeOuterAngle", al.ConeOuterAngle );

			if( modified ) m_Context->MarkDirty();
		} );

		DrawComponent<NavigationMeshSpecificationComponent>( "Navigation Specification", entity, [ & ]( auto& nms )
		{
			Ref<NavBoundsEntity> boundsEntity = entity.As<NavBoundsEntity>();

			Auxiliary::DisabledFlag disabledFlag( true );
			Auxiliary::DrawVec3Control( "Extent", nms.Extent );
			disabledFlag.Pop();

			{
				auto& transform = entity->GetComponent<TransformComponent>();
				nms.Extent = transform.Scale;
			}

			if( ImGui::Button( "Build" ) )
			{
				nms.HasBuilt = true;

				if( boundsEntity )
				{
					boundsEntity->GatherGeometryAndBuild();
				}
			}

			if( boundsEntity->NeedsRebuilding() )
			{
				std::string text = "A rebuild is required for the changes to have effect!";

				ImVec2 padding = ImGui::GetStyle().FramePadding;
				ImVec2 textPosition = ImGui::GetCursorScreenPos();
				ImVec2 textSize = ImGui::CalcTextSize( text.c_str() );

				ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
				ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

				ImGui::GetWindowDrawList()->AddRectFilled( min, max,
					IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

				ImGui::TextUnformatted( text.c_str() );
			}
		} );
	}

	template<typename T, typename UIFunction>
	void Saturn::SceneHierarchyPanel::DrawComponent( const std::string& name, Ref<Entity> entity, UIFunction uiFunction )
	{
		// TODO: Support multiple selections (for this function)
		if( entity->HasComponent<T>() )
		{
			bool removeComponent = false;

			auto& component = entity->GetComponent<T>();

			bool open = ImGui::TreeNodeEx( ( void* ) ( ( uint32_t ) entity | typeid( T ).hash_code() ), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap, name.c_str() );

			ImGui::SameLine();
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0, 0, 0, 0 ) );
			if( ImGui::Button( "+" ) )
			{
				ImGui::OpenPopup( "ComponentSettings" );
			}

			ImGui::PopStyleColor( 2 );

			if( ImGui::BeginPopup( "ComponentSettings" ) )
			{
				// TODO: Add compile time flags to this
				if constexpr( !std::is_same<T, TransformComponent>() && !std::is_same<T, NavigationMeshSpecificationComponent>() )
				{
					if( ImGui::MenuItem( "Remove component" ) )
						removeComponent = true;
				}

				if( ImGui::MenuItem( "Copy component" ) )
				{
					if( m_CopyComponentData.Buffer.Size > 0 )
						m_CopyComponentData.Buffer.Free();

					m_CopyComponentData.Name = name;

					m_CopyComponentData.Buffer.Allocate( sizeof( T ) );
					m_CopyComponentData.Buffer.Write( reinterpret_cast< void* >( &component ), sizeof( T ), 0 );
				}

				if( ImGui::MenuItem( "Paste component" ) )
				{
					if( m_CopyComponentData.Buffer.Size > 0 && m_CopyComponentData.Name == name )
					{
						component = m_CopyComponentData.Buffer.Read<T>( 0 );
					}
				}

				ImGui::EndPopup();
			}

			if( open )
			{
				uiFunction( component );
				ImGui::NextColumn();
				ImGui::Columns( 1 );
				ImGui::TreePop();
			}
			ImGui::Separator();

			if( removeComponent )
			{
				entity->RemoveComponent<T>();
			}
		}
	}
}