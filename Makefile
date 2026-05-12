CC = gcc
CFLAGS = -Wall -g

simpcsv: simpcsv.c
	$(CC) $(CFLAGS) $< -o $@

tags: simpcsv.c
	ctags -R .
