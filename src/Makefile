# vim:set ts=8 sts=8 sw=8 tw=0:
#
# Last Change: 09-Jun-2003.
# Written By:  Muraoka Taro <koron@tka.att.ne.jp>

TAGS_TARGET = *.c *.h

tags:
	ctags $(TAGS_TARGET)

.PHONY: tags clean

clean:
	rm -f *.o
	rm -f *.obj
	rm -f *.exe
	rm -f tags
