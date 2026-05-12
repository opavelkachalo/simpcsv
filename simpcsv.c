#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DA_APPEND(da, elem) do {\
	if((da)->size == (da)->capacity) {\
		if((da)->capacity == 0)\
			(da)->capacity = 1;\
		else\
			(da)->capacity *= 2;\
		(da)->items = realloc((da)->items,\
				sizeof(*(da)->items) * (da)->capacity);\
	}\
	(da)->items[(da)->size] = (elem);\
	(da)->size++;\
} while(0)

#define DA_ZERO_INIT(da) do {\
	(da)->items = NULL;\
	(da)->size = 0;\
	(da)->capacity = 0;\
} while(0)

enum ops { add, sub, mult, divi };

struct arr_of_strs {
	int size, capacity;
	char **items;
};

void free_arr_of_strs(struct arr_of_strs *dstrs)
{
	int i;
	for(i = 0; i < dstrs->size; i++)
		free(dstrs->items[i]);
	free(dstrs->items);
}

struct table {
	int nrows, ncols;
	struct arr_of_strs index, header;
	struct {
		int size, capacity;
		char ***items;
	} values;
};

void free_table(struct table *csv_table)
{
	int i, j;

	free_arr_of_strs(&csv_table->header);
	free_arr_of_strs(&csv_table->index);
	for(i = 0; i < csv_table->nrows; i++) {
		for(j = 0; j < csv_table->ncols; j++)
			free(csv_table->values.items[i][j]);
		free(csv_table->values.items[i]);
	}
	free(csv_table->values.items);
}

struct dstr {
	int size, capacity;
	char *items;
};

void split_str(const char *str, char delim, struct arr_of_strs *dstrs)
{
	const char *p;
	struct dstr word;

	DA_ZERO_INIT(&word);
	for(p = str; *p; p++) {
		if(*p == delim) {
			DA_APPEND(&word, '\0');
			DA_APPEND(dstrs, word.items);
			DA_ZERO_INIT(&word);
		} else {
			DA_APPEND(&word, *p);
		}
	}
	DA_APPEND(&word, '\0');
	DA_APPEND(dstrs, word.items);
}

char *read_line(FILE *file)
{
	int c;
	struct dstr line;

	DA_ZERO_INIT(&line);
	while((c = fgetc(file)) != EOF) {
		if(c == '\n')
			break;
		DA_APPEND(&line, c);
	}
	if(line.size != 0)
		DA_APPEND(&line, '\0');
	return line.items;
}

int create_header(struct arr_of_strs *row, struct table *csv_table)
{
	int header_size;

	csv_table->ncols = row->size - 1;
	if(csv_table->ncols == 0)
		return -1;
	csv_table->header.size = csv_table->ncols;
	csv_table->header.capacity = csv_table->header.size;
	header_size = sizeof(*csv_table->header.items) *
			csv_table->header.size;
	csv_table->header.items = malloc(header_size);
	memcpy(csv_table->header.items, row->items+1, header_size);
	return 0;
}

int read_row(struct arr_of_strs *row, struct table *csv_table)
{
	int row_size;
	char **tmp;

	csv_table->nrows++;
	DA_APPEND(&csv_table->index, row->items[0]);
	row_size = csv_table->ncols * sizeof(**csv_table->values.items);
	tmp = malloc(row_size);
	memcpy(tmp, row->items+1, row_size);
	DA_APPEND(&csv_table->values, tmp);
	return 0;
}

int read_csv(FILE *csv_file, struct table *csv_table, char delim)
{
	int i, res;
	char *line_str;
	struct arr_of_strs row;

	DA_ZERO_INIT(&row);
	for(i = 0; (line_str = read_line(csv_file)) != NULL; i++) {
		split_str(line_str, delim, &row);
		if(i == 0) { /* first line */
			res = create_header(&row, csv_table);
			free(row.items[0]);
		} else {
			res = read_row(&row, csv_table);
		}
		free(line_str);
		free(row.items);
		if(res == -1)
			break;
		DA_ZERO_INIT(&row);
	}
	return res;
}

#define TMP_BUF_SIZE 1028
char tmp_buf[TMP_BUF_SIZE];

void print_entire_file(FILE *file)
{
	if(feof(file))
		clearerr(file);
	fseek(file, 0, SEEK_SET);
	while(fgets(tmp_buf, TMP_BUF_SIZE, file) != NULL)
		fputs(tmp_buf, stdout);
}

int is_num(const char *str)
{
	const char *p;

	for(p = str; *p; p++)
		if(*p < '0' || *p > '9')
			return 0;
	return 1;
}

int find_num(const char *str)
{
	const char *p;

	for(p = str; *p; p++) {
		if('0' <= *p && *p <= '9')
			return p - str;
	}
	return -1;
}

int split_idx(const char *idx, char **col_i, char **row_i)
{
	int pos, size;

	pos = find_num(idx);
	if(pos == -1)
		return -1;
	*col_i = strdup(idx);
	(*col_i)[pos] = '\0';
	size = strlen(idx + pos) + 1;
	*row_i = malloc(size);
	memcpy(*row_i, idx + pos, size);
	if(!is_num(*row_i)) {
		return -1;
	}
	return 0;
}

int find_pos(const char *idx, struct table *csv_table, int *i, int *j)
{
	int res, x, y;
	char *col_i, *row_i;

	col_i = NULL;
	row_i = NULL;
	res = split_idx(idx, &col_i, &row_i);
	if(res == -1)
		goto cleanup;
	res = 0;
	for(x = 0; x < csv_table->nrows; x++) {
		if(strcmp(csv_table->index.items[x], row_i) == 0) {
			*i = x;
			break;
		}
	}
	for(y = 0; y < csv_table->ncols; y++) {
		if(strcmp(csv_table->header.items[y], col_i) == 0) {
			*j = y;
			break;
		}
	}
	if(y == csv_table->ncols || x == csv_table->nrows)
		res = -1;
cleanup:
	free(col_i);
	free(row_i);
	return res;
}

void calculate_expr(struct table *csv_table, const char *expr)
{
}

void calculate_table(struct table *csv_table)
{
	int i, j;
	char *cell;

	for(i = 0; i < csv_table->nrows; i++) {
		for(j = 0; j < csv_table->nrows; j++) {
			cell = csv_table->values.items[i][j];
			if(*cell == '=')
				calculate_expr(csv_table, cell);
		}
	}
}

int main(int argc, char **argv)
{
	int res;
	char *csv_path;
	FILE *csv_file;
	struct table csv_table = {0};

	if(argc < 2) {
		fprintf(stderr, "Usage: %s <csv_path>\n", *argv);
		fprintf(stderr, "You didn't provide a csv path!\n");
		return 1;
	}
	csv_path = argv[1];
	csv_file = fopen(csv_path, "r");
	if(!csv_file) {
		perror(csv_path);
		return 1;
	}
	res = read_csv(csv_file, &csv_table, ',');
	if(res == -1) {
		print_entire_file(csv_file);
		fclose(csv_file);
		return 1;
	}
	free_table(&csv_table);
	fclose(csv_file);
	return 0;
}
