//-----------------------------------------------------------------------------
// Name: SphereMap Direct3D Sample
// 
// Copyright (c) 1998-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


Description
===========
   The SphereMap sample demonstrates an enviroment-mapping technique called 
   sphere-mapping. Environment-mapping is a technique in which the environment
   surrounding a 3D object (such as the lights, etc.) are put into a texture
   map, so that the object can have complex lighting effects without expensive
   lighting calculations.

   Note that not all cards support all features for all the various environment
   mapping techniques (such as cubemapping and projected textures). For more
   information on environment mapping, cubemapping, and projected textures, 
   refer to the DirectX SDK documentation. 


Path
====
   Source:     MSSDK\Samples\Multimedia\D3D\EnvMapping\SphereMap
   Executable: MSSDK\Samples\Multimedia\D3D\Bin


User's Guide
============
   The following keys are implemented. The dropdown menus can be used for the
   same controls.
      <Enter>     Starts and stops the scene
      <Space>     Advances the scene by a small increment
      <F1>        Shows help or available commands.
      <F2>        Prompts user to select a new rendering device or display mode
      <Alt+Enter> Toggles between fullscreen and windowed modes
      <Esc>       Exits the app.


Programming Notes
=================
   Sphere-mapping uses a precomputed (at model time) texture map which contains
   the entire environment as reflected by a chrome sphere. The idea is to
   consider each vertex, compute it's normal, find where the normal matches up
   on the chrome sphere, and then assign that texture coordinate to the vertex.

   Although the math is not complicated, this still involves computations for
   each vertex for every frame. Fortunately, Direct3D has a texture-coordinate
   generation feature that can be used to do this for us. The relevant
   renderstate operation is D3DTSS_TCI_CAMERASPACENORMAL, which takes the
   normal of the vertex in camera space and pumps it through a texture
   transform to generate texture coordinates. We use this and simply set up our
   texture matrix to do the rest. In this simple case, the matrix just has
   to scale and translate the texture coordinates to get from camera space 
   (-1, +1) to texture space (0,1).

   This sample makes use of common DirectX code (consisting of helper functions,
   etc.) that is shared with other samples on the DirectX SDK. All common
   headers and source code can be found in the following directory:
      Mssdk\Samples\Multimedia\Common
