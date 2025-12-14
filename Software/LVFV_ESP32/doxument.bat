
set MAIN=main
set OUTDIR=out
set DESTPDF=Documentacion-ESP32_Andrenacci-Carra.pdf

doxygen doxy/Doxyfile
call "doxy/latex/make.bat"

move doxy\latex\refman.pdf .\%DESTPDF%

pause