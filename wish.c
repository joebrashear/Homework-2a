// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int debug = 1;

typedef struct LinkedList
{
    char *path;
    struct LinkedList *next;
} ListNode;

void deallocate_List(ListNode *head) 
{
    ListNode *temp = NULL;

    while (head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp->path);
        free(temp);
    }

    return;
}

void free_array(char **str_arr)
{
    int i = 0;
    int len = sizeof(str_arr);

    for (i = 0; i < len; i++)
    {
        free(str_arr[i]);
    }
    free(str_arr);
}

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

ListNode *add_path_to_list(ListNode *head, char* new_path, size_t len)
{
    ListNode *temp = NULL;
    
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
        return NULL;
    }

    head->next->path = (char *)calloc(len, sizeof(char));
    if (head->next->path == NULL)
    {
        print_error();
        deallocate_List(temp);
        return NULL;
    }

    strcpy(head->next->path, new_path);

    return temp;
}

char **get_args(char *arguments)
{
    char **args = NULL;
    char *arg = NULL;
    int size = 0;

    while ((arg = strsep(&arguments, " ")) != NULL)
    {
        size++;
        args = realloc(args, size);
        if (args == NULL)
        {
            print_error();
            free_array(args);
            return NULL;
        }
        
        args[size-1] = (char *)calloc(strlen(arg) + 1, sizeof(char));
        if (args[size-1] == NULL)
        {
            print_error();
            free_array(args);
            return NULL;
        }
        strcpy(args[size-1], arg);
    }
    size++;
    args = realloc(args, size);
    if (args == NULL)
    {
        print_error();
        free_array(args);
        return NULL;
    }

    if (debug)
    {
        for (int i = 0; i < size; i++)
        {
            
            if (args[i] == NULL)
            {
                printf("NULL\n");
            }
            else{
                printf("%s,", args[i]);
            }
        }
        printf("%d\n", size);
    }
    
    return args;
}

int process_args(char **args, ListNode *paths)
{
    int len = sizeof(args);
    int i = 0;
    char *arg = NULL;
    ListNode *temp = NULL;

    while (i < len)
    {
        arg = args[i];
        if (strcmp("cd", arg) == 0)
        {
            continue;
        }
        else if (strcmp("path", arg) == 0)
        {
            continue;
        }
        else if(strcmp("exit", arg) == 0)
        {
            free_array(args);
            deallocate_List(paths);
            exit(0);
        }
        else
        {
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
            return 2;
        }
    }
    return 2;
}

int run_wish(FILE *input, int is_interactive)
{
    char *line = NULL;
    size_t len = 0;
    int has_input = 0;
    ListNode *paths = (ListNode *)calloc(1, sizeof(ListNode));
    char **args = NULL;

    paths = add_path_to_list(paths, "/bin", strlen("/bin") + 1);
    if (paths == NULL)
    {
        exit(1);
    }

    // ignore dummy head
    paths = paths->next;

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
                    fclose(input);
                    exit(1);
                }
                fclose(input);
            }
            break;
    }

    return 0;
}