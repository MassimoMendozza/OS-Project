master: master.c
	@echo -n 'Compiling Master...'
	@gcc master.c -std=c89 -pedantic -o master
	@echo -e '\tDONE'

build: master