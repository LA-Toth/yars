.PHONY:  all build edit log viewpdf silent make pdf emacs-x emacs e ex
NAME=$(notdir $(shell pwd))
all:	build
build:	$(NAME).tex
	pslatex ${NAME}.tex
	pdflatex ${NAME}.tex
edit:
	mcedit  ${NAME}.tex
emacs-x:
	emacs ${NAME}.tex &
emacs:
	@emacs -nw ${NAME}.tex
e: emacs
ex: emacs-x
log:
	@mcview ${NAME}.log
viewpdf:
	@acroread $(NAME).pdf
silent:	$(NAME).tex
	@pdflatex ${NAME}.tex >/dev/null 2>&1
dvi:
	@pslatex ${NAME}.tex
make:
	mcedit Makefile
pdf: silent viewpdf
