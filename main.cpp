#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

const int CF_RUN_LOOP_INTERVAL = 300;

CGEventRef LoggingKeystrokesCallback(
    CGEventTapProxy CGEventTapProxy,
    CGEventType CGEventType,
    CGEventRef CGEvent,
    void *refcon)
{
    if (CGEventType == kCGEventKeyDown || CGEventType == kCGEventFlagsChanged)
    {
        int64_t keycode = CGEventGetIntegerValueField(CGEvent, kCGKeyboardEventKeycode);
        if (keycode == 12)
        {
            std::cout << "Q pressed. Shutting down..." << std::endl;
            CFRunLoopStop(CFRunLoopGetCurrent());
            return CGEvent;
        }
        std::cout << "Keycode pressed is : " << keycode << std::endl;
    }

    return CGEvent;
}

int main()
{
    CGEventMask keydownMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);

    CFMachPortRef eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGTailAppendEventTap,
        kCGEventTapOptionListenOnly,
        keydownMask,
        LoggingKeystrokesCallback,
        NULL);

    if (!eventTap)
    {
        std::cerr << "Error with event tap event, check permissions or code" << std::endl;
        exit(1);
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, CF_RUN_LOOP_INTERVAL, false);

    return 0;
}