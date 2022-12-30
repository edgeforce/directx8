//-----------------------------------------------------------------------------
// 
// Sample Name: VB EnumDevices Sample
// 
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  The EnumDevices sample shows how to enumerate and create playback 
  and capture devices.

Path
====
  Source: Mssdk\Samples\Multimedia\VBSamples\DirectSound\EnumDevices 

  Executable: Mssdk\Samples\Multimedia\VBSamples\DirectSound\Bin

User's Guide
============
  Select a playback and capture device from the dropdown lists. Click Create.

Programming Notes
=================
  This sample was intended to be very simple, showing the basics how to 
  enumerate the DirectSound and DirectSoundCapture devices.
  
  To enumerate DirectSound devices call GetDSEnum.
  
  To enumerate DirectSoundCapture devices call GetDSCaptureEnum.

