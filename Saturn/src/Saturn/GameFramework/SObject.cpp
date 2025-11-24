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
#include "SObject.h"

#include "Core/EngineGenerated.h"

static Saturn::SClass* RStaticLnkSObject()
{
	static Saturn::SClass* pClass = nullptr; 
	if( !pClass ) 
	{
		const Saturn::SClassSpecification spec{
			"SObject",
			( Saturn::SClassFlags ) Saturn::SC_None | Saturn::SC_NoExtendedMetadata, 0, 
			sizeof( Saturn::SObject ), alignof( Saturn::SObject ), 
			Saturn::FNV1A64( "SObject" ), 
			nullptr, Saturn::RInternalConstructor<Saturn::SObject>, RStaticLnkSObject, nullptr
		}; 
		
		Saturn::SClass::RConstructClass( pClass, spec );
	} 
	
	return pClass;
} 

Saturn::SClass* Saturn::SObject::GetStaticClassInternal() 
{
	return RStaticLnkSObject();
}

static Saturn::SClassRegistrar RCRSObject( RStaticLnkSObject );
