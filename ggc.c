#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define GGC_VERSION "0.1.0-dev-preview"
#define LINE_SIZE 1024

typedef enum {
    MODE_NONE,
    MODE_HRCF,
    MODE_SFCF
} GGC_Mode;

static const char *instructions[] = {
    "LOAD",
    "MOV",
    "ADD",
    "SUB",
    "RETURN"
};

#define INSTRUCTION_COUNT \
    (sizeof(instructions) / sizeof(instructions[0]))

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char)end[-1]))
        end--;

    *end = '\0';
    return text;
}

static int is_instruction(const char *name)
{
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (strcmp(name, instructions[i]) == 0)
            return 1;
    }

    return 0;
}

static int compile_source(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror(filename);
        return 1;
    }

    char line[LINE_SIZE];
    unsigned int line_number = 0;
    unsigned int instruction_count = 0;

    GGC_Mode mode = MODE_NONE;
    int failed = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;

        if (!strchr(line, '\n') && !feof(file)) {
            fprintf(stderr,
                "GGC ERROR: Line %u is too long.\n",
                line_number);
            failed = 1;
            break;
        }

        char *content = trim(line);

        if (*content == '\0')
            continue;

        char *save = NULL;
        char *command = strtok_r(content, " \t\r\n", &save);

        if (!command)
            continue;

        if (strcmp(command, "MODE") == 0) {
            char *value = strtok_r(NULL, " \t\r\n", &save);
            char *extra = strtok_r(NULL, " \t\r\n", &save);

            if (mode != MODE_NONE) {
                fprintf(stderr,
                    "GGC ERROR: Duplicate MODE declaration at line %u.\n",
                    line_number);
                failed = 1;
                break;
            }

            if (instruction_count != 0) {
                fprintf(stderr,
                    "GGC ERROR: MODE must appear before instructions "
                    "(line %u).\n",
                    line_number);
                failed = 1;
                break;
            }

            if (!value || extra) {
                fprintf(stderr,
                    "GGC ERROR: Expected MODE HRCF or MODE SFCF "
                    "at line %u.\n",
                    line_number);
                failed = 1;
                break;
            }

            if (strcmp(value, "HRCF") == 0)
                mode = MODE_HRCF;
            else if (strcmp(value, "SFCF") == 0)
                mode = MODE_SFCF;
            else {
                fprintf(stderr,
                    "GGC ERROR: Unknown mode '%s' at line %u.\n",
                    value, line_number);
                failed = 1;
                break;
            }

            continue;
        }

        if (mode == MODE_NONE) {
            fprintf(stderr,
                "GGC ERROR: MODE declaration required before "
                "instructions (line %u).\n",
                line_number);
            failed = 1;
            break;
        }

        if (!is_instruction(command)) {
            fprintf(stderr,
                "GGC ERROR: Unknown instruction '%s' at line %u.\n",
                command, line_number);
            failed = 1;
            break;
        }

        instruction_count++;
    }

    if (ferror(file)) {
        perror("GGC: Error reading source");
        failed = 1;
    }

    fclose(file);

    if (failed)
        return 1;

    if (mode == MODE_NONE) {
        fprintf(stderr,
            "GGC ERROR: Source file has no MODE declaration.\n");
        return 1;
    }

    printf("GGC: Source analysis successful.\n");
    printf("GGC: File: %s\n", filename);
    printf("GGC: Mode: %s\n",
        mode == MODE_HRCF ? "HRCF" : "SFCF");
    printf("GGC: Recognized instructions: %u\n",
        instruction_count);

    if (mode == MODE_SFCF) {
        printf("GGC NOTE: SFCF-to-HRCF translation "
               "is not implemented yet.\n");
    }

    printf("GGC: No executable was generated yet.\n");

    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("GGC %s\n", GGC_VERSION);
        return 0;
    }

    if (argc != 2) {
        printf("Garry's GSC Compiler (GGC) %s\n", GGC_VERSION);
        printf("Usage: ggc <source.gsc>\n");
        printf("       ggc --version\n");
        return argc == 1 ? 0 : 1;
    }

    return compile_source(argv[1]);
}
