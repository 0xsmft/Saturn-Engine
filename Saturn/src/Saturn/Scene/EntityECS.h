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

namespace Saturn {

	template<typename T, typename... Args>
	T& Entity::AddComponent( Args&&... args )
	{
		return m_Scene->AddComponent<T>( m_EntityHandle, std::forward<Args>( args )... );
	}

	template<typename T>
	[[nodiscard]] T& Entity::GetComponent()
	{
		return m_Scene->GetComponent<T>( m_EntityHandle );
	}

	template<typename T>
	[[nodiscard]] const T& Entity::GetComponent() const
	{
		return m_Scene->GetComponent<T>( m_EntityHandle );
	}

	template<typename T>
	[[nodiscard]] bool Entity::HasComponent() const
	{
		return m_Scene->HasComponent<T>( m_EntityHandle );
	}

	template<typename... T>
	[[nodiscard]] bool Entity::HasComponents() const
	{
		return m_Scene->template HasComponents<T...>( m_EntityHandle );
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		m_Scene->RemoveComponent<T>( m_EntityHandle );
	}

	template<typename... T>
	void Entity::RemoveComponents()
	{
		m_Scene->template RemoveComponents<T...>( m_EntityHandle );
	}

	template<typename T>
	[[nodiscard]] T* Entity::TryGetComponent()
	{
		return m_Scene->TryGetComponent<T>( m_EntityHandle );
	}

	template<typename T>
	[[nodiscard]] const T* Entity::TryGetComponent() const
	{
		return m_Scene->TryGetComponent<T>( m_EntityHandle );
	}

}
