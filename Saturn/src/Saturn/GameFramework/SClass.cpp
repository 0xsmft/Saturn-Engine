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
#include "SClass.h"
#include "SProperty.h"

#include "Core/ClassMetadataHandler.h"

namespace Saturn {

	void SClass::RConstructClass( SClass** ppClass, const SClassSpecification& rSpec )
	{
		if( *ppClass != nullptr && ( ( *ppClass )->GetFlags() & SC_Initialised ) == 0 )
		{
			return;
		}

		// TODO: Objects will have their own names and Classes will have a different name
//		std::string configName = std::format( "^{0}", rSpec.Name );
		SClass* pNewClass = new SClass( rSpec );
		*ppClass = pNewClass;

		( *ppClass )->SetFlag( SC_Initialised );

		ClassMetadataHandler::Get().RegisterClass( *ppClass );
	}

	SProperty& SClass::GetProperty( const std::string& rPropertyName ) const
	{
		for( int i = 0; i < m_PropertyCount; i++ )
		{
			const SProperty* pProp = m_Properties[ i ];
			
			if( pProp->GetName() == rPropertyName )
			{
				return *( SProperty* ) pProp;
			}
		}

		static SProperty s_Empty;
		return s_Empty;
	}

}
