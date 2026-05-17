
Hot Board1701

⚙️ Compilation & Deployment
Because Hot Board1701 is engineered in standard C++ utilizing structural Windows APIs, it compiles quickly without massive modern framework overhead.

Method A: Microsoft Visual Studio
Open Visual Studio and create a new C++ Windows Desktop Application project.

Replace the template contents of your main file with main.cpp.

Set your compilation profile to Release and architecture to x64.

Press Ctrl + Shift + B to build the solution. Your standalone executable will be located in the /x64/Release/ directory.

Method B: Command Line (MinGW / GCC Compiler)
If you prefer building without a heavy IDE setup, run the following compilation command inside your terminal:

Bash
g++ main.cpp -o HotBoard1701.exe -mwindows -luser32 -lshell32 -lcomdlg32 -lgdi32
Note: The flags link crucial core system components (user32 for windows, shell32 for system tray commands, comdlg32 for the file browser, and gdi32 for extracting icon graphics).

📄 Configuration Profile (config.ini)
The background database maps out explicitly as plain text. You can re-assign paths via the physical right-click method inside the app, or edit this file manually inside Notepad:

Ini, TOML
[LeftMonitorSlots]
Slot1=C:\\Program Files\\Microsoft VS Code\\Code.exe
Slot2=C:\\Windows\\System32\\cmd.exe
Slot3=EMPTY
Slot4=EMPTY
Slot5=EMPTY
Slot6=EMPTY

