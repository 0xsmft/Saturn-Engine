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

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ref.h"

namespace Saturn {

	class NodeRuntime : public RefTarget
	{
	public:
		explicit NodeRuntime( UUID id ) { m_NodeID = std::move( id ); }

		virtual void EvaluateNode() = 0;

	protected:
		UUID m_NodeID = 0;
	};

	class GetAssetRuntime : public NodeRuntime
	{
	public:
		explicit GetAssetRuntime( UUID id ) : NodeRuntime( id ) {}

		virtual void EvaluateNode() override {}

	public:
		uint64_t Value = 0;
	};

	template<typename N>
	class AddValueRuntime : public NodeRuntime
	{
	public:
		explicit AddValueRuntime( UUID id ) : NodeRuntime( id ) {}

		virtual void EvaluateNode() override 
		{
			Result = Value1 + Value2;

			OutPin->ExtraData.Write( ( uint8_t* ) &Result, sizeof( N ), 0 );
		}

	public:
		N Value1 = N();
		N Value2 = N();
		N Result = N();

		Ref<Pin> OutPin = nullptr;
	};

	template<typename N>
	class SubtractValueRuntime : public NodeRuntime
	{
	public:
		explicit SubtractValueRuntime( UUID id ) : NodeRuntime( id ) {}

		virtual void EvaluateNode() override 
		{
			Result = Value1 - Value2;

			OutPin->ExtraData.Write( ( uint8_t* ) &Result, sizeof( N ), 0 );
		}

	public:
		N Value1 = N();
		N Value2 = N();
		N Result = N();

		Ref<Pin> OutPin = nullptr;
	};

	template<typename N>
	class MultiplyValueRuntime : public NodeRuntime
	{
	public:
		explicit MultiplyValueRuntime( UUID id ) : NodeRuntime( id ) {}

		virtual void EvaluateNode() override 
		{
			Result = Value1 * Value2;

			OutPin->ExtraData.Write( ( uint8_t* ) &Result, sizeof( N ), 0 );
		}

	public:
		N Value1 = N();
		N Value2 = N();
		N Result = N();

		Ref<Pin> OutPin = nullptr;
	};

	template<typename N>
	class DivideValueRuntime : public NodeRuntime
	{
	public:
		explicit DivideValueRuntime( UUID id ) : NodeRuntime( id ) {}

		virtual void EvaluateNode() override 
		{
			Result = Value1 / Value2;

			OutPin->ExtraData.Write( ( uint8_t* ) &Result, sizeof( N ), 0 );
		}

	public:
		N Value1 = N();
		N Value2 = N();
		N Result = N();

		Ref<Pin> OutPin = nullptr;
	};

	using AddFloatRuntime = AddValueRuntime<float>;
	using AddIntRuntime = AddValueRuntime<int>;
	using AddDoubleRuntime = AddValueRuntime<double>;

	using SubtractFloatRuntime = SubtractValueRuntime<float>;
	using SubtractIntRuntime = SubtractValueRuntime<int>;
	using SubtractDoubleRuntime = SubtractValueRuntime<double>;

	using MultiplyFloatRuntime = MultiplyValueRuntime<float>;
	using MultiplyIntRuntime = MultiplyValueRuntime<int>;
	using MultiplyDoubleRuntime = MultiplyValueRuntime<double>;

	using DivideFloatRuntime = DivideValueRuntime<float>;
	using DivideIntRuntime = DivideValueRuntime<int>;
	using DivideDoubleRuntime = DivideValueRuntime<double>;
}