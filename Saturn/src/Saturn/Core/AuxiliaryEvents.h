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

#include "Event.h"

namespace Saturn {
	
	//
	// OnRequestRemoveApplicationLayer
	// 
	// This event should only be used if you need to remove
	// a layer whilst you are updating another layer.
	// This is because you cannot remove a layer while the
	// Application is updating all of the current layers.
	// 
	// When the application removes the event
	// a new event will be dispatched _immediately_ 
	// and that will be the reply of this event.
	// 
	// Otherwise, just use Application::PopLayer.
	//
	class OnRequestRemoveApplicationLayer : public Event
	{
		SAT_DEFINE_EVENT( RequestRemoveLayer, EC_Auxiliary );
	public:
		// Pointer to the layer to remove.
		OnRequestRemoveApplicationLayer( class Layer* pLayer ) 
			: Event( EventType::RequestRemoveLayer, EC_Auxiliary ), m_pLayer( pLayer )
		{
		}

		virtual ~OnRequestRemoveApplicationLayer() = default;

	public:
		Layer* GetLayer() const { return m_pLayer; }

	private:
		Layer* m_pLayer;
	};
	
	//
	// Reply event of OnRequestRemoveApplicationLayer
	//
	class OnRequestRemoveApplicationLayerReply : public Event
	{
		SAT_DEFINE_EVENT( RequestRemoveLayerReply, EC_Auxiliary );
	public:
		// Pointer to the layer that was removed.
		OnRequestRemoveApplicationLayerReply( class Layer* pLayer )
			: Event( EventType::RequestRemoveLayerReply, EC_Auxiliary ), m_pLayer( pLayer )
		{
		}

		virtual ~OnRequestRemoveApplicationLayerReply() = default;

	public:
		Layer* GetLayer() const { return m_pLayer; }

	private:
		Layer* m_pLayer;
	};
}
