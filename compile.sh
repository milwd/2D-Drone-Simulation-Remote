gcc -o master               src/master.c
gcc -o bins/Blackboard.out  src/blackboard.c
gcc -o bins/Dynamics.out    src/dynamics.c -lm
gcc -o bins/Keyboard.out    src/keyboard.c -lncurses
gcc -o bins/Window.out      src/windownew.c -lncurses
gcc -o bins/Obstacle.out    src/obstacle.c 
gcc -o bins/Target.out      src/target.c 
g++ -o bins/ObjectSub.out   src/objectsubscriber.cpp src/Generated/ObstaclesPubSubTypes.cxx src/Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds
g++ -o bins/ObjectPub.out   src/objectpublisher.cpp src/Generated/ObstaclesPubSubTypes.cxx src/Generated/ObstaclesTypeObjectSupport.cxx -I. -I/usr/local/include -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -std=c++11 -lstdc++ -lfastcdr -lfastdds
