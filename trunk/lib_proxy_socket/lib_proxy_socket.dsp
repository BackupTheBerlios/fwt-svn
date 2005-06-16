# Microsoft Developer Studio Project File - Name="lib_proxy_socket" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lib_proxy_socket - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "lib_proxy_socket.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "lib_proxy_socket.mak" CFG="lib_proxy_socket - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "lib_proxy_socket - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lib_proxy_socket - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "LIB_PROXY_SOCKET"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lib_proxy_socket - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../LIB"
# PROP Intermediate_Dir "../OBJ/LIB_PROXY_SOCKET/RELEASE"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /MT /W3 /GX /O2 /I "../COMMON" /FI"../COMMON/BUILD/PLATFORM/win32.x86" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX"../OBJ/LIB_PROXY_SOCKET/RELEASE/lib_proxy_socket.pch" /J /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../OBJ/LIB_PROXY_SOCKET/RELEASE/lib_proxy_socket.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "lib_proxy_socket - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../LIB/DEBUG"
# PROP Intermediate_Dir "../OBJ/LIB_PROXY_SOCKET/DEBUG"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /Gr /MTd /W3 /Gm /GX /ZI /Od /I "../COMMON" /FI"../COMMON/BUILD/PLATFORM/win32.x86" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "Trace" /D "Debug" /YX"../OBJ/LIB_PROXY_SOCKET/DEBUG/lib_proxy_socket.pch" /J /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../OBJ/LIB_PROXY_SOCKET/DEBUG/lib_proxy_socket.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF

# Begin Target
# Name "lib_proxy_socket - Win32 Release"
# Name "lib_proxy_socket - Win32 Debug"
# Begin Group "Source Files"
# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"
# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Message files"
# PROP Default_Filter "*.msg"
# Begin Source File
SOURCE=lib_proxy_socket_msg.msg
# Begin Custom Build
InputPath=lib_proxy_socket_msg.msg
InputName=lib_proxy_socket
BuildCmds= \
        ..\COMMON\MSGPROC\msgproc.exe
"$(InputName).c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
"$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# End Group

# End Target
# End Project
