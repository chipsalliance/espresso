all:
	-mkdir -p bin man/man{1,5}
	-mkdir lib include doc
	(cd utility; make ../include/utility.h CAD=..)
	for i in port uprintf errtrap st mm utility; do \
		(cd $$i; make install CAD=..); done
	(cd espresso; make install CAD=..)

clean:
	-rm bin/* man/*
	for i in port uprintf errtrap st mm utility espresso; do \
		(cd $$i; make clean CAD=..); done
