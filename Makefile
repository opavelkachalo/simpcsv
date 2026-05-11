CC = gcc
CFLAGS = -Wall -g

simpcsv: simpcsv.c
	$(CC) $(CFLAGS) $< -o $@

tags: %.c %.h
	ctags -R .
