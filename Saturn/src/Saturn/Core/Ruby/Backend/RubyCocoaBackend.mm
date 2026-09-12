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
    NSTrackingArea* m_pTrackingArea;
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
        m_pTrackingArea = nil;

        [self updateTrackingAreas];
        [self registerForDraggedTypes:@[NSPasteboardTypeURL]];
    }

    return self;
}

- (void)dealloc
{
    [m_pTrackingArea release];
    [super dealloc];
}

-(BOOL)isOpaque
{
    return [pThis->GetData()->m_pWindow isOpaque];
}

-(BOOL)canBecomeKeyView
{
    return YES;
}

-(BOOL)wantsUpdateLayer
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

-(void)updateTrackingAreas
{
    if (m_pTrackingArea != nil)
    {
        [self removeTrackingArea:m_pTrackingArea];
        [m_pTrackingArea release];
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | 
                                          NSTrackingMouseMoved |
                                        NSTrackingActiveInKeyWindow;

    m_pTrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                   options:options
                                                     owner:self
                                                  userInfo:nil];

    [self addTrackingArea:m_pTrackingArea];
    [super updateTrackingAreas];
}

- (void)mouseMoved:(NSEvent*)event
{
    const NSPoint position = [event locationInWindow];
    const NSRect contentRect = [pThis->GetData()->m_pView frame];

    pThis->GetParent()->DispatchEvent<Saturn::RubyMouseMoveEvent>( Saturn::EventType::MouseMoved, ( float ) position.x,  contentRect.size.height - ( float ) position.y );
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

- (void)scrollWheel:(NSEvent*)event
{
    float xOffset = [event scrollingDeltaX];
    float yOffset = [event scrollingDeltaY];

    if( [event hasPreciseScrollingDeltas] )
    {
        xOffset *= 0.1f;
        yOffset *= 0.1f;
    }

    if( fabs( xOffset ) > 0.0f || fabs( yOffset ) > 0.0f )
    {
        pThis->GetParent()->DispatchEvent<Saturn::RubyMouseScrollEvent>( Saturn::EventType::MouseScroll, xOffset, yOffset );
    }
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

	const std::string hex = std::format( "{:08X}", vk );
	SAT_CORE_WARN( "[Ruby] Unknown Key!, Win32 scan code: WSC/0x{}", hex );

    switch( vk )
    {
        default: return RubyKey_UnknownKey;

        case 0x0: return RubyKey_A;
        case 0xB: return RubyKey_B;
        case 0x8: return RubyKey_C;
        case 0x2: return RubyKey_D;
        case 0xE: return RubyKey_E;
        case 0x3: return RubyKey_F;
        case 0x5: return RubyKey_G;
        case 0x4: return RubyKey_H;
        case 0x22: return RubyKey_I;
        case 0x26: return RubyKey_J;
        case 0x28: return RubyKey_K;
        case 0x25: return RubyKey_L;
        case 0x2E: return RubyKey_M;
        case 0x2D: return RubyKey_N;
        case 0x1F: return RubyKey_O;
        case 0x23: return RubyKey_P;
        case 0x0C: return RubyKey_Q;
        case 0x0F: return RubyKey_R;
        case 0x01: return RubyKey_S;
        case 0x11: return RubyKey_T;
        case 0x20: return RubyKey_U;
        case 0x09: return RubyKey_V;
        case 0x0D: return RubyKey_W;
        case 0x07: return RubyKey_X;
        case 0x10: return RubyKey_Y;
        case 0x06: return RubyKey_Z;

        case 0x12: return RubyKey_Num1;
        case 0x13: return RubyKey_Num2;
        case 0x14: return RubyKey_Num3;
        case 0x15: return RubyKey_Num4;
        case 0x17: return RubyKey_Num5;
        case 0x16: return RubyKey_Num6;
        case 0x1A: return RubyKey_Num7;
        case 0x1C: return RubyKey_Num8;
        case 0x19: return RubyKey_Num9;
        case 0x1D: return RubyKey_Num0;

        case 0x33: return RubyKey_Backspace;
        case 0x30: return RubyKey_Tab;
        case 0x24: return RubyKey_Enter;
        case 0x31: return RubyKey_Space;

        case 0x35: return RubyKey_Esc;

        case 0x7B: return RubyKey_LeftArrow;
        case 0x7C: return RubyKey_RightArrow;
        case 0x7E: return RubyKey_UpArrow;
        case 0x7D: return RubyKey_DownArrow;

        case 0x72: return RubyKey_F1;
        case 0x78: return RubyKey_F2;
        case 0x63: return RubyKey_F3;
        case 0x76: return RubyKey_F4;
        case 0x60: return RubyKey_F5;
        case 0x61: return RubyKey_F6;
        case 0x62: return RubyKey_F7;
        case 0x64: return RubyKey_F8;
        case 0x65: return RubyKey_F9;
        case 0x6D: return RubyKey_F10;
        case 0x67: return RubyKey_F11;
        case 0x6F: return RubyKey_F12;

        case 0x73: return RubyKey_CapsLock;
        case 0x39: return RubyKey_LeftShift;
        case 0x3C: return RubyKey_RightShift;
        case 0x3B: return RubyKey_LeftCtrl;
        case 0x3E: return RubyKey_RightCtrl;
        case 0x3A: return RubyKey_LeftAlt;
        case 0x3D: return RubyKey_RightAlt;

        case 0x29: return RubyKey_Semicolon;
        case 0x27: return RubyKey_Apostrophe;
        case 0x2A: return RubyKey_Backslash;
        case 0x1B: return RubyKey_Minus;
        case 0x18: return RubyKey_Equal;
        case 0x1E: return RubyKey_RightBracket;
        case 0x21: return RubyKey_LeftBracket;
        case 0x2B: return RubyKey_Comma;
        case 0x2C: return RubyKey_Slash;
        case 0x2F: return RubyKey_Period;
        case 0x32: return RubyKey_Backslash;
        case 0x0A: return RubyKey_Grave;

        case 0x47: return RubyKey_NumLock;
   }
}

- (void) keyDown:(NSEvent*) event 
{
    const Saturn::RubyKey saturnKey = ConvertMacOSVkToRuby( [event keyCode] );
    const int Modifiers = TranslateMacOSModifiers( [event modifierFlags] );

	pThis->GetParent()->IntrnlSetKeyDown( saturnKey, true );
	pThis->GetParent()->DispatchEvent<Saturn::RubyKeyEvent>( Saturn::EventType::KeyPressed, saturnKey, [event keyCode], Modifiers );

    [self interpretKeyEvents:@[event]];
}

- (void) keyUp:(NSEvent*) event 
{
    const Saturn::RubyKey saturnKey = ConvertMacOSVkToRuby( [event keyCode] );
    const int Modifiers = TranslateMacOSModifiers( [event modifierFlags] );

	pThis->GetParent()->IntrnlSetKeyDown( saturnKey, false );
	pThis->GetParent()->DispatchEvent<Saturn::RubyKeyEvent>( Saturn::EventType::KeyReleased, saturnKey, [event keyCode], Modifiers );
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    NSString* pCharacters;
    if( [aString isKindOfClass:[NSAttributedString class]] )
        pCharacters = [aString string];
    else
        pCharacters = (NSString*)aString;

    for( NSUInteger i = 0; i < [pCharacters length]; ++i )
    {
        uint16_t wc = [pCharacters characterAtIndex:i];
        pThis->GetParent()->DispatchEvent<Saturn::RubyCharacterEvent>( Saturn::EventType::InputCharacter, wc );
    }
}

- (BOOL)hasMarkedText
{
    return NO;
}

- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
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

// Stolen from GLFW.
static float RubyTransformYCocoa( float y )
{
    return CGDisplayBounds(CGMainDisplayID()).size.height - y - 1;
}

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
        m_pData->m_pView = [[RubyEventResponder alloc] initForRuby:this];

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
        switch( Command ) 
        {
            case RubyWindowShowCmd::Default:
            {
                [m_pData->m_pWindow makeKeyAndOrderFront:nil];
            } break;

            case RubyWindowShowCmd::NoActivate:
            {
                [m_pData->m_pWindow orderFront:nil];
            } break;

            case RubyWindowShowCmd::Fullscreen:
            {
                [m_pData->m_pWindow toggleFullScreen:nil];
            } break;
        }
	}

	void RubyCocoaBackend::HideWindow()
	{
        [m_pData->m_pWindow orderOut:nil];
	}

	void RubyCocoaBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
        NSRect frame = [m_pData->m_pWindow frame];
        frame.size.width = Width;
        frame.size.height = Height;

        [m_pData->m_pWindow setFrame:frame display:YES animate:NO];
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
        NSRect frame = [m_pData->m_pWindow frame];
        frame.origin.x = x;

        // Flip the y coordinate because macOS uses a 
        // different coordinate system than most other
        // platforms.
        CGFloat screenHeight = [[NSScreen mainScreen] frame].size.height;
        y = screenHeight - y - frame.size.height;
        frame.origin.y = y;

        [m_pData->m_pWindow setFrame:frame display:YES animate:NO];
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
        NSPoint pos = [m_pData->m_pWindow mouseLocationOutsideOfEventStream];
        NSRect contentRect = [m_pData->m_pView frame];

        // NB: macOS uses a different coordinate system 
        // than most other platforms, so we need to 
        // flip the y coordinate.
//        CGFloat screenHeight = [[NSScreen mainScreen] frame].size.height;
//        pos.y = screenHeight - pos.y;

		return { static_cast<float>( pos.x ), static_cast<float>( contentRect.size.height - pos.y ) };
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
        const NSRect contectRect = [m_pData->m_pWindow contentRectForFrameRect:[m_pData->m_pWindow frame]];
        
        return { static_cast<int>( contectRect.origin.x ), static_cast<int>( RubyTransformYCocoa( contectRect.origin.y + contectRect.size.height - 1 ) ) };
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
