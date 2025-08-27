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
#include "Saturn/Vulkan/Mesh.h"

#include <filesystem>

namespace Saturn {

	enum class AssetImportModificationState
	{
		NotModified,
		Modified
	};

	class AssetImportPopupBase
	{
	public:
		AssetImportPopupBase( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
			: m_AssetToImportPath( rAssetToImportPath ), m_DestinationPath( rDestinationPath )
		{
		}
		virtual ~AssetImportPopupBase() = default;

		virtual void Initialise() {}
		virtual void OnImGuiRender() {}

		void Close() { m_Open = false; }

		[[nodiscard]] bool IsReady() const { return m_IsReady.load(); }
		[[nodiscard]] bool IsOpen() const { return m_Open; }
		[[nodiscard]] AssetImportModificationState GetModificationState() const { return m_ModificationState; }

	protected:
		bool m_Open = false;
		std::atomic_bool m_IsReady{ false };
		AssetImportModificationState m_ModificationState = AssetImportModificationState::NotModified;

		std::filesystem::path m_AssetToImportPath;
		std::filesystem::path m_DestinationPath;
	};

	class MeshImportPopup : public AssetImportPopupBase
	{
	public:
		MeshImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath );
		~MeshImportPopup() = default;

		virtual void Initialise();
		virtual void OnImGuiRender();

	private:
		void DrawGLTFOptions();
		void DrawSkeletalMeshOptions();
		void DrawAndHandleImportBehaviour();
		void FullyImportMesh();

	private:
		std::filesystem::path m_GLTFBinPath;
		bool m_UseBinFile = false;
		bool m_IsSkeletal = false;

		MeshImportBehaviour m_ImportBehaviour = MeshImportBehaviour_Default;
		AssetID m_CurrentAssetIDForMaterial = 0;
	};

	[[nodiscard]] extern bool DrawImportMeshPopup( bool* pOpen, const std::filesystem::path& rImportTargetPath );
	[[nodiscard]] extern bool DrawImportSoundPopup( bool* pOpen, const std::filesystem::path& rImportTargetPath, std::filesystem::path& rDefaultPath );

}