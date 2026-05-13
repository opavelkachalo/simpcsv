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

#define LONG_NUM_LEN 21

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
	int nrows, ncols, *visited;
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
	free(csv_table->visited);
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

int is_num(const char *str)
{
	const char *p;

	p = str;
	if(*p == '-')
		p = str + 1;
	for(; *p; p++)
		if(*p < '0' || *p > '9')
			return 0;
	return 1;
}

int read_row(struct arr_of_strs *row, struct table *csv_table)
{
	int row_size;
	char **tmp;

	if(!is_num(row->items[0]) || row->items[0][0] == '-')
		return -1;
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
	res = -1;
	for(i = 0; (line_str = read_line(csv_file)) != NULL; i++) {
		split_str(line_str, delim, &row);
		if(i == 0) { /* first line */
			res = create_header(&row, csv_table);
			free(row.items[0]);
		} else {
			if(row.size != csv_table->ncols + 1) {
				free_arr_of_strs(&row);
				free(line_str);
				return -1;
			}
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

int is_op(char c)
{
	return c == '+' || c == '-' || c == '*' || c == '/';
}

int find_op(const char *str)
{
	const char *p;

	for(p = str; *p; p++) {
		if(is_op(*p))
			return p - str;
	}
	return -1;
}

int split_expr(const char *expr, char **str1, char *op, char **str2)
{
	int op_pos;
	const char *p;

	op_pos = find_op(expr);
	/* if 0 or more than 1 operation in expr */
	if(op_pos == -1 || find_op(expr+op_pos+1) != -1)
		return -1;
	*op = expr[op_pos];
	p = expr;
	if(*p == '=') {
		p++;
		op_pos--;
	}
	*str1 = strdup(p);
	(*str1)[op_pos] = '\0';
	*str2 = strdup(p + op_pos + 1);
	return 0;
}

int calculate_expr(struct table *csv_table, const char *expr, int i, int j);

int find_arg(const char *str, long *arg, struct table *csv_table)
{
	int i, j;
	char *value;

	/* arg is either a number, an address or a gibberish */
	if(is_num(str)) {
		*arg = strtol(str, NULL, 10);
		return 0;
	}
	if(find_pos(str, csv_table, &i, &j) == -1)
		return -1;
	value = csv_table->values.items[i][j];
	/* address can point either to a number or to another expression */
	if(is_num(value)) {
		*arg = strtol(value, NULL, 10);
		return 0;
	}
	if(*value == '=' && !csv_table->visited[i*csv_table->ncols+j]) {
		csv_table->visited[i*csv_table->ncols+j] = 1;
		if(calculate_expr(csv_table, value, i, j) == 0) {
			value = csv_table->values.items[i][j];
			*arg = strtol(value, NULL, 10);
			return 0;
		}
	}
	return -1;
}

int calculate_expr(struct table *csv_table, const char *expr, int i, int j)
{
	char op, *str1, *str2;
	long arg1, arg2, ans;
	int res;

	res = 0;
	if(split_expr(expr, &str1, &op, &str2) == -1) {
		res = -1;
		goto cleanup;
	}
	if(find_arg(str1, &arg1, csv_table) == -1) {
		res = -1;
		goto cleanup;
	}
	if(find_arg(str2, &arg2, csv_table) == -1) {
		res = -1;
		goto cleanup;
	}
	switch(op) {
	case '+':
		ans = arg1 + arg2;
		break;
	case '-':
		ans = arg1 - arg2;
		break;
	case '*':
		ans = arg1 * arg2;
		break;
	case '/':
		if(arg2 == 0) {
			res = -1;
			goto cleanup;
		}
		ans = arg1 / arg2;
	}
	free(csv_table->values.items[i][j]);
	csv_table->values.items[i][j] = malloc(LONG_NUM_LEN *
				sizeof(*csv_table->values.items[i][j]));
	sprintf(csv_table->values.items[i][j], "%ld", ans);
	csv_table->visited[i*csv_table->ncols+j] = 0;
cleanup:
	free(str1);
	free(str2);
	return res;
}

int calculate_table(struct table *csv_table)
{
	int i, j, nrows, ncols, res;
	char *cell;

	res = 0;
	nrows = csv_table->nrows;
	ncols = csv_table->ncols;
	csv_table->visited = calloc(csv_table->nrows * csv_table->ncols,
			sizeof(*csv_table->visited));
	for(i = 0; i < nrows; i++) {
		for(j = 0; j < ncols; j++) {
			cell = csv_table->values.items[i][j];
			if(*cell != '=' || csv_table->visited[i*ncols + j])
				continue;
			csv_table->visited[i*ncols + j] = 1;
			if(calculate_expr(csv_table, cell, i, j) == -1)
				res = -1;
		}
	}
	return res;
}

void print_table(struct table *csv_table)
{
	int i, j;

	putchar(',');
	for(i = 0; i < csv_table->ncols; i++) {
		printf("%s%c", csv_table->header.items[i],
			i != csv_table->ncols - 1 ? ',' : '\n');
	}
	for(i = 0; i < csv_table->nrows; i++) {
		printf("%s,", csv_table->index.items[i]);
		for(j = 0; j < csv_table->ncols; j++) {
			printf("%s%c", csv_table->values.items[i][j],
				j != csv_table->ncols - 1 ? ',' : '\n');
		}
	}
}

void print_n_chars(char c, int n)
{
	int i;
	for(i = 0; i < n; i++)
		putchar(c);
}

struct dints {
	int size, capacity, *items;
};

void count_col_lengths(struct table *csv_table, struct dints *lengths)
{
	int max, curlen, i, j;

	max = 0;
	for(i = 0; i < csv_table->index.size; i++) {
		curlen = strlen(csv_table->index.items[i]);
		if(curlen > max)
			max = curlen;
	}
	DA_APPEND(lengths, max);
	for(i = 0; i < csv_table->ncols; i++) {
		max = strlen(csv_table->header.items[i]);
		for(j = 0; j < csv_table->nrows; j++) {
			curlen = strlen(csv_table->values.items[j][i]);
			if(curlen > max)
				max = curlen;
		}
		DA_APPEND(lengths, max);
	}
}

void print_horiz_line(struct dints *lengths)
{
	int i;

	putchar('+');
	for(i = 0; i < lengths->size; i++) {
		print_n_chars('-', lengths->items[i]);
		putchar('+');
	}
	putchar('\n');
}

void print_pretty_header(struct table *csv_table, struct dints *lengths)
{
	int i;

	print_horiz_line(lengths);
	putchar('|');
	print_n_chars(' ', lengths->items[0]);
	putchar('|');
	for(i = 1; i < lengths->size; i++)
		printf("%*s|", lengths->items[i],
				csv_table->header.items[i-1]);
	putchar('\n');
	print_horiz_line(lengths);
}

void print_pretty_table(struct table *csv_table)
{
	int i, j;
	struct dints lengths = {0};

	count_col_lengths(csv_table, &lengths);
	print_pretty_header(csv_table, &lengths);
	for(i = 0; i < csv_table->nrows; i++) {
		printf("|%*s|", lengths.items[0],
				csv_table->index.items[i]);
		for(j = 0; j < csv_table->ncols; j++) {
			printf("%*s%s", lengths.items[j+1],
				csv_table->values.items[i][j],
				j != csv_table->ncols - 1 ? "|" : "|\n");
		}
		print_horiz_line(&lengths);
	}
	free(lengths.items);
}

int main(int argc, char **argv)
{
	int res, pretty;
	char *csv_path;
	FILE *csv_file;
	struct table csv_table = {0};

	if(argc < 2) {
		fprintf(stderr, "Usage: %s <csv_path> [-p]\n", *argv);
		fprintf(stderr, "You didn't provide a csv path!\n");
		return 1;
	}
	pretty = argc == 3 && strcmp(argv[2], "-p") == 0;
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
	res = calculate_table(&csv_table);
	if(pretty)
		print_pretty_table(&csv_table);
	else
		print_table(&csv_table);
	free_table(&csv_table);
	fclose(csv_file);
	return res;
}
