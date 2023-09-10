@rem Script to build FAExt with MSVC.
@rem Copyright (C) 2022+ KionX.
@rem
@rem Open a "Visual Studio Command Prompt" (either x86 or x64).
@rem Then cd to this directory and run this script. Use the following
@rem options (in order), if needed. The default is a dynamic release build.
@rem
@rem   nogc64   disable LJ_GC64 mode for x64
@rem   debug    emit debug symbols
@rem   amalg    amalgamated build
@rem   static   static linkage

@if not defined INCLUDE goto :FAIL

@setlocal
@rem Add more debug flags here, e.g. DEBUGCFLAGS=/DLUA_USE_APICHECK
@set DEBUGCFLAGS=/DLUA_USE_ASSERT
@set LJCOMPILE=cl /nologo /c /O2 /GL /W3 /MP /D_CRT_SECURE_NO_DEPRECATE /arch:SSE2 /Zp4
@set LJLINK=link /nologo /LTCG
@set LJMT=mt /nologo
@set LJDLLNAME=FAExt.dll
@set LJLIBNAME=FAExt.lib
@set BUILDTYPE=release

@if "%1" neq "debug" goto :NODEBUG
@shift
@set BUILDTYPE=debug
@set LJCOMPILE=%LJCOMPILE% /Zi %DEBUGCFLAGS%
@set LJLINK=%LJLINK% /opt:ref /opt:icf /incremental:no
:NODEBUG
@set LJLINK=%LJLINK% /%BUILDTYPE%
%LJCOMPILE% /MT /DLUA_BUILD_AS_DLL FAExt.cpp
@if errorlevel 1 goto :BAD
%LJLINK% /DLL /out:%LJDLLNAME% lj_*.obj lib_*.obj FAExt.obj
@if errorlevel 1 goto :BAD

if exist %LJDLLNAME%.manifest^
  %LJMT% -manifest %LJDLLNAME%.manifest -outputresource:%LJDLLNAME%;2

@del FAExt.obj FAExt.manifest
@echo.
@echo === Successfully built FAExt for Windows/%LJARCH% ===

@goto :END
:BAD
@echo.
@echo *******************************************************
@echo *** Build FAILED -- Please check the error messages ***
@echo *******************************************************
@goto :END
:FAIL
@echo You must open a "Visual Studio Command Prompt" to run this script
:END
