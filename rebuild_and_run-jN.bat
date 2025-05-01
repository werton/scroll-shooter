CALL "%~dp0build.bat" clean-release -j%NUMBER_OF_PROCESSORS% 
CALL "%~dp0build.bat" release run -j%NUMBER_OF_PROCESSORS%


