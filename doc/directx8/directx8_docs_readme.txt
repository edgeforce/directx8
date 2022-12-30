_______________________________________________________________________

DirectX 8.0 SDK Documentation Errata:

This section contains corrections to the following components of the DirectX SDK documentation:

  DirectX Audio (DirectMusic and DirectSound)
  DirectMusic Producer
  DirectX Graphics (C++)
  DirectX Graphics (Visual Basic)
  DirectX Input (C++)
  DirectPlay
  DirectShow

General
=======

Refer to the Accessibility documentation in the Platform SDK for
information on how to make your application accessible.


DirectX Audio (DirectMusic and DirectSound)
===========================================

-------------------
Sample Descriptions
-------------------
The path given for the C++ and Visual Basic samples is 
incorrect. All paths on the sample description pages should 
begin with "(SDK Root)\Samples", not "(SDK Root)\Sample".
 

DirectMusic Producer
=====================

Songs
------
Some of the descriptions of script methods make reference 
to segments and songs. Songs are not supported in this release.

Send Effects Topic
------------------
The following statement in the Send Effects topic is incorrect:

"You can place a Send effect at the end of an audiopath 
to route its audio output back to the beginning of any 
other audiopath."

This should read as follows:

"You can place a Send effect in a buffer's effects chain 
to route its audio output to any other buffer."


DirectX Graphics (C++)
======================

------------------
Overview Topics
------------------

Topic: Architectural Overview for Direct3D
----------------------------
The diagram for this topic is mislabeled. The texture values listed in the 
"Pixel Operations" section on the bottom left hand corner should be listed
as Tex0, Tex1, and Tex 2 instead of Tex0, Tex0, Tex0.


Topic: Constant Value Syntax
----------------------------
The example shown for the pixel shader is incorrect. A 
valid example is:

asm
{
    ps.1.0
    tex t0
    mov r0, t0
}


Topic: Effect File Components
-----------------------------
The second technique in this topic is incorrect. The correct code is:

technique t1
{
    pass p0
    {
        Texture[0] = <tex0>;
        
        ColorOp[0] = SelectArg1;
        ColorArg1[0] = Texture;
        ColorOp[1] = Disable;  
    }
    
    pass p1
    {
        AlphaBlendEnable = True;
        SrcBlend = One;
        DestBlend = One;

        Texture[1] = <tex1>;
               
        ColorOp[1] = SelectArg1;
        ColorArg1[1] = Texture;
        ColorOp[1] = Disable;  
    }
}


Topic: Lost Devices and Retrieved Data
--------------------------------------
This topic does not apply to IDirect3DDevice8::UpdateTexture.


Topic: Performance Optimizations
--------------------------------
Add the following tips:

  *  Minimize vertex buffer switches.
  *  Use static vertex buffers where possible.
  *  Use one big static vertex buffer per FVF for static 
     objects, rather than one per object.
  *  Do not copy to output registers unless necessary in 
     shaders. For example:

       mad oD0, r1, c[2], c[3] 

     rather than:

       mad r1,c[2],c[3]
       mov oD0, r1


Topic: Setting Material Properties
-----------------------------------
The code sample does not initialize the emissive properties,
which would be undefined. The emissive property needs to be 
initalized before the material can be used. The code sample 
below can be added to the code sample in this topic to make 
it complete.

// Set the RGBA for emissive color.
mat.Emissive.r = 0.0f;
mat.Emissive.g = 0.0f;
mat.Emissive.b = 0.0f;
mat.Emissive.a = 0.0f;


Topic: Using Dynamic Vertex and Index Buffers
---------------------------------------------
The second usage style is missing a call to Unlock. The correct code for usage
style 2 is below:

for loop()
{
    pVB->Lock(...D3DLOCK_DISCARD...); //Ensures that hardware doesn't 
                                      //stall by returning a new 
                                      //pointer.
    Fill data (optimally 1000s of vertices/indices, no fewer) in pBuffer.
    pBuffer->Unlock
    for loop( 100s of times )
    {
        Change State
        DrawPrimitive() or DrawIndexPrimitives() //Tens of primitives
    }
}


Topic: Vertex Shader Architecture
---------------------------------------------
The diagram for this topic is missing the homogenuous position as an output.



---------------------
Reference Section
---------------------

