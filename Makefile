all:
	gcc main.c -o lab5 -lpthread -lm
	gcc main.c -o lab5san -fsanitize=address -lpthread -lm
	cppcheck --enable=all --inconclusive --std=posix main.c
	/usr/src/linux-source-4.4.0/scripts/checkpatch.pl -f main.c
