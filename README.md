# E_ACCESSDENIED When Debugging Low IL Processes

## Abstract

This sample project showcases an unexpected error when debugging a [`Low Integrity Level`](https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control) process.

The Visual Studio solution consists of:
- SandboxTest\
  Simple C++/WinRT-based console application making a trivial HTTP call.
- SandboxLauncher\
  Console application that launches SandboxTest with Low Integrity Level.

## Steps to Reproduce

1. Clone this repository.
    ```
    git clone https://github.com/jurocha-ms/LowILAccessDenied
    ```
1. Enter the clone's directory.
1. Restore dependencies.
    ```
    msbuild.exe /t:Restore
    ```
1. Open the solution in Visual Studio (LowILAccessDeniedDbg.sln).
1. Build/start the `SandboxLauncher` project.\
  This will launch and attach the debugger to `SandboxLauncher.exe`.
1. When you see the prompt `Please attach a debugger`, attach Visual Studio to `SandboxTest.exe`, then press ENTER.\
  Alternatively, use the [Microsoft Child Process Debugging Power Tool](https://marketplace.visualstudio.com/items?itemName=vsdbgplat.MicrosoftChildProcessDebuggingPowerTool) to automatically attach to `SandboxTest.exe`.

## Expected Result

Valid HTTP response message, example:
```
[SUCCESS]
HTTP STATUS [200]

HTTP CONTENT:
enableScripts: false
```

## Actual Result

`E_ACCESSDENIED` is thrown and handled.
```
[FAILURE]
[0x80070005] A network capability is required to access this network resource
```

## Notes

- `SandboxTest.exe` **will** succeed under the following cases:
  - The debugger is never attached to that child process.
  - `SandboxTest` is launched as the main project (as a non-child standalone), even if a debugger is attached.\
    This happens because the process is started with a Medium Integrity Level (or High, if Visual Studio is started as an Administrator).
- The error message is frequently found in UWP apps that don't have the `Private networks` capability in the application manifest.\
  This is not the root issue here.
- Using WinInet directly, the HTTP call succeeds under the same conditions.\
  Likely, the problem lies in the interaction of the VS debugger with the C++/WinRT/COM stack.
