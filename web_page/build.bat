cd main
..\htp.exe -quiet @main_build.rsp
cd ..
cd shots
..\htp.exe -quiet @shots_build.rsp
cd ..
rem cd ddf
rem ..\htp.exe -quiet @ddf_build.rsp
rem cd ..
rem cd rts
rem ..\htp.exe -quiet @rts_build.rsp
rem cd ..
copy css\*.* out\css
copy images\*.* out\images
copy shots\*.jpg out\shots
copy shots\*.png out\shots
