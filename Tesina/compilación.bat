set "MAIN=main"
set "OUTDIR=out"
set "DESTPDF=Tesina_Andrenacci-Carra.pdf"

rmdir -r "%OUTDIR%"
mkdir "%OUTDIR%"

pdflatex -interaction=nonstopmode -halt-on-error -file-line-error -output-directory="%OUTDIR%" "%MAIN%".tex
biber --input-directory "%OUTDIR%" --output-directory "%OUTDIR%" "%MAIN%"
pdflatex -interaction=nonstopmode -halt-on-error -file-line-error -output-directory="%OUTDIR%" "%MAIN%".tex
pdflatex -interaction=nonstopmode -halt-on-error -file-line-error -output-directory="%OUTDIR%" "%MAIN%".tex

move "%OUTDIR%"\"%MAIN%".pdf .\"%DESTPDF%"