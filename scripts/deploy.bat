mkdir ..\deploy
mkdir ..\deploy\assets
xcopy /Y ..\build\Release\*.exe ..\deploy
xcopy /Y ..\build\Release\*.dll ..\deploy
xcopy /Y /S ..\assets ..\deploy\assets
xcopy /Y ..\*.cfg ..\deploy
xcopy /Y ..\*.ini ..\deploy