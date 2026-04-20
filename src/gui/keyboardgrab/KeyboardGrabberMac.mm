#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CGEvent.h>
#include <wx/log.h>
#include <wx/osx/core/cfstring.h>
#include "KeyboardGrabber.h"


// taken from wxWidgets src/osx/cocoa/window.mm at 6026d2f609656fa44df0efbaeee98b15e54931bd
static long wxOSXTranslateCocoaKey( NSEvent* event, int eventType )
{
    long retval = 0;

    if ([event type] != NSFlagsChanged)
    {
        NSString* s = [event charactersIgnoringModifiersIncludingShift];
        // backspace char reports as delete w/modifiers for some reason
        if ([s length] == 1)
        {
            if ( eventType == wxEVT_CHAR && ([event modifierFlags] & NSControlKeyMask) && ( [s characterAtIndex:0] >= 'a' && [s characterAtIndex:0] <= 'z' ) )
            {
                retval = WXK_CONTROL_A + ([s characterAtIndex:0] - 'a');
            }
            else
            {
                switch ( [s characterAtIndex:0] )
                {
                    // numpad enter key End-of-text character ETX U+0003
                    case 3:
                        retval = WXK_NUMPAD_ENTER;
                        break;
                    // backspace key
                    case 0x7F :
                    case 8 :
                        retval = WXK_BACK;
                        break;
                    case NSUpArrowFunctionKey :
                        retval = WXK_UP;
                        break;
                    case NSDownArrowFunctionKey :
                        retval = WXK_DOWN;
                        break;
                    case NSLeftArrowFunctionKey :
                        retval = WXK_LEFT;
                        break;
                    case NSRightArrowFunctionKey :
                        retval = WXK_RIGHT;
                        break;
                    case NSInsertFunctionKey  :
                        retval = WXK_INSERT;
                        break;
                    case NSDeleteFunctionKey  :
                        retval = WXK_DELETE;
                        break;
                    case NSHomeFunctionKey  :
                        retval = WXK_HOME;
                        break;
                    case NSBeginFunctionKey  :
                        retval = WXK_NUMPAD_BEGIN;
                        break;
                    case NSEndFunctionKey  :
                        retval = WXK_END;
                        break;
                    case NSPageUpFunctionKey  :
                        retval = WXK_PAGEUP;
                        break;
                   case NSPageDownFunctionKey  :
                        retval = WXK_PAGEDOWN;
                        break;
                   case NSHelpFunctionKey  :
                        retval = WXK_HELP;
                        break;
                    default:
                        int intchar = [s characterAtIndex: 0];
                        if ( intchar >= NSF1FunctionKey && intchar <= NSF24FunctionKey )
                            retval = WXK_F1 + (intchar - NSF1FunctionKey );
                        else if ( intchar > 0 && intchar < 32 )
                            retval = intchar;
                        break;
                }
            }
        }
    }

    // Some keys don't seem to have constants. The code mimics the approach
    // taken by WebKit. See:
    // http://trac.webkit.org/browser/trunk/WebCore/platform/mac/KeyEventMac.mm
    switch( [event keyCode] )
    {
        // command key
        case 54:
        case 55:
            retval = WXK_CONTROL;
            break;
        // caps locks key
        case 57: // Capslock
            retval = WXK_CAPITAL;
            break;
        // shift key
        case 56: // Left Shift
        case 60: // Right Shift
            retval = WXK_SHIFT;
            break;
        // alt key
        case 58: // Left Alt
        case 61: // Right Alt
            retval = WXK_ALT;
            break;
        // ctrl key
        case 59: // Left Ctrl
        case 62: // Right Ctrl
            retval = WXK_RAW_CONTROL;
            break;
        // clear key
        case 71:
            retval = WXK_CLEAR;
            break;
        // tab key
        case 48:
            retval = WXK_TAB;
            break;
        default:
            break;
    }

    // Check for NUMPAD keys.  For KEY_UP/DOWN events we need to use the
    // WXK_NUMPAD constants, but for the CHAR event we want to use the
    // standard ascii values
    if ( eventType != wxEVT_CHAR )
    {
        switch( [event keyCode] )
        {
            case 75: // /
                retval = WXK_NUMPAD_DIVIDE;
                break;
            case 67: // *
                retval = WXK_NUMPAD_MULTIPLY;
                break;
            case 78: // -
                retval = WXK_NUMPAD_SUBTRACT;
                break;
            case 69: // +
                retval = WXK_NUMPAD_ADD;
                break;
            case 65: // .
                retval = WXK_NUMPAD_DECIMAL;
                break;
            case 82: // 0
                retval = WXK_NUMPAD0;
                break;
            case 83: // 1
                retval = WXK_NUMPAD1;
                break;
            case 84: // 2
                retval = WXK_NUMPAD2;
                break;
            case 85: // 3
                retval = WXK_NUMPAD3;
                break;
            case 86: // 4
                retval = WXK_NUMPAD4;
                break;
            case 87: // 5
                retval = WXK_NUMPAD5;
                break;
            case 88: // 6
                retval = WXK_NUMPAD6;
                break;
            case 89: // 7
                retval = WXK_NUMPAD7;
                break;
            case 91: // 8
                retval = WXK_NUMPAD8;
                break;
            case 92: // 9
                retval = WXK_NUMPAD9;
                break;
            default:
                //retval = [event keyCode];
                break;
        }
    }
    return retval;
}


