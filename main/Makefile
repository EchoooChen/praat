# Makefile of the 'main' routines.
# Paul Boersma, 10 September 2011

include ../makefile.defs

CPPFLAGS = -I ../num -I ../sys -I ../fon

.PHONY: clean

main_*.o: ../sys/praat.h ../sys/praat_version.h ../sys/Graphics.h

clean:
	$(RM) main_*.o
	$(RM) praat_win.o

# The following is only for Windows.

praat_win.o: praat_win.rc
	$(WINDRES) -o praat_win.o praat_win.rc
