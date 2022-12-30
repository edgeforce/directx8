//-----------------------------------------------------------------------------
// 
// Sample Name: DXVB Messenger Server Sample
// 
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========

  DXVB Messenger Server is the server portion of a client/server instant 
  messaging application.  This sample requires the use of ActiveX Data Object
  (ADO) to maintain the data needed.  If you are running Windows 9x, you must 
  have the latest version of ADO installed on your machine to compile or run 
  this sample.  You can install the latest ADO while Visual Basic Professional 
  is being installed or you can download the latest ADO from the web at:
  http://www.microsoft.com/data/ado/.  ADO is installed by default with 
  Visual Basic 6 Enterprise.
  
Path
====
  Source: Mssdk\Samples\Multimedia\VBSamples\DirectPlay\DXVBMessenger\Server

  Executable: Mssdk\Samples\Multimedia\VBSamples\DirectPlay\Bin

User's Guide
============
  Log onto a server, add friends, and send instant messages.

Programming Notes
=================
  * Handle DirectPlay system messages.  See implemented DirectPlay8Event interfaces
  - Upon Receive event (the following messages can be received): 
    'Login messages
    Msg_Login - Get login information, verify against the database
    Msg_CreateNewAccount - A new account needs to be created, try to create in database
    Msg_AddFriend - Add a friend to this users list
    Msg_BlockFriend - Block someone in this users list
    Msg_SendMessage - User is sending a message to someone, pass it on
