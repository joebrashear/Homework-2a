// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int debug = 0;

typedef struct LinkedList
{
    char *path;
    struct LinkedList *next;
} ListNode;

int process_args(char **args, ListNode *paths)
{
    int len = sizeof(args);
    int i = 0;
    char *arg = args[0];

    switch(arg)
    {
        case "cd":
            break;
        case "path":
            break;
        case "exit":
            free_array(args);
            deallocate_List(paths);
            exit(0);
        default:
            temp = paths;
            while (paths != NULL && access(paths->path, X_OK) == -1)
            {
                paths = paths->next;
            }

            if (paths == NULL)
            {
                print_error();
                paths = temp;
                exit(1);
            }

            while (paths != NULL && execv(paths->path, args) == -1)
            {
                paths = paths->next;
                while (paths != NULL && access(paths->path, X_OK) == -1)
                {
                    paths = paths->next;
                }
            }
            if (paths == NULL)
            {
                print_error();
                paths = temp;
                exit(1);
            }

            paths = temp;
    }
    return 2;
}

void free_array(char **str_arr, int len)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        free(str_arr[i]);
    }
    free(str_arr);
}

char **get_args(char *arguments)
{
    char **args = NULL;
    char *arg = NULL;
    int size = 0;

    while ((arg = strsep(&line, " ")) != NULL)
    {
        size++;
        args = realloc(args, size);
        if (args == NULL)
        {
            print_error();
            free_array(args, size - 1);
            return NULL;
        }
        
        args[size-1] = (char *)calloc(strlen(arg) + 1, sizeof(char));
        if (args[size-1] == NULL)
        {
            print_error();
            free_array(args, size)
            return NULL;
        }
        strcpy(args[size-1], arg);
    }
    return args;
}

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

int add_path_to_list(ListNode *head, char* new_path, size_t len)
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

        new_list->path = (char *)calloc(len, sizeof(char));
        if (new_list->path == NULL)
        {
            print_error():
            free(new_list);
            return 1;
        }

        strcpy(new_list->path, new_path);
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

        head->next->path = (char *)calloc(len, sizeof(char));
        if (head->next->path == NULL)
        {
            print_error():
            deallocate_List(temp);
            return 1;
        }

        strcpy(head->next->path, new_path);
        head = temp;
    }
    return 0;
}

int run_wish(FILE *input, int is_interactive)
{
    char *line = NULL;
    size_t len = 0;
    int has_input = 0;
    char *arg = NULL;
    ListNode *paths = NULL;
    ListNode *temp = NULL;
    char **args = NULL;
    int ret = 0;

    ret = add_path_to_list(paths, "/bin", strlen("/bin") + 1);
    if (ret == 1)
    {
        exit(1);
    }

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

        args = get_args(line);
        if (args == NULL)
        {
            return 1;
        }

        if (process_args(args, paths) == 0)
        {
            exit(0);
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
            if (run_wish(input, is_interactive) == 1)
                exit(1);
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

                if(run_wish(input, is_interactive) == 1)
                {
                    exit(1);
                }
                fclose(input);
            }
            break;
    }

    return 0;
}