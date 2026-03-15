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

#include "Saturn/GameFramework/SClass.h"

// Register a spawnable class
#define SAT_X31_CREATE_AUTO_REG_SPWN( ClassName )																\
static Saturn::SClass* RStaticLnk()																				\
{																												\
	static Saturn::SClass* pClass = nullptr;																	\
	if( !pClass )																								\
	{																											\
		const auto spec = Saturn::SClassSpecification{ #ClassName, (Saturn::SClassFlags) Saturn::SC_Spawnable | Saturn::SC_VisibleInEditor | Saturn::SC_NoExtendedMetadata, 0, sizeof( Saturn::ClassName ), alignof( Saturn::ClassName ), Saturn::FNV1A64( #ClassName ), Saturn::ClassName::Super::StaticClass(), Saturn::RInternalConstructor<Saturn::ClassName>, RStaticLnk, nullptr, {} };\
		Saturn::SClass::RConstructClass( pClass, spec );														\
	}																											\
																												\
	return pClass;																								\
}																												\
Saturn::SClass* Saturn::ClassName::GetStaticClassInternal()														\
{																												\
	return RStaticLnk();																						\
}																												\
static Saturn::SClassRegistrar RCR##ClassName( RStaticLnk )

// Register a non-spawnable class
#define SAT_X31_CREATE_AUTO_REG( ClassName )																	\
static Saturn::SClass* RStaticLnk##ClassName()																	\
{																												\
	static Saturn::SClass* pClass = nullptr;																	\
	if( !pClass )																								\
	{																											\
		const auto spec = Saturn::SClassSpecification{ #ClassName, (Saturn::SClassFlags) Saturn::SC_VisibleInEditor | Saturn::SC_NoExtendedMetadata, 0, sizeof( Saturn::ClassName ), alignof( Saturn::ClassName ), Saturn::FNV1A64( #ClassName ), Saturn::ClassName::Super::StaticClass(), Saturn::RInternalConstructor<Saturn::ClassName>, RStaticLnk##ClassName, nullptr, {} };\
		Saturn::SClass::RConstructClass( pClass, spec );														\
	}																											\
																												\
	return pClass;																								\
}																												\
Saturn::SClass* Saturn::ClassName::GetStaticClassInternal()														\
{																												\
	return RStaticLnk##ClassName();																				\
}																												\
static Saturn::SClassRegistrar RCR##ClassName( RStaticLnk##ClassName )
