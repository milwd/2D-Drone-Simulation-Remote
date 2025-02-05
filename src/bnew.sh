gcc -o master2.out master2.c
gcc -o blb.out blackboard.c
gcc -o dyn.out dynamics.c -lm
gcc -o key.out keyboard.c -lncurses
gcc -o key.out keyboardvisu.c -lncurses
# gcc -o obs.out obstacle.c 
# gcc -o tar.out target.c 
gcc -o win.out windownewvisu.c -lncurses
g++ objectpublisher.cpp -o objectpub.out Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds
g++ objectsubscriber.cpp -o objectsub.out Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds