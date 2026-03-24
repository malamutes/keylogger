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

const int CF_RUN_LOOP_INTERVAL = 300;
const char *quit_string = "QUIT";

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

    char buffer[1024] = {0};
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
            MouseButtonPress = kCGMouseButtonLeft;
        }
        else if (std::strstr(buffer, "RIGHT") != nullptr)
        {
            MouseButtonPress = kCGMouseButtonRight;
        }
        // else is for numbers i guess
        else
        {
            std::istringstream iss(buffer);

            std::string line;
            std::string moveCommand;
            double dx = 0, dy = 0;
            while (iss >> moveCommand >> dx >> dy)
            {
                std::cout << "COMMAND: " << moveCommand << ", DX: " << dx << ", DY: " << dy << std::endl;
            }

            NewMousePosition = {
                currentMousePosition.x + dx,
                currentMousePosition.y + dy,
            };
        }

        CGEventRef MouseEventCreated = CGEventCreateMouseEvent(
            NULL,
            kCGEventMouseMoved,
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
        CFRelease(MouseEventCreated);
    }

    // closing the socket.
    close(serverSocket);

    return 0;
}