//------------------------------------------------------------------------------
// File: Main.cpp
//
// Desc: DirectShow sample code - simple movie player console application.
//
// Copyright (c) 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


//
//   This program uses the PlayCutscene() function provided in cutscene.cpp.  
//   It is only necessary to provide the name of a file and the application's 
//   instance handle.
//
//   If the file was played to the end, PlayCutscene returns S_OK.
//   If the user interrupted playback, PlayCutscene returns S_FALSE.
//   Otherwise, PlayCutscene will return an HRESULT error code.
//
//   Usage: cutscene <required file name>
//

#include <windows.h>

#include "cutscene.h"

#define USAGE \
        TEXT("Cutscene is a console application that demonstrates\r\n")      \
        TEXT("playing a movie at the beginning of your game.\r\n\r\n")       \
        TEXT("Please provide a valid filename on the command line.\r\n")     \
        TEXT("\r\n            Usage: cutscene <filename>\r\n")               \


//
// Main program code
//
int APIENTRY
WinMain (
         HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPTSTR lpszMovie,
         int nCmdShow
         )
{
    HRESULT hr;

    // If no filename is specified, show an error message and exit
    if (lpszMovie[0] == TEXT('\0'))
    {
        MessageBox(NULL, USAGE, "Cutscene Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    // Play movie
    hr = PlayCutscene(lpszMovie, hInstance);

    return hr;
}


