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

#include <type_traits>

namespace Saturn {

	class RefTarget
	{
	public:
		void AddRef() const
		{
			m_RefCount++;
		}

		void RemoveRef() const
		{
			m_RefCount--;
		}

		void AddWeakRef() const
		{
			m_WeakRefCount++;
		}

		void RemoveWeakRef() const
		{
			m_WeakRefCount--;
		}

		uint32_t GetRefCount() const { return m_RefCount; }
		uint32_t GetWeakRefCount() const { return m_WeakRefCount; }

	private:
		mutable int m_RefCount = 0;
		mutable int m_WeakRefCount = 0;
	};
	
	template<typename T>
	class Ref;

	template<typename T>
	class WeakRef final
	{
		static_assert( std::is_base_of<RefTarget, T>::value, "T must be a child of RefTarget class!" );
	public:
		WeakRef() : m_Pointer( nullptr ) {}
		WeakRef( std::nullptr_t ) : m_Pointer( nullptr ) {}

		// Construct from strong ref
		WeakRef( const Ref<T>& rStrongRef ) 
			: m_Pointer( rStrongRef.m_Pointer )
		{
			AddWeakRef();
		}

		WeakRef( const WeakRef<T>& rRef )
			: m_Pointer( rRef.m_Pointer )
		{
			AddWeakRef();
		}

		WeakRef( const WeakRef<T>&& rrOther )
			: m_Pointer( rrOther.m_Pointer )
		{
			rrOther.m_Pointer = nullptr;
		}

		~WeakRef()
		{
			RemoveWeakRef();

			m_Pointer = nullptr;
		}

	public:
		WeakRef& operator=( std::nullptr_t )
		{
			RemoveWeakRef();

			m_Pointer = nullptr;
			
			return *this;
		}

		WeakRef& operator=( const Ref<T>& rStrongRef )
		{
			m_Pointer = rStrongRef.m_Pointer;
		
			AddWeakRef();

			return *this;
		}

		WeakRef& operator=( const WeakRef<T>& rOther )
		{
			if( *this == rOther )
				return;

			RemoveWeakRef();
			m_Pointer = rOther.m_Pointer;
			AddWeakRef();

			return *this;
		}

		WeakRef& operator=( const WeakRef<T>&& rrOther )
		{
			if( *this == rrOther )
				return;

			RemoveWeakRef();
			m_Pointer = rrOther.m_Pointer;
			rrOther.m_Pointer = nullptr;

			return *this;
		}

		bool operator ==( const Ref<T>& rOther ) const { return m_Pointer == rOther.m_Pointer; }
		bool operator !=( const Ref<T>& rOther ) const { return m_Pointer != rOther.m_Pointer; }

	public:
		Ref<T> Access() const 
		{
			if( !Expired() )
			{
				return Ref<T>( m_Pointer );
			}

			return nullptr;
		}

		bool Expired() const { return m_Pointer == nullptr || m_Pointer->GetRefCount() == 0; }

	private:
		void AddWeakRef() const 
		{
			if( m_Pointer )
				m_Pointer->AddWeakRef();
		}

		void RemoveWeakRef() const 
		{
			if( m_Pointer )
			{
				m_Pointer->RemoveWeakRef();
			}
		}

	private:
		mutable T* m_Pointer;

	private:
		friend class Ref<T>;
		friend class WeakRef;
	};

	template<typename T>
	class Ref final
	{
	public:
		Ref() : m_Pointer( nullptr ) {}
		Ref( std::nullptr_t ) : m_Pointer( nullptr ) {}
		Ref( T* pointer ) : m_Pointer( pointer ) { static_assert( std::is_base_of<RefTarget, T>::value, "T must be a child of RefTarget class!" ); AddRef(); }

		template<typename T2>
		Ref( const Ref<T2>& other ) { m_Pointer = ( T* ) other.m_Pointer; AddRef(); }

		Ref( const Ref<T>& other ) { m_Pointer = ( T* ) other.m_Pointer; AddRef(); }

		template<typename T2>
		Ref( Ref<T2>&& other )
		{
			m_Pointer = ( T* )other.m_Pointer;  
			other.m_Pointer = nullptr;
		}

		~Ref() 
		{ 
			RemoveRef();
		}

		[[deprecated("Saturn::Ref::Delete is deprecated and will be removed. Consider using \"Ref::Reset\" instead.")]]
		void Delete() 
		{
			RemoveRef();
		}

		void Reset()
		{
			RemoveRef();

			m_Pointer = nullptr;
		}

		template<typename... VaArgs>
		[[nodiscard]] static Ref<T> Create( VaArgs&&... args )
		{
			return Ref<T>( new T( std::forward<VaArgs>( args )... ) );
		}

	public:
	
		Ref& operator=( std::nullptr_t ) 
		{
			RemoveRef();
			
			m_Pointer = nullptr;

			return *this;
		}

		//////////////////////////////////////////////////////////////////////////

		Ref& operator=( Ref<T>& other )
		{
			other.AddRef();
			RemoveRef();

			m_Pointer = other.m_Pointer;

			return *this;
		}

		template<typename T2>
		Ref& operator=( Ref<T2>& other )
		{
			other.AddRef();
			RemoveRef();

			m_Pointer = other.m_Pointer;

			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		// CONST
		//////////////////////////////////////////////////////////////////////////

		Ref& operator=( const Ref<T>& other )
		{
			other.AddRef();
			RemoveRef();

			m_Pointer = other.m_Pointer;

			return *this;
		}

		template<typename T2>
		Ref& operator=( const Ref<T2>& other )
		{
			other.AddRef();
			RemoveRef();

			m_Pointer = other.m_Pointer;

			return *this;
		}

		//////////////////////////////////////////////////////////////////////////

		template<typename T2>
		Ref& operator=( Ref<T2>&& other )
		{
			RemoveRef();

			m_Pointer = other.m_Pointer;
			other.m_Pointer = nullptr;

			return *this;
		}

		//////////////////////////////////////////////////////////////////////////

		operator bool()       { return m_Pointer != nullptr; }
		operator bool() const { return m_Pointer != nullptr; }

		bool operator ==( const Ref<T>& rOther ) const { return m_Pointer == rOther.m_Pointer; }
		bool operator !=( const Ref<T>& rOther ) const { return m_Pointer != rOther.m_Pointer; }

		T* operator->()             { return m_Pointer; }
		const T* operator->() const { return m_Pointer; }

		T& operator*()             { return *m_Pointer; }
		const T& operator*() const { return *m_Pointer; }

		T* Get()             { return m_Pointer; }
		const T* Get() const { return m_Pointer; }

		template <typename T2>
		[[nodiscard]] Ref<T2> As() const
		{
			return Ref<T2>( *this );
		}

	private:

		void AddRef() const
		{
			if( m_Pointer )
				m_Pointer->AddRef();
		}

		void RemoveRef() const
		{
			if( m_Pointer ) 
			{
				m_Pointer->RemoveRef();

				if( m_Pointer->GetRefCount() == 0 ) 
				{
					delete m_Pointer;
					m_Pointer = nullptr;
				}
			}
		}

	private:

		mutable T* m_Pointer;

	private:
		// Fix cannot access private member declared in class
		friend class Ref;
		friend class WeakRef<T>;
	};

}