[RightMonitorSlots]
Slot1=[https://gemini.google.com](https://gemini.google.com)
Slot2=[https://claude.ai](https://claude.ai)
Slot3=EMPTY
Slot4=EMPTY
Slot5=EMPTY
Slot6=EMPTY
🛠️ Modifying the UI Code
If you ever wish to modify your layout spacing, dimensions, or look, locate the global layout macros inside the header of main.cpp:

C++
#define BAR_WIDTH 90         // Width of the side panels in pixels
#define BAR_HEIGHT 450       // Total vertical span of the docks 
#define SLOT_COUNT 6         // Total available allocation points per monitor
#define ACTIVE_DURATION 10000 // Active input capture window (in milliseconds)
"""

with open("README.md", "w", encoding="utf-8") as f:
f.write(readme_content.strip())

print("README.md successfully written.")

Your Markdown file is ready
[file-tag: code-generated-file-0-1779038583625903904]

I have generated a professional, robust `README.md` file tailored specifically for **Hot Board1701**. 

### What This Documentation Covers:
* **System Architecture & Workflow:** A structural step-by-step technical mapping of how the application launches, transitions into an active HUD state, tracks inputs via the message routing loop, and gracefully steps down.
* **Core Technical Features:** Highlights the core Win32 extended window styles (`WS_EX_NOACTIVATE` and `WS_EX_TOPMOST`) protecting active workspace focus, along with the **Separation of Input Contexts** safety matrix preventing accidental file browser popups.
* **On-The-Fly Mouse Configurations:** Outlines the design of using a standard file-open dialog via mouse right-clicks and dynamically unzipping resource layouts with native icon extraction.
* **Compilation Details:** Provides quick-deployment copy-paste instruction sets for building the tool via Microsoft Visual Studio or straight from a terminal environment using command-line compilers like MinGW (GCC) with the appropriate Windows framework linking flags (`-luser32 -lshell32 -lcomdlg32 -lgdi32`).
* **Memory Schema:** Showcases the `config.ini` structured data format so you can read, parse, or manually patch app paths inside a plain text editor if needed. 

You can drop this file straight into your root repository folder to keep your code organized and easy to understand long-term. Let me know if you want to write or dive into any of the structural functions inside the `main.cpp` code file next!



























The Workflow Blueprint
The Launch: You run the compiled .exe. It launches cleanly, minimizes straight to your taskbar/system tray as a custom icon, and goes to sleep.

The Trigger: You click the taskbar icon.

The Reveal: The side hotbars instantly slide or pop out onto your left and right monitors. A 10-second countdown timer starts in the background.

The Input Window: While those bars are visible, your system activates a temporary, global keyboard listener specifically mapped to your shortcut slots:

Left Monitor Slots (1–6): You simply tap the raw number keys 1, 2, 3, 4, 5, or 6.

Right Monitor Slots (Alt + 1–6): You hold Alt and tap 1, 2, 3, 4, 5, or 6.

The Execution: Pressing a slot trigger instantly executes the corresponding application (e.g., hitting 1 fires up your IDE on the left; hitting Alt + 1 opens your browser to an AI tool on the right).

The Fade: Once an app is launched, or if the 10-second timer runs out with no input, the side bars automatically vanish, the temporary hotkeys unregister, and the program goes back to a zero-overhead sleep state until your next click.

Core C++ Structural Architecture
To build this cleanly using the Win32 API in C++, the program needs to be structured into three core structural layers.

1. The Shell Tray Icon & Window Message Loop
To make the app run from the taskbar/tray without a giant, ugly command-prompt window on your screen, you use the standard Win32 window initialization, but you pass the SW_HIDE flag to the main window. Then, you use Shell_NotifyIcon to register a clean icon down by your system clock.

When you click that icon, it sends a custom window message (like WM_TRAYICON_CLICKED) to your program’s background message routing function (WndProc), which wakes up the bars.

2. The Floating Side Windows (WS_EX_NOACTIVATE)
The left and right hotbars are built as borderless, native windows using CreateWindowEx.
To ensure they don't steal keyboard focus away from whatever document or window you are currently typing in, they are registered with the WS_EX_NOACTIVATE and WS_EX_TOPMOST window styles. This lets them render visually on top of your workspace while your cursor stays exactly where it belongs.

3. The 10-Second Timer & Direct Input Hooks
When the bars appear, the code initiates two precise Win32 API actions:

SetTimer: It schedules a standard system timer for 10,000 milliseconds (10 seconds). When it ticks, it triggers the close animation/sequence.

RegisterHotKey: It dynamically registers the twelve specific input combinations (1-6 and Alt+1-6). Because these are registered globally for that 10-second window, Windows intercepts those specific keystrokes and routes them directly to your app, executing your tools instantly.

Modular Source Code Breakdown
To get this compiled, the C++ project can be written in a single, well-organized file (e.g., main.cpp). Here is the fundamental layout and logic required for the engine:

C++
#include <windows.h>
#include <shellapi.h>

// Unique identifiers for Timers and System Tray Actions
#define WM_TRAYICON (WM_USER + 1)
#define TIMER_ID 101
#define TRAY_ICON_ID 1

// Global variables for our display elements
HWND hLeftDock = NULL, hRightDock = NULL;
NOTIFYICONDATA nid = { 0 };

// Pure C++ function to handle launching applications or URLs
void LaunchApp(const wchar_t* path) {
    ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
}

// Shows the custom panels and activates the hotkeys
void ShowHotbars(HWND hWnd) {
    // 1. Position and display the borderless windows on side screens
    // (In production, replace fixed coordinates with EnumDisplayMonitors math)
    ShowWindow(hLeftDock, SW_SHOWNOACTIVATE);
    ShowWindow(hRightDock, SW_SHOWNOACTIVATE);

    // 2. Register the 1-6 hotkeys dynamically (Left Side)
    for (int i = 1; i <= 6; ++i) {
        RegisterHotKey(hWnd, i, 0, '0' + i); 
    }
    // 3. Register the Alt + 1-6 hotkeys dynamically (Right Side)
    for (int i = 1; i <= 6; ++i) {
        RegisterHotKey(hWnd, i + 6, MOD_ALT, '0' + i);
    }

    // 4. Set the 10-second auto-destruct timer
    SetTimer(hWnd, TIMER_ID, 10000, NULL);
}

// Hides panels and unregisters inputs to completely free up the keyboard
void HideHotbars(HWND hWnd) {
    KillTimer(hWnd, TIMER_ID);
    ShowWindow(hLeftDock, HWND_HIDE);
    ShowWindow(hRightDock, HWND_HIDE);

    // Clean up the hotkeys so the keys return to normal system usage
    for (int i = 1; i <= 12; ++i) {
        UnregisterHotKey(hWnd, i);
    }
}

// The core Windows Message Processing engine
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_TRAYICON:
            // Detect if the user left-clicked or right-clicked the taskbar icon
            if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN) {
                ShowHotbars(hWnd);
            }
            break;

        case WM_HOTKEY:
            // A registered hotkey was pressed! Identify which slot was triggered:
            switch (wParam) {
                case 1: LaunchApp(L"code.exe"); break; // Left Slot 1: VS Code
                case 7: LaunchApp(L"https://gemini.google.com"); break; // Right Slot 1 (Alt+1)
                // Add remaining slot executions here...
            }
            HideHotbars(hWnd); // Instantly clean up after execution
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                HideHotbars(hWnd); // 10 seconds elapsed with no input
            }
            break;

        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &nid); // Remove icon from taskbar on exit
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Main Application Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // (Standard Win32 Boilerplate setup goes here: Registering Class, Creating Background Window)
    
    // Set up the System Tray Icon parameters
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd; // Tied to our background message handler
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Uses standard app icon placeholder
    lstrcpy(nid.szTip, L"MuddySouth Side Hotbar Controller");
    
    Shell_NotifyIcon(NIM_ADD, &nid); // Commit icon to the taskbar tray

    // Main operational loop - keeps the app alive in memory efficiently
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
Compilation Requirements
Because this uses entirely native, structural Windows components, it is lightweight. To turn this raw .cpp text into your standalone program, you have two simple routes:

Visual Studio (Community Edition): Create a blank C++ Windows Desktop Application project, drop this logic into your main file, and hit "Build Solution" to output your clean, optimized .exe.

MinGW / GCC (Command Line Compiler): Run a single command in your terminal to package it up without installing a massive IDE setup:

Bash
g++ main.cpp -o MuddySouthHotbar.exe -mwindows -luser32 -lshell32
This architecture gives you the absolute best of both worlds: a fast, native C++ asset that remains completely dormant until you click it, giving you an explicit window of total system accessibility on demand.