//-----------------------------------------------------------------------------
// Name: ActionMap DirectInput Sample
// 
// Copyright (c) 1998-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


Description
===========
   The ActionMap sample illustrates the use of DirectInputs ActionMapping feature.
   this feature allows you to assign various keys to constants that are kept in
   a queue that the application can read at any time. This sample also show how
   those mappings can be user configurable.

   
Path
====
   Source:     MSSDK\Samples\Multimedia\Vbsamples\DirectInput\ActionMap
   Executable: MSSDK\Samples\Multimedia\vbsamples\DirectInput\Bin


User's Guide
============
   None
	

Programming Notes
=================
   ActionMap.cls should not be used unmodified. For the puropose of this sample it will
   query for any and all input devices and does not diffrentiate where the input is comming
   from. Most applications will want to modify the class to respond to only 1 given input
   or diffrentiate the input devices into different players or purposes.

   This sample makes use of common DirectX code (consisting of helper functions,
   etc.) that is shared with other samples on the DirectX SDK. All common
   classes and modules can be found in the following directory:
      Mssdk\Samples\Multimedia\VBSamples\Common

