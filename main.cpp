#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <string.h>
#include <sstream>
#include <ApplicationServices/ApplicationServices.h>

const uint16 CF_RUN_LOOP_INTERVAL = 300;
const char *quit_string = "QUIT";
const float SENSITIVITY = 0.75;
const int32_t SCROLL_UNIT = 5;
const uint32_t WHEELCOUNT = 1;

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
    // CGEventMask keydownMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);

    // CFMachPortRef eventTap = CGEventTapCreate(
    //     kCGSessionEventTap,
    //     kCGTailAppendEventTap,
    //     kCGEventTapOptionListenOnly,
    //     keydownMask,
    //     LoggingKeystrokesCallback,
    //     NULL);

    // if (!eventTap)
    // {
    //     std::cerr << "Error with event tap event, check permissions or code" << std::endl;
    //     exit(1);
    // }

    // CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

    // CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    // CGEventTapEnable(eventTap, true);

    // CFRunLoopRunInMode(kCFRunLoopDefaultMode, CF_RUN_LOOP_INTERVAL, false);

    // code base template got from geeks for geeks
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    bool exitLoop = false;

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    bind(serverSocket, (struct sockaddr *)&serverAddress,
         sizeof(serverAddress));

    // listening to the assigned socket
    listen(serverSocket, 5);

    // accepting connection request
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    std::cout << "DEBUG: Phone has connected! Waiting for data..." << clientSocket << std::endl;
    if (clientSocket == -1)
    {
        std::cout << "Exiting program, no phones founs." << std::endl;
    };

    CGPoint currentMousePosition = {.x = 0, .y = 0};

    CGMouseButton MouseButtonPress = kCGMouseButtonLeft;

    CGEventType MouseEventType = kCGEventMouseMoved;
    CGEventType MouseEventTypeUp = kCGEventMouseMoved;
    bool click = false;
    bool isDragDown = false;
    bool isScrolling = false;

    if (!AXIsProcessTrusted())
    {
        std::cout << "WARNING: No Accessibility Permissions!" << std::endl;
        std::cout << "Go to System Settings > Privacy & Security > Accessibility" << std::endl;
        std::cout << "And ensure your Terminal/IDE is enabled." << std::endl;
    }
    else
    {
        std::cout << "SUCCESS: Accessibility Permissions granted." << std::endl;
    }

    char buffer[32] = {0};
    while (!exitLoop)
    {
        memset(buffer, 0, sizeof(buffer));
        // recieving data
        recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "Message from client: " << buffer
                  << std::endl;

        if (std::strstr(buffer, quit_string) != nullptr)
        {
            exitLoop = true;
            std::cout << "Exiting mouse logging  " << buffer
                      << std::endl;
        }

        // getting mouse coord every update or frame
        CGEventRef dummyEvent = CGEventCreate(NULL);
        currentMousePosition = CGEventGetLocation(dummyEvent);
        CFRelease(dummyEvent);

        CGPoint NewMousePosition = {
            currentMousePosition.x, currentMousePosition.y};

        if (std::strstr(buffer, "LEFT") != nullptr)
        {
            click = true;
            MouseButtonPress = kCGMouseButtonLeft;
            MouseEventType = kCGEventLeftMouseDown;
            MouseEventTypeUp = kCGEventLeftMouseUp;
        }
        else if (std::strstr(buffer, "RIGHT") != nullptr)
        {
            click = true;
            MouseButtonPress = kCGMouseButtonRight;
            MouseEventType = kCGEventRightMouseDown;
            MouseEventTypeUp = kCGEventRightMouseUp;
        }
        else if (std::strstr(buffer, "DRAG_UP") != nullptr)
        {
            if (isDragDown)
            {
                isDragDown = false;
                click = false;
                MouseButtonPress = kCGMouseButtonLeft;
                MouseEventTypeUp = kCGEventLeftMouseUp;
                MouseEventType = kCGEventMouseMoved;

                CGEventRef MouseEventReleaseDrag = CGEventCreateMouseEvent(
                    NULL,
                    MouseEventTypeUp,
                    NewMousePosition,
                    MouseButtonPress);
                CGEventPost(kCGHIDEventTap, MouseEventReleaseDrag);
                CFRelease(MouseEventReleaseDrag);
            }
        }
        // simulating dragging where i need to let go
        else if (std::strstr(buffer, "DRAG_DOWN") != nullptr)
        {
            isDragDown = true;
            click = false;
            MouseButtonPress = kCGMouseButtonLeft;
            MouseEventType = kCGEventLeftMouseDown;
        }
        else if (std::strstr(buffer, "SCROLL_UP") != nullptr)
        {
            isDragDown = false;
            click = false;
            isScrolling = true;
            MouseButtonPress = kCGMouseButtonLeft;
            MouseEventType = kCGEventMouseMoved;

            CGEventRef MouseScrollUp = CGEventCreateScrollWheelEvent(
                NULL,
                kCGScrollEventUnitLine,
                WHEELCOUNT, // wheelcount, number of scrolling devices, just one
                SCROLL_UNIT);

            CGEventPost(kCGHIDEventTap, MouseScrollUp);
            CFRelease(MouseScrollUp);
        }
        else if (std::strstr(buffer, "SCROLL_DOWN") != nullptr)
        {
            isDragDown = false;
            click = false;
            isScrolling = true;
            MouseButtonPress = kCGMouseButtonLeft;
            MouseEventType = kCGEventMouseMoved;

            CGEventRef MouseScrollDown = CGEventCreateScrollWheelEvent(
                NULL,
                kCGScrollEventUnitLine,
                WHEELCOUNT, // wheelcount, number of scrolling devices, just one
                -1 * SCROLL_UNIT);

            CGEventPost(kCGHIDEventTap, MouseScrollDown);
            CFRelease(MouseScrollDown);
        }
        // else is for numbers i guess, for moving, has state for regular moving or dragging and moving
        else
        {
            click = false;
            std::istringstream iss(buffer);

            std::string line;
            std::string moveCommand;
            double dx = 0, dy = 0;
            if (isDragDown)
            {
                MouseEventType = kCGEventLeftMouseDragged;
            }
            else
            {
                MouseEventType = kCGEventMouseMoved;
            }

            while (iss >> moveCommand >> dx >> dy)
            {
                std::cout << "COMMAND: " << moveCommand << ", DX: " << dx << ", DY: " << dy << std::endl;
            }

            NewMousePosition = {
                currentMousePosition.x + dx * SENSITIVITY,
                currentMousePosition.y + dy * SENSITIVITY,
            };
        }

        CGEventRef MouseEventCreated = CGEventCreateMouseEvent(
            NULL,
            MouseEventType,
            NewMousePosition,
            MouseButtonPress);

        if (!MouseEventCreated)
        {
            std::cerr << "Error with mouse event tap event, check permissions or code" << std::endl;
            exit(1);
        }

        std::cout << "Attempting to post event..." << std::endl;
        CGEventPost(kCGHIDEventTap, MouseEventCreated);
        std::cout << "Post successful!" << std::endl;

        // if click then need to do a mirror event which will lift mouse button up at the end
        // and free everything
        if (click)
        {
            CGEventRef MouseEventCreatedLiftUp = CGEventCreateMouseEvent(
                NULL,
                MouseEventTypeUp,
                NewMousePosition,
                MouseButtonPress);
            CGEventPost(kCGHIDEventTap, MouseEventCreatedLiftUp);
            CFRelease(MouseEventCreatedLiftUp);
        }

        CFRelease(MouseEventCreated);
    }

    // closing the socket.
    close(serverSocket);

    return 0;
}