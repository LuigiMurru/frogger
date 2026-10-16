# Frogger: Resurrection

A text-based Frogger clone written in C with `ncurses`, built in two versions that implement the same game logic using two different concurrency models:

* `versione_processi/` — multi-process architecture: the crocodiles, the frog, the bullet manager and the grenade manager are separate processes (`fork`) that communicate through pipes.
* `versione_thread/` — multi-threaded architecture: the same components are POSIX threads that communicate through a shared producer/consumer buffer, mutexes and semaphores.

## How to play

* Use the arrow keys to move the frog; the goal is to reach the 5 dens at the top.
* Press the spacebar to throw grenades (max. 1 per second).
* Press `Q` to open the pause menu.
* Scoring: +50 for each den reached, -10 for each death, plus a bonus for the remaining time.

## Build and run

Requires `gcc` and the `ncurses` library (`libncurses-dev` on Debian/Ubuntu).

```
cd versione_processi   # or versione_thread
make
./frogger
```

## Credits

Project by Daniele Dessì and Luigi Murru for the Operating Systems and Network Programming course.