Topic: D3DCAPS8
------------------
The description of Presentation Intervals should read:
Bit mask of values representing what presentation swap 
intervals are available.


Topic: D3DFVF_TEXCOORDSIZEn
----------------------------
The FVF declared in this section is invalid. The invalid 
FVF need the flag D3DFVF_TEX2 added in order to be correct.

The correct FVF declaration would be:

DWORD dwFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE |
              D3DFVF_SPECULAR | D3DFVF_TEX2 |
              D3DFVF_TEXCOORDSIZE1(0) |  \\ Uses 1-D texture coordinates for
                                         \\ texture coordinate set 1 (index 0).
              D3DFVF_TEXCOORDSIZE2(1);   \\ And 2-D texture coordinates for 
                                         \\ texture coordinate set 2 (index 1).


Topic: D3DRENDERSTATETYPE
--------------------------
The code sample for D3DRS_TWEENFACTOR should be:
pd3dDevice8->SetRenderState(D3DRS_TWEENFACTOR, *((DWORD*)&TweenFactor));

The last sentence for the description of D3DRS_ALPHABLENDENABLE 
is incorrect and can be ignored.


Topic: D3DXCOLOR Extensions
----------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXLoadSurfaceFromFileA
-------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromFileA is called and the surface was not already 
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call IDirect3DTexture8::AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromFileInMemory
--------------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromFileInMemory is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call IDirect3DTexture8::AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromFileW
-------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromFileW is called and the surface was not already 
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call IDirect3DSurface8::AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromMemory
--------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be
updated. If D3DXLoadSurfaceFromMemory is called and the surface was not already 
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call IDirect3DTexture8::AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromResourceA
-----------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromResourceA is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call IDirect3DTexture8::AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromResourceW
-----------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromResourceW is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call IDirect3DTexture8.AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromSurface
---------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DXLoadSurfaceFromSurface is called and the surface was not already
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call IDirect3DTexture8::AddDirtyRect on the surface.


Topic: D3DXLoadVolumeFromMemory
-------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the volume texture will not cause the 
dirty rectangle to be updated. If D3DXLoadVolumeFromMemory is called and the 
texture was not already dirty (this is unlikely under normal usage scenarios), 
the application needs to explicitly call IDirect3DVolumeTexture8::AddDirtyBox on
the volume texture.


Topic: D3DXLoadVolumeFromVolume
-------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the volume texture will not cause the 
dirty rectangle to be updated. If D3DXLoadVolumeFromVolume is called and the 
surface was not already dirty (this is unlikely under normal usage scenarios), 
the application needs to explicitly call IDirect3DVolumeTexture8::AddDirtyBox on
the surface.


Topic: D3DXFilterCubeTexture
----------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the texture will not cause the dirty 
rectangle to be updated. If D3DXFilterCubeTexture is called and the surface was 
not already dirty (this is unlikely under normal usage scenarios), the 
application needs to explicitly call IDirect3DCubeTexture8.AddDirtyRect on the 
texture.


Topic: D3DXFilterTexture
------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the texture will not cause the dirty 
rectangle to be updated. If D3DXFilterTexture is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call IDirect3DTexture8::AddDirtyRect on the texture.


Topic: D3DXFilterVolumeTexture
------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the texture will not cause the dirty
rectangle to be updated. If D3DXFilterVolumeTexture is called and the surface
was not already dirty (this is unlikely under normal usage scenarios), the 
application needs to explicitly call IDirect3DVolumeTexture8::AddDirtyBox on the
texture.


Topic: D3DXMATRIX Extensions
-----------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXMatrixPerspectiveOffCenterLH
---------------------------------------
The correct description for the parameters are:
pOut
  [in, out] Pointer to the D3DXMATRIX structure that is the result of the operation.
l
  [in] Minimum x-value of view-volume.
r
  [in] Maximum x-value of view-volume.
t
  [in] Minimum y-value of view-volume.
b
  [in] Maximum y-value of view-volume.
zn
  [in] Minimum z-value of the view volume.
zf
  [in] Maximum z-value of the view volume.


Topic: D3DXMatrixPerspectiveOffCenterRH
---------------------------------------
The correct description for the parameters are:
pOut
  [in, out] Pointer to the D3DXMATRIX structure that is the result of the operation.
l
  [in] Minimum x-value of view-volume.
r
  [in] Maximum x-value of view-volume.
