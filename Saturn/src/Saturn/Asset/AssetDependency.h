/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "Asset.h"

namespace Saturn {

	class AssetDependencyBase
	{
#if !defined(SAT_DIST)
	public:
		virtual void OnUpdate( AssetID ) = 0;
	protected:
		void UnregisterAssetDependency( AssetID id );
		void RegisterAssetDependency( AssetID id );
#endif
	};

	template<Saturn::AssetType... Types>
	class AssetDependency : public AssetDependencyBase
	{
	public:
		AssetDependency() = default;
		~AssetDependency() = default;

		AssetDependency( Saturn::AssetID id );

	public:
		Saturn::AssetID AssetID = 0;

		operator Saturn::AssetID() { return AssetID; }
		operator const Saturn::AssetID() const { return AssetID; }

		operator uint64_t() { return AssetID; }
		operator const uint64_t() const { return AssetID; }

		using TypesTuple = std::tuple<std::integral_constant<Saturn::AssetType, Types>...>;

		static constexpr std::array<Saturn::AssetType, sizeof...( Types )> GetTypes()
		{
			return { Types... };
		}

#if !defined(SAT_DIST)
		void OnUpdate( Saturn::AssetID newID )
		{
			AssetID = newID;
		}

		Saturn::AssetID operator=( const Saturn::AssetID& rOther ) noexcept
		{
			UnregisterAssetDependency( AssetID );
			
			AssetID = rOther;
			
			RegisterAssetDependency( AssetID );

			return AssetID;
		}
#else
		Saturn::AssetID operator=( const Saturn::AssetID& rOther ) noexcept
		{
			AssetID = rOther;
			return AssetID;
		}
#endif
	};
}
