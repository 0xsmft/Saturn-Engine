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

#include "sppch.h"
#include "RubyCocoaBackend.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/Vulkan/Texture.h"

#if defined( SAT_RBY_INCLUDE_VULKAN )
#include <vulkan_metal.h>
#endif

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import "RubyNSApp.h"

@interface RubyMacWindow : NSWindow {}
@end

@implementation RubyMacWindow

- (BOOL)canBecomeKeyWindow
{
    // Required for NSWindowStyleMaskBorderless windows
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end

@interface RubyWindowNotificationMgr : NSObject
{
    Saturn::RubyCocoaBackend* pThis;
}

- (instancetype)initForRuby:(Saturn::RubyCocoaBackend*)pBackend;

@end

@interface RubyEventResponder : NSView<NSTextInputClient>
{
    Saturn::RubyCocoaBackend* pThis;
}

- (instancetype)initForRuby:(Saturn::RubyCocoaBackend*)initEventResponder;

@end

namespace Saturn {
    struct RubyMacOSData
    {
        RubyMacWindow* m_pWindow = nil;
        RubyEventResponder* m_pView = nil;
        RubyWindowNotificationMgr* m_pNotificationMgr = nil;
        CAMetalLayer* m_pMetalLayer = nil;
    };
}

@implementation RubyEventResponder

- (instancetype)initForRuby:(Saturn::RubyCocoaBackend*)initEventResponder
{
    self = [super init];
    if (self != nil)
    {
        pThis = initEventResponder;

        [self updateTrackingAreas];
        [self registerForDraggedTypes:@[NSPasteboardTypeURL]];
    }

    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

- (void)mouseMoved:(NSEvent*)event
{
    const NSPoint position = [event locationInWindow];
    const NSRect contentRect = [pThis->GetData()->m_pView frame];

    pThis->GetParent()->DispatchEvent<Saturn::RubyMouseMoveEvent>( Saturn::EventType::MouseMoved, ( float ) position.x, contentRect.size.height - ( float ) position.y );
}

- (void)mouseDown:(NSEvent*)event
{
	pThis->GetParent()->IntrnlSetMouseState( Saturn::RubyMouseButton_Left, true );
	pThis->GetParent()->DispatchEvent<Saturn::RubyMouseEvent>( Saturn::EventType::MousePressed, ( int )Saturn::RubyMouseButton_Left );
}

- (void)mouseUp:(NSEvent*)event
{
	pThis->GetParent()->IntrnlSetMouseState( Saturn::RubyMouseButton_Left, false );
	pThis->GetParent()->DispatchEvent<Saturn::RubyMouseEvent>( Saturn::EventType::MouseReleased, ( int )Saturn::RubyMouseButton_Left );
}

- (void)rightMouseDown:(NSEvent*)event
{
	pThis->GetParent()->IntrnlSetMouseState( Saturn::RubyMouseButton_Right, true );
	pThis->GetParent()->DispatchEvent<Saturn::RubyMouseEvent>( Saturn::EventType::MousePressed, ( int )Saturn::RubyMouseButton_Right );
}

- (void)rightMouseUp:(NSEvent*)event
{
	pThis->GetParent()->IntrnlSetMouseState( Saturn::RubyMouseButton_Right, false );
	pThis->GetParent()->DispatchEvent<Saturn::RubyMouseEvent>( Saturn::EventType::MouseReleased, ( int )Saturn::RubyMouseButton_Right );
}

- (void)mouseEntered:(NSEvent*)event
{
    pThis->GetParent()->DispatchEvent<Saturn::Event>( Saturn::EventType::MouseEnterWindow, Saturn::EventCategory::EC_Ruby );
}

- (void)mouseExited:(NSEvent*)event
{
    pThis->GetParent()->DispatchEvent<Saturn::Event>( Saturn::EventType::MouseLeaveWindow, Saturn::EventCategory::EC_Ruby );
}

static int TranslateMacOSModifiers(NSUInteger flags) 
{
    int mods = Saturn::RubyKey_UnknownKey;

    if( flags & NSEventModifierFlagShift )
        mods |= Saturn::RubyKey_LeftShift;

    if( flags & NSEventModifierFlagCommand )
        mods |= Saturn::RubyKey_LeftCtrl;

    if( flags & NSEventModifierFlagOption )
        mods |= Saturn::RubyKey_LeftAlt;

    if( flags & NSEventModifierFlagControl )
        mods |= Saturn::RubyKey_LeftCtrl;

    return mods;
}

static Saturn::RubyKey ConvertMacOSVkToRuby( uint16_t vk ) 
{
    using namespace Saturn;

    switch( vk ) 
    {
        default: return RubyKey_UnknownKey;

        case 0x0: return RubyKey_A;
    }
}

- (void) keyDown:(NSEvent*) event 
{
    const Saturn::RubyKey saturnKey = ConvertMacOSVkToRuby( [event keyCode] );
    const int Modifiers = TranslateMacOSModifiers( [event modifierFlags] );

	pThis->GetParent()->IntrnlSetKeyDown( saturnKey, true );
	pThis->GetParent()->DispatchEvent<Saturn::RubyKeyEvent>( Saturn::EventType::KeyPressed, saturnKey, [event keyCode], Modifiers );
}

- (void) keyUp:(NSEvent*) event 
{
    const Saturn::RubyKey saturnKey = ConvertMacOSVkToRuby( [event keyCode] );
    const int Modifiers = TranslateMacOSModifiers( [event modifierFlags] );

	pThis->GetParent()->IntrnlSetKeyDown( saturnKey, false );
	pThis->GetParent()->DispatchEvent<Saturn::RubyKeyEvent>( Saturn::EventType::KeyReleased, saturnKey, [event keyCode], Modifiers );
}

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
    return @[];
}

@end

@implementation RubyWindowNotificationMgr

- (instancetype)initForRuby:(Saturn::RubyCocoaBackend*)pBackend
{
    self = [super init];
    if (self != nil)
        pThis = pBackend;

    return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    pThis->GetParent()->DispatchEvent<Saturn::Event>( Saturn::EventType::Close, Saturn::EventCategory::EC_Ruby );
	pThis->CloseWindow();

    return NO;
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    pThis->GetParent()->DispatchEvent<Saturn::RubyFocusEvent>( Saturn::EventType::WindowFocus, true );
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    pThis->GetParent()->DispatchEvent<Saturn::RubyFocusEvent>( Saturn::EventType::WindowFocus, false );
}

@end

namespace Saturn {

	RubyCocoaBackend::RubyCocoaBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow )
	{
		m_WindowSpecification = rSpec;
		m_pWindow = pWindow;
        m_pData = new RubyMacOSData();
	}

    static NSWindowStyleMask ChooseStyle( RubyStyle rubyStyle ) 
    {
        switch( rubyStyle )
        {
            default:
            case RubyStyle::Default:
            {
                return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;   
            };

            case RubyStyle::Borderless:
            {
                return NSWindowStyleMaskBorderless | NSWindowStyleMaskResizable;
            };

            case RubyStyle::BorderlessFullscreen:
            case RubyStyle::BorderlessNoResize:
            {
                return NSWindowStyleMaskBorderless;
            };
        }
    }

	void RubyCocoaBackend::Create()
	{
        NSRect frame = NSMakeRect( 0, 0, m_WindowSpecification.Width, m_WindowSpecification.Height );
        const auto style = ChooseStyle( m_WindowSpecification.Style );

        m_pData->m_pNotificationMgr = [[RubyWindowNotificationMgr alloc] initForRuby:this];

        m_pData->m_pWindow =
            [[RubyMacWindow alloc]
                initWithContentRect:frame
                styleMask:style
                backing:NSBackingStoreBuffered
                defer:NO];

        NSString* nsTitle =
            [[NSString alloc]
                initWithBytes:m_WindowSpecification.Name.data()
                    length:m_WindowSpecification.Name.size() * sizeof(wchar_t)
                    encoding:NSUTF32LittleEndianStringEncoding];

        [m_pData->m_pWindow setTitle:nsTitle];

        // Create the view.
        m_pData->m_pView = [[RubyEventResponder alloc]
            initForRuby:this];

        m_pData->m_pView.frame =
            [[m_pData->m_pWindow contentView] bounds];

        // Make the view resize with the window.
        [m_pData->m_pView setAutoresizingMask:
            NSViewWidthSizable | NSViewHeightSizable];

        [m_pData->m_pWindow setContentView:m_pData->m_pView];
        [m_pData->m_pWindow makeFirstResponder:m_pData->m_pView];
        [m_pData->m_pWindow setDelegate:m_pData->m_pNotificationMgr];
        [m_pData->m_pWindow setAcceptsMouseMovedEvents:YES];
        [m_pData->m_pWindow setRestorable:NO];

        m_pData->m_pMetalLayer = [CAMetalLayer layer];
        [m_pData->m_pView setWantsLayer:YES];
        [m_pData->m_pView setLayer:m_pData->m_pMetalLayer];

        if( m_WindowSpecification.ShowNow )
            PresentWindow();
	}

	RubyCocoaBackend::~RubyCocoaBackend()
	{
        delete m_pData;
	}

	void RubyCocoaBackend::PollEvents()
	{
        @autoreleasepool {
        while(true) 
        {
            NSEvent* pEvent = [NSApp nextEventMatchingMask:NSEventMaskAny
                                    untilDate:[NSDate distantPast]
                                    inMode:NSDefaultRunLoopMode
                                    dequeue:YES];

            if (pEvent == nil)
                break;

            [NSApp sendEvent:pEvent];
        }
        }
	}

	void RubyCocoaBackend::Maximize()
	{
        [m_pData->m_pWindow zoom:nil];
	}

	void RubyCocoaBackend::Minimize()
	{
        if (![m_pData->m_pWindow isMiniaturized])
            [m_pData->m_pWindow miniaturize:nil];
	}

	void RubyCocoaBackend::Restore()
	{
        if( [m_pData->m_pWindow isZoomed] )
            [m_pData->m_pWindow zoom:nil];
	}

	bool RubyCocoaBackend::Minimized()
	{
		return [m_pData->m_pWindow isMiniaturized];
	}

	bool RubyCocoaBackend::Maximized()
	{
		return [m_pData->m_pWindow isZoomed];
	}

	bool RubyCocoaBackend::Focused()
    {
		return [m_pData->m_pWindow isVisible] && [ m_pData->m_pWindow isKeyWindow];
	}

	WindowType RubyCocoaBackend::GetNativeHandle()
	{
		return ( WindowType ) m_pData->m_pWindow;
	}

	void RubyCocoaBackend::DestroyWindow()
	{
	}

	void RubyCocoaBackend::CloseWindow()
	{
        [m_pData->m_pWindow close];
        m_WindowClosed = true;
	}

	void RubyCocoaBackend::PresentWindow( RubyWindowShowCmd Command /*= RubyWindowShowCmd::Default */ )
	{
        [m_pData->m_pWindow orderFront:nil];
	}

	void RubyCocoaBackend::HideWindow()
	{
        [m_pData->m_pWindow orderOut:nil];
	}

	void RubyCocoaBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
	}

	RubyIVec2 RubyCocoaBackend::GetSize()
	{
        NSRect contentFrame = [[m_pData->m_pWindow contentView] frame];

        CGFloat width  = contentFrame.size.width;
        CGFloat height = contentFrame.size.height;

		return { static_cast<int>( width ), static_cast<int>( height ) };
	}

	void RubyCocoaBackend::MoveWindow( int x, int y )
	{
	}

	void RubyCocoaBackend::SetTitle( const std::string& rTitle )
	{
	}

	void RubyCocoaBackend::SetTitle( const std::wstring& rTitle )
	{
	}

	void RubyCocoaBackend::SetMousePos( double x, double y )
	{
        CGWarpMouseCursorPosition( CGPointMake( x, y ) );
	}

	RubyVec2 RubyCocoaBackend::GetMousePos()
	{
        NSPoint pos = [NSEvent mouseLocation];
		return { static_cast<float>( pos.x ), static_cast<float>( pos.y ) };
	}

	VkResult RubyCocoaBackend::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
        VkMetalSurfaceCreateInfoEXT CreateInfo{ VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT };
        CreateInfo.pLayer = m_pData->m_pMetalLayer;

		return vkCreateMetalSurfaceEXT( Instance, &CreateInfo, nullptr, pOutSurface );
	}

	void RubyCocoaBackend::SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason /*= RubyMouseCursorSetReason::User */ )
	{
	}

	void RubyCocoaBackend::SetMouseCursorMode( RubyCursorMode mode )
	{
	}

	void RubyCocoaBackend::SetClipboardText( const std::string& rTextData )
	{
	}

	void RubyCocoaBackend::SetClipboardText( const std::wstring& rTextData )
	{
	}

	std::string RubyCocoaBackend::GetClipboardText()
	{
		return {};
	}

	std::wstring RubyCocoaBackend::GetClipboardTextW()
	{
		return {};
	}

	bool RubyCocoaBackend::PendingClose()
	{
		return m_WindowClosed;
	}

	void RubyCocoaBackend::Focus()
	{
        [NSApp activateIgnoringOtherApps:YES];
        [m_pData->m_pWindow makeKeyAndOrderFront:nil];
	}

	RubyIVec2 RubyCocoaBackend::GetWindowPos()
	{
        NSPoint pos = [m_pData->m_pWindow frame].origin;

		return { (int)pos.x, (int)pos.y };
	}

	bool RubyCocoaBackend::MouseInRect()
	{
        NSPoint mouse = [NSEvent mouseLocation];
        for (NSWindow *window in [NSApp orderedWindows]) 
        {
            if (![window isVisible])
                continue;

            if (NSPointInRect(mouse, window.frame)) 
            {
                return true;
            }
        }

		return false;
	}

    RubyMacOSData* RubyCocoaBackend::GetData() 
    {
        return m_pData;
    }

	void RubyCocoaBackend::FlashAttention()
	{
        [NSApp requestUserAttention:NSCriticalRequest];
	}

	void RubyCocoaBackend::SetIcon( Ref<class Texture2D> icon )
	{
	}

    std::vector<RubyMonitor> RubyCocoaBackend::GetMonitors() 
    {
        std::vector<RubyMonitor> monitors;

        NSArray< NSScreen* >* pScreens = [ NSScreen screens ];

        for( NSScreen* pScreen in pScreens )
        {
            NSRect frame = [pScreen frame];
            NSRect workArea = [pScreen visibleFrame];
            NSString* pName = [pScreen localizedName];

            auto& rMonitor = monitors.emplace_back();
            rMonitor.WorkSize = { static_cast<int>( workArea.size.width ), static_cast<int>( workArea.size.height ) };
            rMonitor.MonitorSize = { static_cast<int>( frame.size.width ), static_cast<int>( frame.size.height ) };
            rMonitor.MonitorPosition = { static_cast<int>( frame.origin.x ), static_cast<int>( frame.origin.y ) };
            rMonitor.Primary = true;
        }

        return monitors;
    }

}
