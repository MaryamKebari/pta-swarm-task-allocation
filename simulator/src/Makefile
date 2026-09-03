CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDLIBS  = -lm

OFILES  = main.o sim.o params.o random.o output.o fxn.o ftarget.o ftracker.o gnu.o animate.o
HFILES  = types.h global.h extern.h

sim: $(OFILES) $(HFILES)
	$(CC) $(OFILES) -o sim $(LDLIBS)

xx: test.o random.o random.h
	$(CC) random.o test.o -o xx $(LDLIBS)

test.o: test.c
	$(CC) $(CFLAGS) -c test.c

main.o: main.c $(HFILES)
	$(CC) $(CFLAGS) -c main.c

sim.o: sim.c sim.h $(HFILES)
	$(CC) $(CFLAGS) -c sim.c

params.o: params.c params.h $(HFILES)
	$(CC) $(CFLAGS) -c params.c

random.o: random.c random.h $(HFILES)
	$(CC) $(CFLAGS) -c random.c

output.o: output.c output.h $(HFILES)
	$(CC) $(CFLAGS) -c output.c

fxn.o: fxn.c fxn.h $(HFILES)
	$(CC) $(CFLAGS) -c fxn.c

ftarget.o: ftarget.c ftarget.h $(HFILES)
	$(CC) $(CFLAGS) -c ftarget.c

ftracker.o: ftracker.c ftracker.h $(HFILES)
	$(CC) $(CFLAGS) -c ftracker.c

gnu.o: gnu.c gnu.h $(HFILES)
	$(CC) $(CFLAGS) -c gnu.c

animate.o: animate.c animate.h $(HFILES)
	$(CC) $(CFLAGS) -c animate.c

.PHONY: clean distclean
clean:
	rm -f $(OFILES) sim xx test.o *.o

distclean: clean
	rm -f sim xx
	rm -rf opfiles opfiles_ga
	rm -f run.num run.num.0
