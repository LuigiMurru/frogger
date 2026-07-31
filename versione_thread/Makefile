frogger: main.o collisioni.o gioco.o interfaccia.o threads.o proiettili.o
	gcc main.o collisioni.o gioco.o interfaccia.o threads.o proiettili.o -o frogger -lncurses

main.o: main.c frogger.h
	gcc -c main.c -lncurses

collisioni.o: collisioni.c frogger.h
	gcc -c collisioni.c -lncurses

gioco.o: gioco.c frogger.h
	gcc -c gioco.c -lncurses

interfaccia.o: interfaccia.c frogger.h
	gcc -c interfaccia.c -lncurses

threads.o: threads.c frogger.h
	gcc -c threads.c -lncurses

proiettili.o: proiettili.c frogger.h
	gcc -c proiettili.c -lncurses

clean:
	rm *.o