REM El comando REM crea un comentario
REM El comando set crea 'defines'
REM Como ejecuto el bash desde el VSCode, en la carpeta del repo, es decir Tesina/.., primero cambio el directorio a ./Tesina, luego creo el PDF desde Latex

set MAIN=main
set OUTDIR=out
set DESTPDF=Poster_Andrenacci-Carra.pdf

cd .\Documentos\Poster

pdflatex -interaction=nonstopmode -halt-on-error -file-line-error -output-directory=%OUTDIR% %MAIN%.tex

move %OUTDIR%\%MAIN%.pdf .\%DESTPDF%