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

#define SAT_ARRAYSIZE( x ) ( ( int ) ( sizeof( x ) / sizeof( *( x ) ) ) )

#define SAT_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

template<typename Ty>
consteval auto BIT( Ty x ) { return 1 << x; }

//////////////////////////////////////////////////////////////////////////
// CONCAT
#define SAT_CONTACT_INNER(x,y) x##y
#define SAT_CONTACT(x,y) SAT_CONTACT_INNER(x,y)

#define SAT_CONTACT_THREE_INNER(x,y,z) x##y##z
#define SAT_CONTACT_THREE(x,y,z) SAT_CONTACT_THREE_INNER(x,y,z)

//////////////////////////////////////////////////////////////////////////
// VERSION
template<typename Ty>
consteval auto SAT_MAKE_VERSION( Ty major, Ty minor, Ty patch ) { return ( ( ( ( unsigned int ) ( major ) ) << 22 ) | ( ( ( unsigned int ) ( minor ) ) << 12 ) | ( ( unsigned int ) ( patch ) ) ); }

// Current version is Alpha 0.2.6 (Alpha 2.6)
constexpr auto SAT_CURRENT_VERSION = SAT_MAKE_VERSION( 0, 2, 6 );
constexpr auto SAT_CURRENT_VERSION_STRING = "0.2.6";

#define SAT_DECODE_VERSION(source, major, minor, patch) \
patch = (source) & 0xFF; \
minor = ((source) >> 12) & 0x3FF;\
major = (source) >> 22

#define SAT_DECODE_VER_STRING( sourceVersion, string ) \
{ \
uint32_t major, minor, patch; \
SAT_DECODE_VERSION( sourceVersion, major, minor, patch ); \
string = std::format( "{0}.{1}.{2}", major, minor, patch ); \
}

// Key
// A = Alpha ( BETA | ALPHA | PATCH )
// X = Release ( MAJOR | MINOR | PATCH )
constexpr auto SAT_VERSION_A_0_1_0 = SAT_MAKE_VERSION( 0, 1, 0 );
constexpr auto SAT_VERSION_A_0_1_1 = SAT_MAKE_VERSION( 0, 1, 1 );
constexpr auto SAT_VERSION_A_0_1_2 = SAT_MAKE_VERSION( 0, 1, 2 );
constexpr auto SAT_VERSION_A_0_1_3 = SAT_MAKE_VERSION( 0, 1, 3 );
constexpr auto SAT_VERSION_A_0_1_4 = SAT_MAKE_VERSION( 0, 1, 4 );
constexpr auto SAT_VERSION_A_0_2_0 = SAT_MAKE_VERSION( 0, 2, 0 );
constexpr auto SAT_VERSION_A_0_2_1 = SAT_MAKE_VERSION( 0, 2, 1 );
constexpr auto SAT_VERSION_A_0_2_2 = SAT_MAKE_VERSION( 0, 2, 2 );
constexpr auto SAT_VERSION_A_0_2_3 = SAT_MAKE_VERSION( 0, 2, 3 );
constexpr auto SAT_VERSION_A_0_2_4 = SAT_MAKE_VERSION( 0, 2, 4 );
constexpr auto SAT_VERSION_A_0_2_5 = SAT_MAKE_VERSION( 0, 2, 5 );
constexpr auto SAT_VERSION_A_0_2_6_WIP = SAT_MAKE_VERSION( 0, 2, 6 );

namespace Saturn::Core {

	inline void BreakDebug()
	{
	#if defined( _WIN32 )
		__debugbreak();
	#else
		raise( SIGTRAP );
	#endif // _MSC_VER
	}

}

constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 3u;

#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
#define SAT_SINGLETON_LAZY( x ) static inline x& Get() { return *SingletonStorage::GetOrCreateSingleton<x>(); }
#else
#define SAT_SINGLETON_LAZY( x ) static inline x& Get() { static x _; return _; }
#endif

// Inject asserts
#define __CORE_INCLUDED__
#include "Asserts.h"
#include "Verifies.h"

// Common includes
#include "Timestep.h"
#include "Ref.h"
#include "SingletonStorage.h"
#include "Platform.h"
