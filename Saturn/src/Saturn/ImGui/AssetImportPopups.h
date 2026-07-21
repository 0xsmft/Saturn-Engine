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

#pragma once

#include "AssetImportPopupErrors.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/TextureLoadFlags.h"

#include "Saturn/Vulkan/Mesh.h"

#include <filesystem>

namespace Saturn {

	enum class AssetImportModificationState : uint8_t
	{
		// Failed, popup may of modified but m_Error in the popup will be set to the correct error.
		Failed, 

		// Operation was canceled
		NotModified, 
		
		// Succeeded
		Modified 
	};

	//
	// AssetImportPopupBase is the base class for all asset popups,
	// every popup does not save the asset manager, you must save it after the popup has been modified.
	//
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

	public:
		void Close() { m_Open = false; }
		void DrawErrorTextAndDescription();
		void SetIsReimport( bool reimport, AssetID reimportID ) { m_IsReimport = reimport; m_ReimportID = reimportID; }

	public:
		[[nodiscard]] bool IsReady() const { return m_IsReady.load(); }
		[[nodiscard]] bool IsOpen() const { return m_Open; }
		[[nodiscard]] AssetImportModificationState GetModificationState() const { return m_ModificationState; }
		[[nodiscard]] AssetImportPopupError GetError() const { return m_Error; }
		[[nodiscard]] bool HasError() const { return m_Error != AssetImportPopupError::None; }

	protected:
		AssetImportModificationState m_ModificationState = AssetImportModificationState::NotModified;
		AssetImportPopupError m_Error = AssetImportPopupError::None;
		bool m_Open = false;
		bool m_IsReimport = false;
		std::atomic_bool m_IsReady{ false };

		AssetID m_ReimportID = 0;

		std::filesystem::path m_AssetToImportPath;
		std::filesystem::path m_DestinationPath;
	};

	//////////////////////////////////////////////////////////////////////////

	class AssetImportPopupAuxiliary
	{
	public:
		// NB: Will return Unknown if not found!
		static std::shared_ptr<AssetImportPopupBase> CreatePopupFromAssetTypeReimport( Ref<Asset> asset, const std::filesystem::path& rDestinationPath );
	};
	
	//////////////////////////////////////////////////////////////////////////

	class UnknownImportPopup : public AssetImportPopupBase 
	{
	public:
		UnknownImportPopup( const std::filesystem::path& rAssetToImportPath );
		~UnknownImportPopup() = default;

		virtual void Initialise() override;
		virtual void OnImGuiRender() override;
	};

	class Texture2D;

	//
	// Imports a raw texture source file and create a TextureSourceAsset (.stx)
	//
	class TextureSourceAssetImportPopup : public AssetImportPopupBase
	{
	public:
		TextureSourceAssetImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath );
		virtual ~TextureSourceAssetImportPopup();

		virtual void Initialise() override;
		virtual void OnImGuiRender() override;
	
	private:
		void CreateNew();
		void Reimport();

	private:
		Ref<Texture2D> m_PreviewTexture = nullptr;
		std::underlying_type_t<TextureLoadFlags> m_ImportBehaviour = TextureLoadFlags_None;
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

		AssetImportPopupError FullyImportMesh();
		AssetImportPopupError ImportDynamic();
		AssetImportPopupError ImportStatic();

	private:
		std::filesystem::path m_GLTFBinPath;
		bool m_UseBinFile = false;
		bool m_GLTFBinFileExists = false;
		bool m_IsSkeletal = false;

		MeshImportBehaviour m_ImportBehaviour = MeshImportBehaviour_Default;
		AssetID m_CurrentAssetIDForMaterial = 0;
		AssetID m_CurrentAssetIDForSkeleton = 0;
	};

	//
	// SoundImportPopup
	// 
	// This popup handles the importation of a sound asset.
	// 
	// It will always take in a raw sound source file e.g. MySound.mp3
	//
	class SoundImportPopup : public AssetImportPopupBase
	{
	public:
		SoundImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath );
		~SoundImportPopup() = default;

		virtual void Initialise();
		virtual void OnImGuiRender();

	private:
		void CreateNew();
		void Reimport();
		std::string GetLastModificationDate();
	};

	class FontImportPopup : public AssetImportPopupBase 
	{
	public:
		FontImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath );
		~FontImportPopup() = default;

		virtual void Initialise();
		virtual void OnImGuiRender();
	
	private:
		void CreateNew();
		void Reimport();
	};

}