// taken from wxWidgets src/osx/cocoa/window.mm at 6026d2f609656fa44df0efbaeee98b15e54931bd and added peer arg
static void SetupKeyEvent(wxKeyEvent &wxevent , NSEvent * nsEvent, NSString* charString, wxWindow* peer)
{
    UInt32 modifiers = [nsEvent modifierFlags] ;
    int eventType = [nsEvent type];

    wxevent.m_shiftDown = modifiers & NSShiftKeyMask;
    wxevent.m_rawControlDown = modifiers & NSControlKeyMask;
    wxevent.m_altDown = modifiers & NSAlternateKeyMask;
    wxevent.m_controlDown = modifiers & NSCommandKeyMask;

    wxevent.m_rawCode = [nsEvent keyCode];
    wxevent.m_rawFlags = modifiers;

    wxevent.SetTimestamp( static_cast<long>([nsEvent timestamp] * 1000) ) ;
    wxevent.m_isRepeat = (eventType == NSKeyDown) && [nsEvent isARepeat];

    wxString chars;
    if ( eventType != NSFlagsChanged )
    {
        NSString* nschars = [[nsEvent charactersIgnoringModifiersIncludingShift] uppercaseString];
        if ( charString )
        {
            // if charString is set, it did not come from key up / key down
            wxevent.SetEventType( wxEVT_CHAR );
            chars = wxCFStringRef::AsString(charString);
        }
        else if ( nschars )
        {
            chars = wxCFStringRef::AsString(nschars);
        }
    }

    int aunichar = chars.Length() > 0 ? chars[0] : 0;
    long keyval = 0;

    if (wxevent.GetEventType() != wxEVT_CHAR)
    {
        keyval = wxOSXTranslateCocoaKey(nsEvent, wxevent.GetEventType()) ;
        switch (eventType)
        {
            case NSKeyDown :
                wxevent.SetEventType( wxEVT_KEY_DOWN )  ;
                break;
            case NSKeyUp :
                wxevent.SetEventType( wxEVT_KEY_UP )  ;
                break;
            case NSFlagsChanged :
                switch (keyval)
                {
                    case WXK_CONTROL:
                        wxevent.SetEventType( wxevent.m_controlDown ? wxEVT_KEY_DOWN : wxEVT_KEY_UP);
                        break;
                    case WXK_SHIFT:
                        wxevent.SetEventType( wxevent.m_shiftDown ? wxEVT_KEY_DOWN : wxEVT_KEY_UP);
                        break;
                    case WXK_ALT:
                        wxevent.SetEventType( wxevent.m_altDown ? wxEVT_KEY_DOWN : wxEVT_KEY_UP);
                        break;
                    case WXK_RAW_CONTROL:
                        wxevent.SetEventType( wxevent.m_rawControlDown ? wxEVT_KEY_DOWN : wxEVT_KEY_UP);
                        break;
                }
                break;
            default :
                break ;
        }
    }
    else
    {
        long keycode = wxOSXTranslateCocoaKey( nsEvent, wxEVT_CHAR );
        if ( (keycode > 0 && keycode < WXK_SPACE) || keycode == WXK_DELETE || keycode >= WXK_START )
        {
            keyval = keycode;
        }
    }

    if ( !keyval && aunichar < 256 ) // only for ASCII characters
    {
        if ( wxevent.GetEventType() == wxEVT_KEY_UP || wxevent.GetEventType() == wxEVT_KEY_DOWN )
            keyval = wxToupper( aunichar ) ;
        else
            keyval = aunichar;
    }

    // OS X generates events with key codes in Unicode private use area for
    // unprintable symbols such as cursor arrows (WXK_UP is mapped to U+F700)
    // and function keys (WXK_F2 is U+F705). We don't want to use them as the
    // result of wxKeyEvent::GetUnicodeKey() however as it's supposed to return
    // WXK_NONE for "non characters" so explicitly exclude them.
    //
    // We only exclude the private use area inside the Basic Multilingual Plane
    // as key codes beyond it don't seem to be currently used.
    if ( !(aunichar >= 0xe000 && aunichar < 0xf900) )
        wxevent.m_uniChar = aunichar;

    wxevent.m_keyCode = keyval;

    if ( peer )
    {
        wxevent.SetEventObject(peer);
        wxevent.SetId(peer->GetId()) ;
    }
}


