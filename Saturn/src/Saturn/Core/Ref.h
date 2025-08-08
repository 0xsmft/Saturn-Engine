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
#include <atomic>

namespace Saturn {

	/* 
	+----------------------------------------------------------------------------------------+
	| TYPE          | NAME       | USAGE       | LIMIATIONS									 |	
	+----------------------------------------------------------------------------------------+
	| INTRUSIVE     | Ref        | All Classes | No Weak Refs								 |
	| NON-INTRUSIVE | SharedPtr  | All Classes | More heap demand + use of virtual functions |
	| NON-INTRUSIVE | WeakRef    | All Classes | See above									 |
	| -				| -			 | -		   | -											 |
	| -				| -			 | -		   | -											 |
	+----------------------------------------------------------------------------------------+
	*/

	/*
	* NOTE: You may see throughout the Engine that we use Ref<Ty> and SharedPtr<Ty> in the same place,
	* The reason why is using Ref is the preferred way and should be picked over SharedPtr,
	* SharedPtr should only be used if we know that we are going to need to use a WeakRef or if we just want to reference count non-intrusively, or we know that an object could be used over different threads (as Ref is not thread safe atm)
	*/

	//////////////////////////////////////////////////////////////////////////
	// INTRUSIVE REFERENCE COUNTED OBJECTS

	// Base class for all intrusive reference counted objects
	class RefTarget
	{
	public:
		inline void AddRef() const
		{
			m_RefCount++;
		}

		inline void RemoveRef() const
		{
			m_RefCount--;
		}

		inline uint32_t GetRefCount() const { return m_RefCount; }

	private:
		// Maximum references for a single object is 4,294,967,296
		mutable unsigned int m_RefCount = 0;
	};

