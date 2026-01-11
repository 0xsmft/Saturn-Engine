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

#define SAT_SERIALISER_CLASS( Name ) \
	class Name##AssetSerialiser : public AssetSerialiser \
	{ \
	public: \
		virtual void Serialise( const Ref<Asset>& rAsset ) const override; \
		[[nodiscard]] virtual bool TryLoadData( Ref<Asset>& rAsset ) const override; \
	};

namespace Saturn {

	class AssetSerialiser
	{
	public:
		virtual void Serialise   ( const Ref<Asset>& rAsset ) const = 0;
		[[nodiscard]] virtual bool TryLoadData (       Ref<Asset>& rAsset ) const = 0;
	};

	class MaterialAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class PrefabSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise   ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData (	  Ref<Asset>& rAsset ) const override;
	};

	class StaticMeshAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class SkeletalMeshAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class SoundSpecificationAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class PhysicsMaterialAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class BehaviourTreeMemorySpecAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class SkeletonAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class SkeletalAnimationAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class AluraFontAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};

	class AluraStylingProfileAssetSerialiser : public AssetSerialiser
	{
	public:
		virtual void Serialise  ( const Ref<Asset>& rAsset ) const override;
		[[nodiscard]] virtual bool TryLoadData(       Ref<Asset>& rAsset ) const override;
	};
}
