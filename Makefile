VERSION=0.3

DEBUG=-ggdb3 #-D_DEBUG #-fprofile-arcs -ftest-coverage # -pg -g
CXXFLAGS+=-O3 -Wall -DVERSION=\"${VERSION}\" $(DEBUG)
LDFLAGS+=$(DEBUG) -lSDL -lSDL_gfx -lSDL_image -lsndfile

OBJS=keyboard.o moppie.o error.o

all: moppie

moppie: $(OBJS)
	$(CXX) -Wall -W $(OBJS) $(LDFLAGS) -o moppie

install: moppie
	cp moppie /usr/local/bin

clean:
	rm -f $(OBJS) moppie

package: clean
	mkdir moppie-$(VERSION)
	cp *.cpp *.h Makefile readme.txt *.wav *.png moppie-$(VERSION)
	tar czf moppie-$(VERSION).tgz moppie-$(VERSION)
	rm -rf moppie-$(VERSION)

check:
	cppcheck -v --enable=all --std=c++11 --inconclusive -I. . 2> err.txt