// CGEventTap callback function
static CGEventRef keyboardEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon)
{
    KeyboardGrabber* grabber = static_cast<KeyboardGrabber*>(refcon);
    wxWindow* window = grabber->getDestinationWindow();
    if (!window) {
        wxLogDebug("KeyboardGrabber::keyboardEventCallback: no destination window");
        return event;
    }

    NSEvent* nsevent = [NSEvent eventWithCGEvent:event];
    wxLogDebug("KeyboardGrabber::keyboardEventCallback: nsEvent=%p", nsevent);

    wxKeyEvent wxevent(wxEVT_KEY_DOWN);
    SetupKeyEvent(wxevent, nsevent, nullptr, window);

    wxLogDebug("KeyboardGrabber::keyboardEventCallback: wxEvent eventType=%d keyCode=%ld rawCode=%d",
               wxevent.GetEventType(), wxevent.m_keyCode, wxevent.m_rawCode);

    bool handled = window->HandleWindowEvent(wxevent);

    // try CHAR event for unhandled down key
    if (!handled && wxevent.GetEventType() == wxEVT_KEY_DOWN)
    {
        NSString* chars = [nsevent characters];
        if ([chars length] > 0)
        {
            wxKeyEvent charEvent(wxEVT_CHAR);
            SetupKeyEvent(charEvent, nsevent, chars, window);
            wxLogDebug("KeyboardGrabber::keyboardEventCallback: sending CHAR event for '%s'", wxCFStringRef::AsString(chars).c_str());
            window->HandleWindowEvent(charEvent);
        }
    }

    // Return nullptr to prevent the event from being processed further
    return nullptr;
}


bool KeyboardGrabber::hasPermission()
{
    return AXIsProcessTrusted();
}


void KeyboardGrabber::showPermissionDialog()
{
    NSDictionary* options = @{(__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES};
    AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
}


void KeyboardGrabber::grab(wxWindow* window)
{
  if(m_grab) {
    wxLogDebug("KeyboardGrabber::grab: already grabbed, doing nothing");
    return;
  }

  if (!hasPermission()) {
    wxLogDebug("KeyboardGrabber::grab: no permission, doing nothing");
    return;
  }

  // Create event tap for keyboard events
  CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | 
                        CGEventMaskBit(kCGEventKeyUp) | 
                        CGEventMaskBit(kCGEventFlagsChanged);

  m_grab = (void*)CGEventTapCreate(kCGSessionEventTap, 
                               kCGHeadInsertEventTap, 
                               kCGEventTapOptionDefault,
                               eventMask,
                               keyboardEventCallback,
                               this);

  if (!m_grab) {
      wxLogDebug("KeyboardGrabber::grab: failed to create event tap for grab");
      return;
  }

  // Enable the event tap
  CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, (CFMachPortRef)m_grab, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
  CFRelease(runLoopSource);

  m_destinationWindow = window;

  wxLogDebug("KeyboardGrabber::grab: success");
}


void KeyboardGrabber::ungrab()
{
  if (m_grab) {
      CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, (CFMachPortRef)m_grab, 0);
      CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
      CFRelease(runLoopSource);
      CFRelease((CFMachPortRef)m_grab);
      m_grab = nullptr;
      m_destinationWindow = nullptr;
      wxLogDebug("KeyboardGrabber::ungrab: released grab");
  } else {
      wxLogDebug("KeyboardGrabber::ungrab: there was no grab");
  }
}
