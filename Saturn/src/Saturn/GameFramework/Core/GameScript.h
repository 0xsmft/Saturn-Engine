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

#include "Saturn/GameFramework/SAutoCompletionAux.h"

#define SCLASS(...)
#define SPROPERTY(...)

#define SAT_CONTACT_FOUR_(x,y,z,w) x##y##z##w
#define SAT_CONTACT_FOUR(x,y,z,w) SAT_CONTACT_FOUR_(x,y,z,w)

// Include a empty macro, this will be defined when the header tool is running.
#define CURRENT_FILE_ID
#define GENERATED_BODY(...) SAT_CONTACT_FOUR(CURRENT_FILE_ID,_,__LINE__,_GENERATED_BODY);

#define SAT_DECLARE_CLASS_EXTERNAL_BASE_NO_INT( x ) \
private: \
	x& operator=(x&&); \
	x& operator=(const x&); \
	static SClass* GetStaticClassInternal(); \
public: \
	inline static SObject* X31_DefConstructor() { return static_cast< SObject* >( new x ); } \
	inline static [[nodiscard]] SClass* StaticClass() { return GetStaticClassInternal(); } \
public: \
	typedef x ThisClass; \

#define SAT_DECLARE_CLASS( x, BaseClass ) \
private: \
	x& operator=(x&&); \
	x& operator=(const x&); \
	friend class x##Int; \
	static SClass* GetStaticClassInternal(); \
public: \
	inline static SObject* X31_DefConstructor() { return static_cast< SObject* >( new x ); } \
	inline static [[nodiscard]] SClass* StaticClass() { return GetStaticClassInternal(); } \
public: \
	typedef x ThisClass; \
	typedef BaseClass Super; \
	typedef x##Int InternalClass; \

#define SAT_DECLARE_CLASS_NO_INTER( x, BaseClass ) \
private: \
	x& operator=(x&&); \
	x& operator=(const x&); \
public: \
	inline static SObject* X31_DefConstructor() { return static_cast< SObject* >( new x ); } \
	inline static [[nodiscard]] SClass* StaticClass() { return nullptr; } \
public: \
	typedef x ThisClass; \
	typedef BaseClass Super; \

#define SAT_DECLARE_CLASS_MOVE( x, BaseClass ) \
private: \
	friend class x##Int;\
	static SClass* GetStaticClassInternal(); \
public: \
	inline static SObject* X31_DefConstructor() { return static_cast< SObject* >( new x ); } \
	inline static [[nodiscard]] SClass* StaticClass() { return GetStaticClassInternal(); } \
public: \
	typedef x ThisClass; \
	typedef BaseClass Super; \
	typedef x##Int InternalClass; \
