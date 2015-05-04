SOURCES = conclusion.tex discussion.tex dsp.tex integration.tex main.tex neon.tex

all:	main.pdf

main.pdf:	$(SOURCES)
	pdflatex main.tex && pdflatex main.tex

clean:
	rm -f main.aux main.log
