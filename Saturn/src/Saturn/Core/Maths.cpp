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
#include "Maths.h"

#include <glm/gtx/matrix_decompose.hpp>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Saturn::Maths {

	//////////////////////////////////////////////////////////////////////////
	// DecomposeTransform with a vector3 as rotation.

	bool DecomposeTransform( const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale )
	{
		using namespace glm;
		using T = float;

		mat4 LocalMatrix( transform );

		// Normalize the matrix.
		if( epsilonEqual( LocalMatrix[ 3 ][ 3 ], static_cast< float >( 0 ), epsilon<T>() ) )
			return false;

		// First, isolate perspective.  This is the messiest.
		if(
			epsilonNotEqual( LocalMatrix[ 0 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) ||
			epsilonNotEqual( LocalMatrix[ 1 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) ||
			epsilonNotEqual( LocalMatrix[ 2 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) )
		{
			// Clear the perspective partition
			LocalMatrix[ 0 ][ 3 ] = LocalMatrix[ 1 ][ 3 ] = LocalMatrix[ 2 ][ 3 ] = static_cast< T >( 0 );
			LocalMatrix[ 3 ][ 3 ] = static_cast< T >( 1 );
		}

		// Next take care of translation (easy).
		translation = vec3( LocalMatrix[ 3 ] );
		LocalMatrix[ 3 ] = vec4( 0, 0, 0, LocalMatrix[ 3 ].w );

		vec3 Row[ 3 ];

		// Now get scale and shear.
		for( length_t i = 0; i < 3; ++i )
			for( length_t j = 0; j < 3; ++j )
				Row[ i ][ j ] = LocalMatrix[ i ][ j ];

		// Compute X scale factor and normalize first row.
		scale.x = length( Row[ 0 ] );
		Row[ 0 ] = detail::scale( Row[ 0 ], static_cast< T >( 1 ) );
		scale.y = length( Row[ 1 ] );
		Row[ 1 ] = detail::scale( Row[ 1 ], static_cast< T >( 1 ) );
		scale.z = length( Row[ 2 ] );
		Row[ 2 ] = detail::scale( Row[ 2 ], static_cast< T >( 1 ) );

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross( Row[ 1 ], Row[ 2 ] ); // v3Cross(row[1], row[2], Pdum3);
		if( dot( Row[ 0 ], Pdum3 ) < 0 )
		{
			for( length_t i = 0; i < 3; i++ )
			{
				scale[ i ] *= static_cast< T >( -1 );
				Row[ i ] *= static_cast< T >( -1 );
			}
		}
#endif

		rotation.y = asin( -Row[ 0 ][ 2 ] );
		if( cos( rotation.y ) != 0 ) {
			rotation.x = atan2( Row[ 1 ][ 2 ], Row[ 2 ][ 2 ] );
			rotation.z = atan2( Row[ 0 ][ 1 ], Row[ 0 ][ 0 ] );
		}
		else {
			rotation.x = atan2( -Row[ 2 ][ 0 ], Row[ 1 ][ 1 ] );
			rotation.z = 0;
		}


		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// DecomposeTransform with a quaternion.

	bool DecomposeTransform( const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale )
	{
		using namespace glm;
		using T = float;

		mat4 LocalMatrix( transform );

		// Normalize the matrix.
		if( epsilonEqual( LocalMatrix[ 3 ][ 3 ], static_cast< float >( 0 ), epsilon<T>() ) )
			return false;

		// First, isolate perspective.  This is the messiest.
		if(
			epsilonNotEqual( LocalMatrix[ 0 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) ||
			epsilonNotEqual( LocalMatrix[ 1 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) ||
			epsilonNotEqual( LocalMatrix[ 2 ][ 3 ], static_cast< T >( 0 ), epsilon<T>() ) )
		{
			// Clear the perspective partition
			LocalMatrix[ 0 ][ 3 ] = LocalMatrix[ 1 ][ 3 ] = LocalMatrix[ 2 ][ 3 ] = static_cast< T >( 0 );
			LocalMatrix[ 3 ][ 3 ] = static_cast< T >( 1 );
		}

		// Next take care of translation (easy).
		translation = vec3( LocalMatrix[ 3 ] );
		LocalMatrix[ 3 ] = vec4( 0, 0, 0, LocalMatrix[ 3 ].w );

		vec3 Row[ 3 ];

		// Now get scale and shear.
		for( length_t i = 0; i < 3; ++i )
			for( length_t j = 0; j < 3; ++j )
				Row[ i ][ j ] = LocalMatrix[ i ][ j ];

		// Compute X scale factor and normalize first row.
		scale.x = length( Row[ 0 ] );
		Row[ 0 ] = detail::scale( Row[ 0 ], static_cast< T >( 1 ) );
		scale.y = length( Row[ 1 ] );
		Row[ 1 ] = detail::scale( Row[ 1 ], static_cast< T >( 1 ) );
		scale.z = length( Row[ 2 ] );
		Row[ 2 ] = detail::scale( Row[ 2 ], static_cast< T >( 1 ) );

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross( Row[ 1 ], Row[ 2 ] ); // v3Cross(row[1], row[2], Pdum3);
		if( dot( Row[ 0 ], Pdum3 ) < 0 )
		{
			for( length_t i = 0; i < 3; i++ )
			{
				scale[ i ] *= static_cast< T >( -1 );
				Row[ i ] *= static_cast< T >( -1 );
			}
		}
#endif

		int i, j, k = 0;
		T root, trace = Row[ 0 ].x + Row[ 1 ].y + Row[ 2 ].z;
		if( trace > static_cast< T >( 0 ) )
		{
			root = sqrt( trace + static_cast< T >( 1.0 ) );
			rotation.w = static_cast< T >( 0.5 ) * root;
			root = static_cast< T >( 0.5 ) / root;
			rotation.x = root * ( Row[ 1 ].z - Row[ 2 ].y );
			rotation.y = root * ( Row[ 2 ].x - Row[ 0 ].z );
			rotation.z = root * ( Row[ 0 ].y - Row[ 1 ].x );
		} // End if > 0
		else
		{
			static int Next[ 3 ] = { 1, 2, 0 };
			i = 0;
			if( Row[ 1 ].y > Row[ 0 ].x ) i = 1;
			if( Row[ 2 ].z > Row[ i ][ i ] ) i = 2;
			j = Next[ i ];
			k = Next[ j ];

#           ifdef GLM_FORCE_QUAT_DATA_XYZW
			int off = 0;
#           else
			int off = 1;
#           endif

			root = sqrt( Row[ i ][ i ] - Row[ j ][ j ] - Row[ k ][ k ] + static_cast< T >( 1.0 ) );

			rotation[ i + off ] = static_cast< T >( 0.5 ) * root;
			root = static_cast< T >( 0.5 ) / root;
			rotation[ j + off ] = root * ( Row[ i ][ j ] + Row[ j ][ i ] );
			rotation[ k + off ] = root * ( Row[ i ][ k ] + Row[ k ][ i ] );
			rotation.w = root * ( Row[ j ][ k ] - Row[ k ][ j ] );
		} // End if <= 0

		return true;
	}
}