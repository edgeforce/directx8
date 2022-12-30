//-----------------------------------------------------------------------------
// 
// Sample Name: VB Simple Server Sample
// 
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  A very simplistic Server application that can only connect route client messages.
  
Path
====
  Source: Mssdk\Samples\Multimedia\VBSamples\DirectPlay\SimpleServer

  Executable: Mssdk\Samples\Multimedia\VBSamples\DirectPlay\Bin

User's Guide
============
  Connect to a server, and make funny faces.

Programming Notes
=================
  The SimpleServer sample allows players to make funny faces at anyone else on the server.
  

  * Handle DirectPlay system messages.  See implemented DirectPlay8Event interfaces

  - Upon Receive event: 
  Checks to see if there is more than one player in the session.  If there is, notify 
  everyone that a funny face was made.  Otherwsie notify the client that they are 
  the only player in the session.
