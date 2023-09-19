call cmake.bat
msbuild ..\build\TridotEditor.vcxproj /property:Configuration=Debug
msbuild ..\build\TridotGame.vcxproj /property:Configuration=Debug
msbuild ..\build\Template.vcxproj /property:Configuration=Debug