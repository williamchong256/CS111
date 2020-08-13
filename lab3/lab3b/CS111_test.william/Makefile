UID=205114665

default:build

build:
	ln lab3b_helper lab3b
	chmod u+x lab3b

dist:
	tar -cvzf lab3b-$(UID).tar.gz Makefile README lab3b.py lab3b_helper

clean:
	rm -f lab3b lab3b-$(UID).tar.gz
