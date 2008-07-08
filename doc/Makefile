# Steven Herbst
# herbst@mit.edu
#
# Builds all LaTeX documentation

LATEX=latex
BIBTEX=bibtex
DVIPDF=dvipdf

lec.pdf : lec.dvi
	$(DVIPDF) lec.dvi

lec.dvi : lec.tex lec.bib ../fig/*.eps
	make clean && $(LATEX) lec && $(BIBTEX) lec && $(LATEX) lec && $(LATEX) lec 

clean: 
	rm -f *.log *.aux *.dvi *.blg *.bbl

