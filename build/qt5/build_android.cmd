@echo off

set NYA_PRO=%cd%\nya_engine.pro
rmdir ..\..\bin\build-nya_engine-Android-armeabi-v7a-auto /s /q
md ..\..\bin\build-nya_engine-Android-armeabi-v7a-auto
pushd ..\..\bin\build-nya_engine-Android-armeabi-v7a-auto
call %QMAKE_ANDROID% -spec android-g++ %NYA_PRO%
if errorlevel 1 (
    echo -----------
    echo ----------- nya-engine build failed
    echo -----------
    popd
    exit /b 1
)
call %PLATFORM_MAKE% --jobs
if errorlevel 1 (
    echo -----------
    echo ----------- nya-engine build failed
    echo -----------
    popd
    exit /b 1
) else (
    echo -----------
    echo ----------- nya-engine built
    echo -----------
    popd
)
