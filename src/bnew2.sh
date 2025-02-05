gcc -o master.out master.c
gcc -o Blackboard.out blackboard.c
gcc -o Dynamics.out dynamics.c -lm
gcc -o Keyboard.out keyboard.c -lncurses
gcc -o Window.out windownew.c -lncurses
gcc -o Obstacle.out obstacle.c 
gcc -o Target.out target.c 
g++ objectsubscriber.cpp -o ObjectSub.out Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds
g++ objectpublisher.cpp -o ObjectPub.out Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds
