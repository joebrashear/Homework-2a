// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

int debug = 0;

typedef struct LinkedList
{
    char *command;
    struct LinkedList *next;
} ListNode;

void deallocate_List(ListNode *head) 
{
    ListNode *temp = NULL;

    while (head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp->command);
        free(temp);
    }

    return;
}

int add_command_to_list(ListNode *head, char* new_command, size_t len)
{
    ListNode *new_list = NULL;
    ListNode *temp = NULL;
    
    if (head == NULL)
    {

        ListNode *new_list = (ListNode *)calloc(1, sizeof(ListNode));
        if (new_list == NULL)
        {
            print_error();
            return 1;
        }

        new_list->command = (char *)calloc(len, sizeof(char));
        if (new_list->command == NULL)
        {
            print_error():
            free(new_list);
            return 1;
        }

        strcpy(new_list->command, new_command);
        head = new_list;
    } 
    else
    {
        temp = head;
        while (head->next != NULL)
        {
            head = head->next;
        }

        head->next = (ListNode *)calloc(1, sizeof(ListNode));
        if (head->next == NULL)
        {
            print_error();
            deallocate_List(temp);
            return 1;
        }

        head->next->command = (char *)calloc(len, sizeof(char));
        if (head->next->command == NULL)
        {
            print_error():
            deallocate_List(temp);
            return 1;
        }

        strcpy(head->next->command, new_command);
        head = temp;
    }
    return 0;
}

// int execute_command(char *command, char *line, int index)
// {

// }

int run_wish(FILE *input, int is_interactive)
{
    char *line = NULL;
    size_t len = 0;
    int left = 0;
    int right = 0;
    int has_input = 0;
    char *arg = NULL;

    while(1)
    {
        if (is_interactive)
        {
            printf("wish> ");
        }

        has_input = getline(&line, &len, input);
        if (has_input == -1) 
        {
            break;
        }
        
        while ((arg = strsep(&line, " ")) != NULL)
        {
            switch(arg)
            {
                case "cd":
                    break;
                case "path":
                    break;
                case "exit":
                    exit(0);
                default:

            }
        }
    }
    return 0;
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
            break;
    }

    return 0;
}