$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
cmd.exe /c "`"$vcvars`" && cl test.cpp /EHsc /nologo && .\test.exe"
