@echo off
setlocal
rem The portable kit layout is root\source, root\Qt, root\VulkanSDK.
rem Environment variables are local to this process; no SDK installer is run.
for %%I in ("%~dp0..\..") do set "KIT_ROOT=%%~fI"
if not "%~1"=="" for %%I in ("%~1") do set "KIT_ROOT=%%~fI"
set "QTDIR=%KIT_ROOT%\Qt"
set "VULKAN_SDK=%KIT_ROOT%\VulkanSDK"
if not exist "%QTDIR%\bin\moc.exe" (
  echo Portable Qt SDK is not ready at "%QTDIR%".
  exit /b 1
)
if not exist "%VULKAN_SDK%\Include\vulkan\vulkan.h" (
  echo Portable Vulkan SDK headers are not ready at "%VULKAN_SDK%".
  exit /b 1
)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo The existing Visual Studio installation could not be located.
  exit /b 1
)
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VS_ROOT=%%I"
if not defined VS_ROOT (
  echo No existing Visual Studio MSBuild installation was found.
  exit /b 1
)
call "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
set "PATH=%QTDIR%\bin;%PATH%"
set "_CL_=/Z7"
python "%KIT_ROOT%\source\.ci\prepare-workspace-headers.py"
if errorlevel 1 exit /b 1
pushd "%KIT_ROOT%\source"
msbuild rpcs3.sln /m:1 /nr:false /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 /p:UseMultiToolTask=false /p:MultiProcessorCompilation=true /p:CL_MPCount=4 /fl /flp:"LogFile=workspace-build.log;Verbosity=normal" /p:CustomAfterMicrosoftCommonTargets="%KIT_ROOT%\source\.ci\workspace-build.targets" /v:minimal
set "BUILD_RESULT=%ERRORLEVEL%"
popd
exit /b %BUILD_RESULT%
