DirectShow SDK Samples README
------------------------------

The DirectShow SDK samples are subdivided into directories according
to their major function:

	BaseClasses
	BDA
	Capture
	Common (shared source files)
	DMO
	DVD
	Editing
	Filters
	Misc
	Players

There are also several Visual Basic samples in the VBSamples\DirectShow
directory.  Even if you are exclusively a Visual C++ developer, we recommend
that you review the Visual Basic sample executables, which are accessible from
the SDK's "Visual Basic Samples" Start Menu folder.  Applications like
SlideShow and Trimmer demonstrate many DirectShow Editing Servies (DES)
capabilities that are not yet demonstrated in C++ samples.  The DexterVB tool
is also useful for exploring and visualizing XML timelines used with DES.


Setting up the build environment
--------------------------------
In your Visual C++ build environment, be sure to set your Include and Lib 
directories to point to the DirectX 8 SDK's include and lib path.
The DirectX 8 SDK directories must be the first directories in the search path.
Otherwise, you may encounter linker errors, as you would continue using
the headers and libraries supplied with Visual C++ or with an earlier version
of the Platform SDK.

Building the ASFCopy sample additionally requires the Microsoft Windows Media
Format SDK and a valid software certificate.  The SDK download page is located 
at http://msdn.microsoft.com/workshop/imedia/windowsmedia/sdk/wmsdk.asp, with
links to the SDK itself and information for obtaining a certificate.  Once 
installed, the ASFCopy project link settings (or alternatively Visual C++ 
Tools->Options->Directories library path) must be updated to add the path for
WMStub.lib and WMVCore.lib provided with the Windows Media Format SDK.

For more information, see "Setting Up The Build Environment" in the 
"Getting Started" section of the DirectX 8 documentation.


Building the Samples
--------------------

There are two ways to build the SDK samples:
	1) Using the DShow.DSW Visual C++ workspace
	2) Building individual projects within their VC++ workspaces


Using the Workspace:

The easiest way to build the samples is to open the DShow.DSW workspace,
which is installed to the DirectShow samples root directory.  This workspace
includes references to each of the project files for the individual samples.
You can select a project (in the FileView pane) and right-click to display a
build menu for the project.  Select 'Build' to build a project and its 
dependencies.  Note that if you select 'Build (selection only), you will not
automatically build any dependencies (like the base classes, if required).  

You can also build or rebuild all of the projects together by selecting
'Build->Batch Build' from the Visual C++ main menu.  Projects included in
this workspace know their dependencies and will build them if needed.

Some of the projects listed in the FileView pane do not build applications,
but instead build support libraries used by other projects within the main
workspace.  For example, the GargleDMO sample depends on the 'ControlBase'
and 'MedParamBase' projects that are contained within the workspace.  
If you build GargleDMO, Visual C++ will automatically build the 
ControlBase and MedParamBase libraries if they need to be built, and those
libraries will be linked into the GargleDMO DLL.


Building Individual Projects:

You can open any of the provided project or workspace files in Visual C++
if you want to build, modify, or debug an individual sample.  If you build
outside of the DShow.DSW workspace, however, you might need to first build the
DirectShow Base Classes, which are installed to the "BaseClasses" directory.

The DirectMedia 6 SDK installed binaries for the BaseClasses
(as strmbase.lib and the debug build, strmbasd.lib), which were compiled versions
of the base classes previously installed into the SDK's classes\base directory.  
DirectX 8 ships the source code for these base classes as a sample project,
which allows you to modify the classes and build them with Visual C++.
Since many of the samples (especially the sample filters) need to link with 
STRMBASE.LIB (or STRMBASD.LIB), you must first build the base classes 
to allow you to build other sample filters and applications that use them.

Don't forget to build both Debug and Release versions of the BaseClasses
project if you intend to build both Debug and Release versions of the samples.