t
  [in] Minimum y-value of view-volume.
b
  [in] Maximum y-value of view-volume.
zn
  [in] Minimum z-value of the view volume.
zf
  [in] Maximum z-value of the view volume.


Topic: D3DXPLANE Extensions
---------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXQUATERNION Extensions
--------------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXVECTOR2 Extensions
-----------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXVECTOR3 Extensions
------------------------------
The correct header for this topic is: D3dx8math.h


Topic: D3DXVECTOR4 Extensions
-----------------------------
The correct header for this topic is: D3dx8math.h


Topic: IDirect3DCubeTexture8::AddDirtyRect
------------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: IDirect3DCubeTexture8::LockRect
--------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
IDirect3DDevice8::UpdateTexture for more information.


Topic: IDirect3DDevice8::CopyRects
----------------------------------
The following is missing from the remarks section:
If the destination surface is a level-zero of a texture, then it will be marked
dirty. See IDirect3DDevice8::UpdateTexture and IDirect3DTexture8::AddDirtyRect 
for more details.


Topic: IDirect3DDevice8::DrawPrimitiveUP
----------------------------------------
The following comment was added to the remarks section:
The vertex data passed to DrawPrimitiveUP does not need to persist after the
call. Direct3D completes its access to that data prior to returning from the
call.


Topic: IDirect3DDevice8::DrawIndexedPrimitiveUP
-----------------------------------------------
The following comment was added to the remarks section:
The vertex data passed to DrawIndexPrimitiveUP does not need to persist after
the call. Direct3D completes its access to that data prior to returning from the
call.


Topic: IDirect3DDevice8::SetStreamSource
----------------------------------------
Disregard the following sentence in the Remarks section:

When the texture is no longer needed, set the texture at 
the appropriate stage to NULL.


Topic: IDirect3DDevice8::UpdateTexture
--------------------------------------
Disregard the following sentence in the Remarks section:
When the texture is no longer needed, set the texture at the appropriate stage
to NULL.

The followin is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: IDirect3DSurface8::LockRect
----------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
IDirect3DDevice8::UpdateTexture for more information.


Topic: IDirect3DTexture8::AddDirtyRect
--------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: IDirect3DTexture8::LockRect
----------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
IDirect3DDevice8::UpdateTexture for more information.


Topic: IDirect3DVolume8::LockBox
--------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockBox is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
IDirect3DDevice8::UpdateTexture for more information.


Topic: IDirect3DVolumeTexture8::AddDirtyBox
-------------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: IDirect3DVolumeTexture8::LockBox
---------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockBox is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
IDirect3DDevice8::UpdateTexture for more information.


Topic: mad - Pixel Shader
-------------------------
The remarks section is incorrect. The correct remarks section is below.
This instruction performs a multiply-accumulate operation. It takes the first 
two arguments, multiplies them together, and adds them to the remaining 
input/source argument, and places that into the result register. 

This instruction performs the multiply-add based on the following formula.
tSrc0 * tSrc1 + tSrc2

The following example shows how this instruction might be used.
mad  d, s0, s1, s2;    d = s0 * s1 + s2



DirectX Graphics (Visual Basic)
===============================

---------------
Overview Topics
---------------


Topic: Constant Value Syntax
----------------------------
The example shown the pixel shader is incorrect. A valid sample is shown below:

asm
{
    ps.1.0
    tex t0
    mov r0, t0
}


Topic: Lost Devices and Retrieved Data
--------------------------------------
This topic does not apply to IDirect3DDevice8::UpdateTexture.


Topic: Setting Material Properties
----------------------------------
The code sample does not initialize the emissive properties, 
which would be undefined. The emissive property needs to 
be initalized before the material can be used. The code 
sample below can be added to the code sample in this topic to
make it complete.

' Set the RGBA for emissive color.
mat.Emissive.r = 0#
mat.Emissive.g = 0#
mat.Emissive.b = 0#
mat.Emissive.a = 0#


Topic: Using Dynamic Vertex and Index Buffers
---------------------------------------------
The second usage style is missing a call to Unlock. The correct code for usage
style 2 is below:

for loop()
{
    pVB->Lock(...D3DLOCK_DISCARD...); //Ensures that hardware doesn't 
                                      //stall by returning a new 
                                      //pointer.
    Fill data (optimally 1000s of vertices/indices, no fewer) in pBuffer.
    pBuffer->Unlock
    for loop( 100s of times )
    {
        Change State
        DrawPrimitive() or DrawIndexPrimitives() //Tens of primitives
    }
}