	// Ref is an intrusive, shared ownership, reference counted, smart pointer similar to std::shared_ptr
	// Intrusive refs do not support the usage of WeakRefs, you must use SharedPtr for that!
	// Refs take up less space than SharedPtrs, as the Ref class it self only needs 8 bytes (on 64 bit machines) for the pointer and then T needs an additional 4 bytes for the reference count bringing it to a total of 12 bytes but only 8 bytes per ref!
	//
	// NOTE: Any class that must use Ref must be a child of RefTarget
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
	};

	//////////////////////////////////////////////////////////////////////////
	// NON-INTRUSIVE REFERENCE COUNTED OBJECTS

	class __declspec(novtable) ReferenceControlBlockBase
	{
	protected:
		constexpr ReferenceControlBlockBase() noexcept = default;

	public:
		virtual ~ReferenceControlBlockBase() {}

		virtual void Destroy() = 0;

		void AddRef() 
		{
			_MT_INCR( m_RefCount );
		}

		void AddWeakRef()
		{
			_MT_INCR( m_WeakCount );
		}

		void DecRef() 
		{
			if( _MT_DECR( m_RefCount ) == 0 )
			{
				Destroy();
				// Check if we can delete this
				DecWeakRef();
			}
		} 

		void DecWeakRef()
		{
			if( _MT_DECR( m_WeakCount ) == 0 )
			{
				delete this;
			}
		}

		[[nodiscard]] long GetRefCount() const { return (long)m_RefCount; }
		[[nodiscard]] long GetWeakCount() const { return (long)m_WeakCount; }

	private:
		unsigned long m_RefCount{ 1 };
		unsigned long m_WeakCount{ 1 };
	};
	
	template<typename Ty>
	class ReferenceControlBlock final : public ReferenceControlBlockBase
	{
	public:
		ReferenceControlBlock( Ty* pClass ) 
			: m_pClass( pClass ) 
		{
		}

		ReferenceControlBlock( ReferenceControlBlock& ) = delete;
		ReferenceControlBlock& operator=( const ReferenceControlBlock& ) = delete;
		ReferenceControlBlock( ReferenceControlBlock&& ) = delete;
		ReferenceControlBlock& operator=( ReferenceControlBlock&& ) = delete;

		~ReferenceControlBlock() 
		{
			Destroy();
		}

		inline virtual void Destroy() override 
		{
			if( m_pClass )
			{
				delete m_pClass;
				m_pClass = nullptr;
			}
		}

		[[nodiscard]] Ty* GetClass() const { return m_pClass; }

	private:
		Ty* m_pClass = nullptr;
	};

	template<typename Ty, typename Deleter>
	class ReferenceControlBlockDeleter final : public ReferenceControlBlockBase
	{
	public:
		ReferenceControlBlockDeleter( Ty* pClass, Deleter d )
			: m_pClass( pClass ), m_Deleter( d )
		{
		}

		~ReferenceControlBlockDeleter() noexcept override = default;

		inline virtual void Destroy() override
		{
			if( m_pClass )
			{
				m_Deleter( m_pClass );
				m_pClass = nullptr;
			}
		}

	private:
		Ty* m_pClass = nullptr;
		Deleter m_Deleter;
	};

	template<typename Ty>
	class WeakRef;

	template<class Callee, class Args, class = void>
	struct IsSharedPtrDeleterViaible : std::false_type {};

	template<class Callee, class Args>
	struct IsSharedPtrDeleterViaible<Callee, Args, std::void_t<decltype(std::declval<Callee>()(std::declval<Args>()))>> : std::true_type {};

	template<typename Ty>
	class SharedPtr;

	template<typename Ty>
	class EnabledSharedFromThis
	{
	public:
		[[nodiscard]] SharedPtr<Ty> SharedFromThis() 
		{
			return m_Weak.Access();
		}

		[[nodiscard]] SharedPtr<const Ty> SharedFromThis() const 
		{
			return m_Weak.Access();
		}

		friend class SharedPtr<Ty>;

	protected:
		mutable WeakRef<Ty> m_Weak;
	};

	template<typename Ty, typename = void>
	struct IsEnableSharedFromThis : std::false_type {};

	template<typename Ty>
	struct IsEnableSharedFromThis<
		Ty, 
		std::void_t<decltype(std::declval<Ty&>().m_Weak)>> : std::true_type {};

	// Shared ownership of a single pointer, smart pointer, reference counted, 
	// works very similar to std::shared_ptr.
	//
	// SharedPtr is non-intrusive, thread safe and authoritative.
	template<typename Ty>
	class SharedPtr final
	{
	public:
		SharedPtr() noexcept = default;
		SharedPtr( std::nullptr_t ) noexcept {}

		SharedPtr( Ty* pPointer )
			: m_Pointer( pPointer ), m_pControlBlock( new ReferenceControlBlock<Ty>( pPointer ) )
		{
			// no need to inc ref count... it's already at one
			if constexpr( IsEnableSharedFromThis<Ty>::value )
			{
				pPointer->m_Weak = *this;
			}
		}

		template<typename Deleter>
		SharedPtr( Ty* pPointer, Deleter deleter )
			: m_Pointer( pPointer ), m_pControlBlock( new ReferenceControlBlockDeleter<Ty, Deleter>( pPointer, std::move( deleter ) ) )
		{
			// no need to inc ref count... it's already at one
			if constexpr( IsEnableSharedFromThis<Ty>::value )
			{
				pPointer->m_Weak = *this;
			}
		}

		// Copy from SharedPtr with the same type
		SharedPtr( const SharedPtr& rOther )
			: m_Pointer( ( Ty* ) rOther.m_Pointer ), m_pControlBlock( rOther.m_pControlBlock )
		{
			m_pControlBlock->AddRef();
		}

		// Copy from SharedPtr a different type
		template<typename Ty2>
		SharedPtr( const SharedPtr<Ty2>& rOther )
			: m_Pointer( ( Ty* ) rOther.m_Pointer ), m_pControlBlock( rOther.m_pControlBlock )
		{
			m_pControlBlock->AddRef();
		}

		// Move from SharedPtr with the same type
		SharedPtr( SharedPtr&& rrOther ) noexcept
		{
			MoveFrom( std::move( rrOther ) );
		}

		~SharedPtr() 
		{
			TryRelease();
		}

		//////////////////////////////////////////////////////////////////////////

		[[nodiscard]] Ty* Get() const { return m_Pointer; }
		Ty* operator->() const { return m_Pointer; }
		Ty& operator*() const { return *m_Pointer; }
		operator bool() const { return m_Pointer != nullptr; }

		SharedPtr<Ty>& operator=( std::nullptr_t )
		{
			TryRelease();

			m_Pointer = nullptr;
			m_pControlBlock = nullptr;

			return *this;
		}

		SharedPtr<Ty>& operator=( const SharedPtr<Ty>& rOther )
		{
			TryRelease();

			m_pControlBlock = rOther.m_pControlBlock;
			m_Pointer = rOther.m_Pointer;
			m_pControlBlock->AddRef();

			return *this;
		}

		template<typename T2>
		SharedPtr<T2>& operator=( const SharedPtr<T2>& rOther )
		{
			TryRelease();

			m_pControlBlock = rOther.m_pControlBlock;
			m_Pointer = rOther.m_Pointer;
			m_pControlBlock->AddRef();

			return *this;
		}

		SharedPtr<Ty>& operator=( SharedPtr<Ty>& rOther )
		{
			TryRelease();

			m_pControlBlock = rOther.m_pControlBlock;
			m_Pointer = rOther.m_Pointer;
			m_pControlBlock->AddRef();

			return *this;
		}

		template<typename T2>
		SharedPtr& operator=( SharedPtr<T2>& rOther )
		{
			TryRelease();

			m_pControlBlock = rOther.m_pControlBlock;
			m_Pointer = rOther.m_Pointer;
			m_pControlBlock->AddRef();

			return *this;
		}

		template<typename T2>
		SharedPtr& operator=( SharedPtr<T2>&& rrOther )
		{
			TryRelease();

			m_pControlBlock = rrOther.m_pControlBlock;
			m_Pointer = rrOther.m_Pointer;

			rrOther.m_pControlBlock = nullptr;
			rrOther.m_Pointer = nullptr;

			return *this;
		}

		bool operator==( const SharedPtr<Ty>& rOther ) const
		{
			return m_Pointer == rOther.m_Pointer;
		}

		bool operator!=( const SharedPtr<Ty>& rOther ) const
		{
			return m_Pointer != rOther.m_Pointer;
		}

		template <typename T2>
		[[nodiscard]] SharedPtr<T2> As() const
		{
			return SharedPtr<T2>( *this );
		}

	public:
		template<typename... VaArgs>
		[[nodiscard]] static SharedPtr<Ty> Create( VaArgs&&... rrArgs ) 
		{
			return SharedPtr<Ty>( new Ty( std::forward<VaArgs>( rrArgs )... ) );
		}

	protected:
		void ConstructWeak( const WeakRef<Ty>& weak ) 
		{
			m_Pointer = weak.m_pPointer;
			m_pControlBlock = weak.m_pControlBlock;

			m_pControlBlock->AddRef();
		}

	private:
		void TryRelease() 
		{
			if( m_pControlBlock )
			{
				m_pControlBlock->DecRef();
			}

			m_Pointer = nullptr;
			m_pControlBlock = nullptr;
		}

		template<typename T2>
		void MoveFrom( SharedPtr<T2>&& rrOther )
		{
			m_Pointer = rrOther.m_Pointer;
			m_pControlBlock = rrOther.m_pControlBlock;

			rrOther.m_pControlBlock = nullptr;
			rrOther.m_Pointer = nullptr;
		}

	private:
		Ty* m_Pointer = nullptr;
		ReferenceControlBlockBase* m_pControlBlock = nullptr;

	private:
		friend class SharedPtr;
		friend class WeakRef<Ty>;
	};

	// WeakRef, reference counted, works very similar to std::weak_ptr.
	// WeakRef is non-intrusive however, NOT authoritative.
	//
	// A Weak pointer can help break cyclic dependencies (which would otherwise be stopped by using a raw ptr or some workaround) they can also be used in places where we simply want to use the object (if it's alive) without owning it and stopping it from deletion.
	template<typename Ty>
	class WeakRef final
	{
	public:
		WeakRef() = default;
		
		// Construct from strong ref
		WeakRef( const SharedPtr<Ty>& rStrongRef )
			: m_pPointer( rStrongRef.m_Pointer ), m_pControlBlock( rStrongRef.m_pControlBlock )
		{
			TryAddWeakRef();
		}

		// copy
		WeakRef( const WeakRef<Ty>& rRef )
			: m_pPointer( rRef.m_pPointer ), m_pControlBlock( rRef.m_pControlBlock )
		{
			TryAddWeakRef();
		}

		// move
		WeakRef( WeakRef<Ty>&& rrOther ) noexcept
			: m_pPointer( rrOther.m_pPointer ), m_pControlBlock( rrOther.m_pControlBlock )
		{
			rrOther.m_pPointer = nullptr;
			rrOther.m_pControlBlock = nullptr;
		}

		~WeakRef() 
		{
			TryRelease();
		}

		SharedPtr<Ty> Access() const
		{
			if( !Expired() )
			{
				SharedPtr<Ty> strongRef;
				strongRef.ConstructWeak( *this );

				return strongRef;
			}

			return nullptr;
		}

		[[nodiscard]] bool Expired() const { return m_pControlBlock ? m_pControlBlock->GetRefCount() == 0 : true; }

		WeakRef& operator=( const SharedPtr<Ty>& rStrongRef )
		{
			TryRelease();

			m_pPointer = rStrongRef.m_Pointer;
			m_pControlBlock = rStrongRef.m_pControlBlock;

			TryAddWeakRef();

			return *this;
		}

		WeakRef& operator=( const WeakRef<Ty>& rOther )
		{
			if( this == &rOther )
				return *this;

			TryRelease();
			
			m_pPointer = rOther.m_pPointer;
			m_pControlBlock = rOther.m_pControlBlock;

			TryAddWeakRef();

			return *this;
		}

	private:
		inline void TryRelease()
		{
			if( m_pControlBlock )
			{
				m_pControlBlock->DecWeakRef();
			}

			m_pPointer = nullptr;
			m_pControlBlock = nullptr;
		}

		inline void TryAddWeakRef()
		{
			if( m_pControlBlock )
			{
				m_pControlBlock->AddWeakRef();
			}
		}

	private:
		Ty* m_pPointer = nullptr;
		ReferenceControlBlockBase* m_pControlBlock = nullptr;

	private:
		friend class WeakRef;
		friend class SharedPtr<Ty>;
	};

}
