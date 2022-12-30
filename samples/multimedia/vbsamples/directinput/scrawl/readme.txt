//-----------------------------------------------------------------------------
// Name: Scrawlb DirectInput Sample
// 
// Copyright (c) 1998-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


Description
===========
   The Scrawlb sample illustrates the use of DirectInput to create a simple drawing program
   It shows the use of callbacks to recieve mouse movement events

   
Path
====
   Source:     MSSDK\Samples\Multimedia\Vbsamples\DirectInput\ScrawlB
   Executable: MSSDK\Samples\Multimedia\vbsamples\DirectInput\Bin


User's Guide
============
   Left Click and drag on the canvas to draw
   Right Click will bring up a pop up menu with choices
	Speed 1 	slow mouse movement
	Speed 2 	fast mouse movement
	Speed 3 	faster mouse movement
	Clear		Clear the canvas
	Suspend		Change from pencil to cursor pointer
	Exit		Exit the application


Programming Notes
=================
  The application subclasses the Display window to capture ENTERMENU messages so that
  the cursor can be reset when select the menu. this is done through a call to
  SetWindowLong. Note that failure to comment out these lines while running the sample from
  within the Visual Basic environment will result in undefined behavior

   
