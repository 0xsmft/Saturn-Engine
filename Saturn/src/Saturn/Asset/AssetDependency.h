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

	// AssetDependency<AssetType...>
	// This class is a wrapper for an AssetID with additional support for registering asset dependencies
	// 
	// In Development configurations:
	// - The size of this class will be 16 bytes 8 for the AssetID and 8 for the virtual function
	// - It will automatically RegisterAssetDependency/UnregisterAssetDependency when assigning a new value.
	//
	// In Distribution configurations:
	// - This class then becomes a glorified Saturn::AssetID wrapper as this class will not register any Asset Dependencies futhermore, the size of this class will be 8 bytes only holding an AssetID
	// - AssetDependencies do not exist in Distribution configurations
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
			UnregisterAssetDependency( AssetID );
			AssetID = newID;
			RegisterAssetDependency( AssetID );
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

	// AssetDependencyNotifier
	// This class is a wrapper for an AssetID with additional support for registering asset dependencies
	// This class is very similar to AssetDependency<> however, it does not give compile time asset types.
	// The usage of this class is similar to AssetDependency<> however, it should be used when you don't actually care about the asset ids but you still need to depend on them. (See: AssetRegistry)
	// 
	// AssetDependencyNotifier will call a function when the value is changed.
	// 
	// In Development configurations:
	// - The size of this class will be 80 bytes, 8 for the AssetID and 8 for the virtual function and 64 bytes for the std::function
	// - It will automatically RegisterAssetDependency/UnregisterAssetDependency when assigning a new value.
	// - Call function when value changed.
	//
	// In Distribution configurations:
	// - This class then becomes a glorified Saturn::AssetID wrapper as this class will not register any Asset Dependencies futhermore, the size of this class will be 8 bytes only holding an AssetID
	// - AssetDependencies do not exist in Distribution configurations
	class AssetDependencyNotifier : public AssetDependencyBase
	{
	public:
		Saturn::AssetID ID;
	public:
		AssetDependencyNotifier() = default;

		template<typename Func>
		AssetDependencyNotifier( Func&& rrFunc ) 
#if !defined(SAT_DIST)
			: CallbackFunction( std::forward<Func>( rrFunc ) ) {}
#else
		{ }
#endif

		operator Saturn::AssetID() { return ID; }
		operator const Saturn::AssetID() const { return ID; }

		operator uint64_t() { return ID; }
		operator const uint64_t() const { return ID; }

		Saturn::AssetID operator=( const Saturn::AssetID& rOther ) noexcept
		{
#if !defined(SAT_DIST)
			UnregisterAssetDependency( ID );

			if( CallbackFunction ) ( ID, rOther );

			ID = rOther;

			RegisterAssetDependency( ID );
#else
			ID = rOther;
#endif

			return ID;
		}

#if !defined(SAT_DIST)
	public:
		void OnUpdate( Saturn::AssetID newID )
		{
			UnregisterAssetDependency( ID );
			if( CallbackFunction ) ( ID, newID );
			ID = newID;
			RegisterAssetDependency( ID );
		}
	private:
		//                          New       Old
		std::function<void( Saturn::AssetID, Saturn::AssetID )> CallbackFunction;
#endif
	};

}
