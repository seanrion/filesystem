all:
	gcc -g myshell.c myfile.c disk.c  -o  myshell

clean:
	rm -rf main myshell