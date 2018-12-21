@echo off
set cmake="F:\Program Files (x86)\CMake\bin\cmake.exe"


set "CurrPath=%~dpnx0" & call set "CurrPath=%%CurrPath:\%~nx0=%%" 
set ConfigFile=%CurrPath%\env-config.cmake


if not exist build mkdir build
cd build
%cmake%  -C %ConfigFile% ..
%cmake%  --build . --target ALL_BUILD --config Debug
%cmake%  --build . --target ALL_BUILD --config Release


cd ..
pause