all:
	gcc -o file `pkg-config --libs plplotd` `pkg-config --libs sndfile` \
		`pkg-config --libs fftw3`  file.c
	gcc -o compare `pkg-config --libs plplotd` `pkg-config --libs sndfile` \
		`pkg-config --libs fftw3` compare.c