------------------
Reference Section
------------------
 
Topic: CONST_D3DRENDERSTATETYPE
--------------------------------
The last sentence for the description of D3DRS_ALPHABLENDENABLE 
is incorrect and can be ignored.

Topic: D3DCAPS8
---------------
The description of Presentation Intervals should read:
  Bit mask of values representing what presentation swap 
  intervals are available.


Topic: D3DX8.LoadSurfaceFromFile
--------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DX8.LoadSurfaceFromFileA is called and the surface was not already 
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call Direct3DTexture8.AddDirtyRect on the surface.


Topic: D3DX8.LoadSurfaceFromFileInMemory
----------------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DX8.LoadSurfaceFromFileInMemory is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call Direct3DTexture8.AddDirtyRect on the surface.


Topic: D3DX8.LoadSurfaceFromMemory
----------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be
updated. If D3DXLoadSurfaceFromMemory is called and the surface was not already 
dirty (this is unlikely under normal usage scenarios), the application needs to 
explicitly call Direct3DTexture8.AddDirtyRect on the surface.


Topic: D3DXLoadSurfaceFromResource
----------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DX8.LoadSurfaceFromResource is called and the surface was not 
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call Direct3DTexture8.AddDirtyRect on the surface.


Topic: D3DX8.LoadSurfaceFromSurface
-----------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface will not cause the dirty rectangle to be 
updated. If D3DX8.LoadSurfaceFromSurface is called and the surface was not
already dirty (this is unlikely under normal usage scenarios), the application 
needs to explicitly call Direct3DTexture8.AddDirtyRect on the surface.


Topic: D3DX8.LoadVolumeFromMemory
---------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the volume texture will not cause the 
dirty rectangle to be updated. If D3DX8.LoadVolumeFromMemory is called and the 
texture was not already dirty (this is unlikely under normal usage scenarios), 
the application needs to explicitly call Direct3DVolumeTexture8.AddDirtyBox on
the volume texture.


Topic: D3DX8.LoadVolumeFromVolume
---------------------------------
The following is missing from the remarks section:
Writing to a non-level-zero surface of the volume texture will not cause the 
dirty rectangle to be updated. If D3DX8.LoadVolumeFromVolume is called and the 
surface was not already dirty (this is unlikely under normal usage scenarios), 
the application needs to explicitly call Direct3DVolumeTexture8.AddDirtyBox on
the surface.


Topic: D3DXMatrixPerspectiveOffCenterLH
---------------------------------------
The correct description for the parameters are:
pOut
  [in, out] Pointer to the D3DXMATRIX structure that is the result of the operation.
l
  [in] Minimum x-value of view-volume.
r
  [in] Maximum x-value of view-volume.
t
  [in] Minimum y-value of view-volume.
b
  [in] Maximum y-value of view-volume.
zn
  [in] Minimum z-value of the view volume.
zf
  [in] Maximum z-value of the view volume.


Topic: D3DXMatrixPerspectiveOffCenterRH
---------------------------------------
The correct description for the parameters are:
pOut
  [in, out] Pointer to the D3DXMATRIX structure that is the result of the operation.
l
  [in] Minimum x-value of view-volume.
r
  [in] Maximum x-value of view-volume.
t
  [in] Minimum y-value of view-volume.
b
  [in] Maximum y-value of view-volume.
zn
  [in] Minimum z-value of the view volume.
zf
  [in] Maximum z-value of the view volume.


Topic: Direct3DCubeTexture8.AddDirtyRect
----------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DCubeTexture8.LockRect
------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
Direct3DDevice8.UpdateTexture for more information.


Topic: Direct3DDevice8.CopyRects
--------------------------------
The following is missing from the remarks section:
If the destination surface is a level-zero of a texture, then it will be marked
dirty. See Direct3DDevice8.UpdateTexture and Direct3DTexture8.AddDirtyRect for
more details.


Topic: Direct3DDevice8.SetMaterial
----------------------------------
The Error Codes section is incorrect. The correct wording for 
this section is:

If the method fails, an error is raised and Err.Number can 
be set to D3DERR_INVALIDCALL. For information on trapping 
errors, see the Microsoft(r) Visual Basic(r) Error Handling topic.


