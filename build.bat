@echo off
echo Building Game Tam Quoc (Native C with raylib)...

:: -I.\include: chỉ định thư mục chứa raylib.h
:: -L.\lib: chỉ định thư mục chứa thư viện liên kết (libraylib.a, raylib.dll)
:: -lraylib -lgdi32 -lwinmm: các thư viện cần thiết cho raylib trên Windows

gcc main.c game_data.c render.c update.c -o gametamquoc.exe -O2 -Wall -I.\include -L.\lib -lraylib -lgdi32 -lwinmm

if %ERRORLEVEL% equ 0 (
    echo.
    echo =========================================
    echo Build successful! 
    echo Ban co the chay file gametamquoc.exe 
    echo =========================================
    echo.
) else (
    echo.
    echo =========================================
    echo Build failed. Vui long kiem tra lai code hoac moi truong gcc.
    echo =========================================
    echo.
)
