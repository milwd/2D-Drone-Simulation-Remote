gcc -o master.out master.c
gcc -o blb.out blackboard.c
gcc -o dyn.out dynamics.c -lm
gcc -o key.out keyboard.c -lncurses
gcc -o obs.out obstacle.c 
gcc -o tar.out target.c 
gcc -o win.out window.c -lncurses

