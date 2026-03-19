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

#include "Saturn/Core/Buffer.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>

namespace Saturn {

	class JoltBinaryReader : public JPH::StreamIn
	{
	public:
		JoltBinaryReader( const Buffer& rBuffer ) : m_pBuffer( &rBuffer ) {}
		~JoltBinaryReader()
		{
			m_pBuffer = nullptr;
			m_BytesRead = 0llu;
		}

		virtual void ReadBytes( void* pOutData, size_t numBytes ) override
		{
			std::memcpy( pOutData, ( ( uint8_t* ) m_pBuffer->Data ) + m_BytesRead, numBytes );
			m_BytesRead += numBytes;
		}

		virtual bool IsEOF() const override
		{
			return m_pBuffer == nullptr || m_BytesRead > m_pBuffer->Size;
		}

		virtual bool IsFailed() const override
		{
			return m_pBuffer == nullptr || m_pBuffer->Data == nullptr || m_pBuffer->Size == 0;
		}

	private:
		const Buffer* m_pBuffer = nullptr;
		uint64_t m_BytesRead = 0llu;
	};

	class JoltBinaryWriter : public JPH::StreamOut
	{
	public:
		virtual void WriteBytes( const void* pData, size_t numBytes ) override
		{
			const size_t currentSize = m_TemporaryBuffer.size();
		
			m_TemporaryBuffer.resize( currentSize + numBytes );

			std::memcpy( m_TemporaryBuffer.data() + currentSize, pData, numBytes );
		}

		virtual bool IsFailed() const override
		{
			return false;
		}

		Buffer ToBuffer() const 
		{
			return Buffer::Copy( m_TemporaryBuffer.data(), m_TemporaryBuffer.size() );
		}

	private:
		std::vector<uint8_t*> m_TemporaryBuffer;
	};

}
