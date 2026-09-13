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
#include "MacOSAuxiliary.h"

#import <Foundation/Foundation.h>
#import <Appkit/AppKit.h>

namespace Saturn::Auxiliary {

    void MacOS::OpenFolderInExplorer( const std::filesystem::path& rPath, bool select ) 
    {
        @autoreleasepool 
        {
            const std::wstring& rPathW = rPath.wstring();

            NSString* nsStringPath =
                [[NSString alloc]
                    initWithBytes:rPathW.data()
                        length:rPathW.size() * sizeof(wchar_t)
                        encoding:NSUTF32LittleEndianStringEncoding];

            NSURL* nsPath = [NSURL fileURLWithPath:nsStringPath isDirectory:YES];

            if( select ) 
            {
                [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[nsPath]];
            }
            else
            {
                [[NSWorkspace sharedWorkspace] openURL:nsPath];
            }
        }
    }

    void MacOS::ShowErrorDialogBox( const std::string& rText, const std::string& rTitle ) 
    {
        @autoreleasepool 
        {
            NSString* nsText = [NSString stringWithUTF8String:rText.c_str()];
            NSString* nsTitle = [NSString stringWithUTF8String:rTitle.c_str()];

            NSAlert* pAlert = [[NSAlert alloc] init];
            [pAlert setMessageText:nsText];
            [pAlert setInformativeText:nsTitle];
            [pAlert setAlertStyle:NSAlertStyleCritical];
            [pAlert addButtonWithTitle:@"OK"];
            [pAlert runModal];
        }
    }

    const std::filesystem::path MacOS::GetAppDataPath() 
    {
        std::filesystem::path appDataPath;

        @autoreleasepool 
        {
            // TODO: Use NSBundle.
            NSArray* pPaths = NSSearchPathForDirectoriesInDomains(
                NSApplicationSupportDirectory,
                NSUserDomainMask,
                YES
            );

            NSString* appSupportPath = pPaths.firstObject;

            appDataPath = [appSupportPath fileSystemRepresentation];
        }

        return appDataPath;
    }

}
