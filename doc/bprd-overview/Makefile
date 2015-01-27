bprd-overview: 
	pdflatex -draftmode $@
	pdflatex -draftmode $@
	bibtex   $@
	pdflatex -draftmode $@
	pdflatex $@

clean:
	rm -rf *.aux *.bbl *.blg *.lof *.log *.lot *.toc *.out
