#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

int32_t main(int32_t argc, char **argv)
{
	FILE *fp = stdin;
	if (argc > 1 && (fp = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "%s: failed to open file '%s'\n", argv[0], argv[1]);
		return 1;
	}
	uint8_t cur_byte = 0;
	uint8_t cur_byte_bits_read = 0;
	int32_t c;
	while ((c = fgetc(fp)) != EOF)
	{
		if (c == ';') {
			while (c != EOF && c != '\n') c = fgetc(fp);
		}
		if (c != '0' && c != '1') continue;
		cur_byte = cur_byte*2 + c-'0';
		if (++cur_byte_bits_read == 8) {
			fputc(cur_byte, stdout);
			cur_byte = cur_byte_bits_read = 0;
		}
	}

	if (argc > 1) fclose(fp);

	return 0;
}