Topic: Direct3DDevice8.UpdateTexture
--------------------------------------------------------------------------------
The followin is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DSurface8.LockRect
--------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
Direct3DDevice8.UpdateTexture for more information.


Topic: Direct3DTexture8.AddDirtyRect
------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DTexture8.AddDirtyRect
------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DTexture8.LockRect
--------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockRect is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
Direct3DDevice8.UpdateTexture for more information.


Topic: Direct3DVolume8.AddDirtyRect
-----------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DVolume8.LockBox
------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockBox is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
Direct3DDevice8.UpdateTexture for more information.


Topic: Direct3DVolumeTexture8.AddDirtyBox
-----------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a 
texture. For sub-levels, it is assumed that the corresponding (scaled) rectangle
or box is also dirty. Dirty regions are automatically recorded when LockRect or
LockBox is called without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. Also, the
destination surface of a CopyRects call is marked dirty.


Topic: Direct3DVolumeTexture8.LockBox
-------------------------------------
The following is missing from the remarks section:
For performance reasons, dirty regions are only recorded for level zero of a
texture. Dirty regions are automatically recorded when LockBox is called
without D3DLOCK_NO_DIRTY_UPDATE or D3DLOCK_READONLY. See 
Direct3DDevice8.UpdateTexture for more information.


Topic: mad - Pixel Shader
-------------------------
The remarks section is incorrect. The correct remarks section is below.
This instruction performs a multiply-accumulate operation. It takes the first 
two arguments, multiplies them together, and adds them to the remaining 
input/source argument, and places that into the result register. 

This instruction performs the multiply-add based on the following formula.
tSrc0 * tSrc1 + tSrc2

The following example shows how this instruction might be used.
mad  d, s0, s1, s2;    d = s0 * s1 + s2



DirectX Input (C++)
===============================

---------------
Reference Topics
---------------


Topic: IDirectInput8::EnumDevices
----------------------------------
The declared type for the lpCallback parameter is incorrectly identified as LPDIENUMCALLBACK. It should be LPDIENUMDEVICESCALLBACK.


Topic: IDirectInput8::GetProperty
----------------------------------
The following additional properties have been added.

DIPROP_VIDPID 
Read-only device property that retrieves the vendor ID and product ID of a HID device. This property is of type DIPROPDWORD and contains both values. These two WORD values are combined in the    DIPROPDWORD.dwData field so the values should be extracted as follows:

        wVendorID  = LOWORD( DIPROPDWORD.dwData );
        wProductID = HIWORD( DIPROPDWORD.dwData );

This property applies to the entire device, rather than to any particular object, so the DIPROPHEADER.dwHow field must be DIPH_DEVICE.

DIPROP_TYPENAME
A predefined property which retrieves the type name of a device. For most game controllers this is the registry key name under REGSTR_PATH_JOYOEM from which static device settings may be retrieved, but predefined joystick types have special names consisting of a "#" character followed by a character dependent upon the type. This value may not be available for all devices. 

DIPROP_CALIBRATION 
A predefined property which allows the application to access the information that DirectInput uses to manipulate axes that require calibration. This property exists primarily for control panel-type applications. Normal applications should have no need to deal with calibration information.

You can access the calibration mode property for a particular axis by setting the DIPROPHEADER.dwHow field of the DIPROPHEADER structure to DIPH_BYID or to DIPH_BYOFFSET and setting the DIPROPHEADER.dwObj field to the object id or offset (respectively).

Control panel applications that set new calibration data must also invoke the IDirectInputJoyConfig::SendNotify method to notify other applications of the change in calibration.


Topic: IDirectInput8::SetProperty
----------------------------------
The following additional property has been added.

DIPROP_CALIBRATION 
A predefined property which allows the application to access the information that DirectInput uses to manipulate axes that require calibration. This property exists primarily for control panel-type applications. Normal applications should have no need to deal with calibration information.

You can access the calibration mode property for a particular axis by setting the DIPROPHEADER.dwHow field of the DIPROPHEADER structure to DIPH_BYID or to DIPH_BYOFFSET and setting the DIPROPHEADER.dwObj field to the object id or offset (respectively).

Control panel applications that set new calibration data must also invoke the IDirectInputJoyConfig::SendNotify method to notify other applications of the change in calibration.



