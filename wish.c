// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

int run_wish(FILE *input, int is_interactive)
{
    
}

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char *argv[])
{
    FILE *input = NULL;
    int is_interactive = 0;
    int i = 0;

    switch(argc)
    {
        case 0:
            print_error();
            exit(1);
            break;
        case 1:
            input = stdin;
            is_interactive = 1;
            run_wish(input, is_interactive);
            break;
        default:
            for (i = 1; i < argc; i++)
            {
                input = fopen(argv[i], "r");
                if (input == NULL)
                {
                    print_error();
                    exit(1);
                }

                run_wish(input, is_interactive);
                fclose(input);
            }

    }
    return 0;
}