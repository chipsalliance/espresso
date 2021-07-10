.PHONY: all clean

all: espresso.1 espresso.5

clean:
	rm -f espresso.1 espresso.5

espresso.1: espresso.adoc
	asciidoctor -b manpage -o $@ $<

espresso.5: pla.adoc
	asciidoctor -b manpage -o $@ $<

