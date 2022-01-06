@echo off
call shell.bat

set opt= -O2 -Oi -GL -GF -fp:fast
set def= -DDEBUG -DINTERNAL
echo BUILDING %exe_name% IN RELEASE MODE

set warn= -W2 -wd4505 -wd4305 -wd4244 -wd4201 -wd4100 -wd4189
set build_options= -DWIN32=1 %def% %warn%
set compile_flags= -nologo %opt% -EHa- -Zi -FC -I ../code/
set link_flags= winmm.lib -DEBUG -opt:ref -incremental:no
REM set link_flags= gdi32.lib user32.lib winmm.lib Comdlg32.lib -DEBUG -opt:ref -incremental:no dxguid.lib d3d11.lib dwmapi.lib

set /A comp_fail = 0
if not exist build mkdir build
pushd build
cl %build_options% %compile_flags% ../code/main.cpp /link %link_flags% /out:ray.exe || set /A comp_fail = 1
popd
if %comp_fail% EQU 1 (CALL)
