call cmake.bat
msbuild ..\build\TridotEditor.vcxproj /property:Configuration=Release
msbuild ..\build\TridotGame.vcxproj /property:Configuration=Release
msbuild ..\build\Template.vcxproj /property:Configuration=Release