DirectPlay
===========

----------------------
Overview Topics
----------------------

Cancelling Outstanding Sends
----------------------------
  Applications should call CancelAsyncOperation to cancel all
  outstanding Sends prior to calling Close or doing a final 
  Release call on the IDirectPlay8Client, IDirectPlay8Server, or
  IDirectPlay8Peer interfaces.  Failing to do so will cause 
  unpredictable results.


Client-Server Sessions
----------------------
  When the server closes a session, it gets DPN_MSGID_DESTROY_PLAYER 
  for all connected players. Because the server knows that it
  is disconnecting, this is normal behavior, and the dwReason member 
  of the associated structure will be set to
  DPNDESTROYPLAYERREASON_NORMAL.The
  DPNDESTROYPLAYERREASON_SESSIONTERMINATED value is only set 
  for unexpected disconnections.


TCP/IP Port Settings
--------------------

  Application developers that choose to override the default DirectPlay
  8 dialog for TCP/IP are strongly urged to implement a solution that
  allows the user to override the port used for a connect or 
  enumeration.  One possible solution is to allow users to follow 
  the host name with colon and then the port, as implemented by the
  default DirectPlay 8 TCP/IP dialog (i.e. "host.domain.com:8090" ).
  Another possible solution would be to add another field to the UI
  that allows the user to enter a port.

Using the Netmon Parser
-----------------------
  To parse ports other than the default DirectPlay ports,
  create two new DWORD values under the
  HKEY_CURRENT_USER\Microsoft\DirectPlay\Parsers key
  
    MinUserPort=x
    MaxUserPort=y
  
  x and y define the range you want to parse in addition 
  to the default DirectPlay ports. They can be the same value if 
  you only need one custom port.

-----------------
Reference Topics
-----------------

Connect() methods
------------------
  After a call to Connect() succeeds, all outstanding Enumerations will 
  be canceled with the return of DPNERR_USERCANCEL.

SetApplicationDescription Methods
---------------------------------
  You cannot set the dwMaxPlayers parameter to a smaller value 
  than the current number of players in the session.


IDirectPlay8Peer::CancelAsyncOperation IDirectPlay8Client::CancelAsyncOperation
-----------------------------------------
  DPN_CANCELCONNECT and DPN_CANCELSEND should be DPNCANCEL_CONNECT
  and DPNCANCEL_SEND.




DirectShow
===========

-----------------
Reference Topics
-----------------

Media Locator
--------------

  The description of the SFN_VALIDATEF_USELOCAL flag is incorrect.  It 
  should read:

    Always use a local file, even if a version of the file exists 
    on the network.

  For more information, see IMediaLocator::FindMediaFile and 
  IRenderEngine::SetSourceNameValidation.

DV Muxer Filter
-----------------------
  The following sentence is incorrect:

	If no audio pins are connected, the filter produces 
    DV frames with no audio.
    
  It should read:
    If no audio pins are connected, the output 
    contains the audio data from the incoming DV frames.
    This might be silence, or valid audio data.

DirectX Media Objects
----------------------

  The descriptions of the IMediaObject::GetInputType and GetOutputType
  methods are incomplete.  They should include the following 
  information:

	The pmt parameter can be NULL.  In that case, the method 
        returns	S_OK if the type index is in range.  Otherwise, 
        it returns DMO_E_NO_MORE_ITEMS or another error code.



DirectShow (Visual Basic)
=========================

  DirectShow Visual Basic documentation is presently tied in closely 
  with the C++ documentation.  Information on DirectShow Visual 
  Basic programming, including use of DirectShow Editing 
  Services, is located exclusively in an appendix to the 
  C++ document.  There is no separate Visual Basic Documentation.

MSWebDVD
=========================
  The documentation states that you can change the size of the 
  video display through the methods GetClipVideoRect and 
  GetVideoSize.  This is incorrect. The size of the clipped 
  video rectangle is changed through the SetClipVideoRect 
  method. The size of the native video obviously cannot be modified.

CBaseInputPin
=========================
The reference documentation lists the following methods which must be overridden when implementing this class: BeginFlush, EndFlush, and Receive. Since CBaseInputPin derives from CBasePin, you must also override certain CBasePin methods as well, including CBasePin::CheckMediaTypes and CBasePin::GetMediaType. See the reference page for CBasePin for more details.

