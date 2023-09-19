mkdir ..\deploy
mkdir ..\deploy\assets
mkdir ..\deploy\data

xcopy /Y ..\..\..\build\Release\*.exe ..\deploy
xcopy /Y ..\..\..\build\Release\*.dll ..\deploy

xcopy /Y ..\build\Release\*.exe ..\deploy
xcopy /Y ..\build\Release\*.dll ..\deploy
xcopy /Y /S ..\..\..\assets ..\deploy\assets
xcopy /Y /S ..\data ..\deploy\data
xcopy /Y ..\..\..\*.cfg ..\deploy
xcopy /Y ..\*.cfg ..\deploy
xcopy /Y ..\*.ini ..\deploy
xcopy /Y ..\..\..\editor.cfg ..\deploy