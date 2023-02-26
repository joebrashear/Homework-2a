// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int debug = 0;
int size = 0;
char **args = NULL;

typedef struct LinkedList
{
    char *path;
    struct LinkedList *next;
} ListNode;

ListNode *paths = NULL;


void deallocate_List() 
{
    ListNode *temp = NULL;

    while (paths != NULL)
    {
        temp = paths;
        paths = paths->next;
        free(temp->path);
        free(temp);
    }

    return;
}

void free_array()
{
    int i = 0;

    for (i = 0; i < size - 1; i++)
    {
        free(args[i]);
    }
    free(args);
}

void print_error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int add_path_to_list(char* new_path, size_t len)
{
    ListNode *temp = NULL;
    
    temp = paths;
    while (paths->next != NULL)
    {
        paths = paths->next;
    }

    paths->next = (ListNode *)calloc(1, sizeof(ListNode));
    if (paths->next == NULL)
    {
        print_error();
        deallocate_List(temp);
        return 1;
    }

    paths->next->path = (char *)calloc(len, sizeof(char));
    if (paths->next->path == NULL)
    {
        print_error();
        deallocate_List(temp);
        return 1;
    }

    strcpy(paths->next->path, new_path);

    paths = temp;

    return 0;
}

int get_args(char *arguments)
{
    args = NULL;
    char *arg = NULL;
    size = 0;

    while ((arg = strsep(&arguments, "\t ")) != NULL)
    {        
        if (*arg == '\0')
            continue;
        size++;
        if (args == NULL)
        {
            args = (char **)calloc(size, sizeof(char *));
        }
        else
        {
            args = (char **)realloc(args, size * sizeof(char *));
        }
        
        if (args == NULL)
        {
            print_error();
            free_array();
            return 1;
        }
        args[size-1] = (char *)calloc(sizeof(arg), sizeof(char));
        if (args[size-1] == NULL)
        {
            print_error();
            free_array();
            return 1;
        }
        strcpy(args[size-1], arg);
        // cut off new line character, if any
        if (args[size-1][strlen(arg)-1] == '\n')
        {
            args[size-1][strlen(arg)-1] = '\0';
        }
    }
    size++;
    args = realloc(args, size);
    if (args == NULL)
    {
        print_error();
        free_array();
        return 1;
    }

    if (debug)
    {
        for (int i = 0; i < size - 1; i++)
        {
            
            if (args[i] == NULL)
            {
                printf("NULL\n");
            }
            else
            {
                printf("%s,", args[i]);
            }
        }
        printf("%d\n", size);
    }
    
    return 0;
}

int process_args(int left)
{
    ListNode *temp = NULL;
    int right = size - 1;

    if (left == size - 1)
    {
        return 2;
    }
    
    if (strcmp("cd", args[left]) == 0)
    {
        if (right - left < 2 || right - left - 1 > 2)
        {
            print_error();
            exit(1);
        }

        if (chdir(args[right-1]) == -1)
        {
            if (debug)
                printf("here");
            print_error();
            return 1;
        }
    }
    else if (strcmp("path", args[left]) == 0)
    {
        deallocate_List(paths);
        paths = (ListNode *)calloc(1, sizeof(ListNode));
        
        ListNode *temp = paths;
        
        for (int j = left + 1; j < right; j++)
        {
            add_path_to_list(args[j], sizeof(args[j]));
            if (paths == NULL)
            {
                print_error();
                return 1;
            }
        }
        // free dummy head
        paths = temp;
        ListNode *temp2 = paths;
        paths = paths->next;
        free(temp2);
    }
    else if(strcmp("exit", args[left]) == 0)
    {
        if (right - left != 1)
        {
            print_error();
            return 1;
        }
        return 0;
    }
    else if (strcmp("&", args[left]))
    {
        ;
    }
    else if (fork() == 0)
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
            return 1;
        }

        if (strcmp(">", args[left]))
        {
            if (right - left > 2 || right - left < 2)
            {
                print_error();
                return 1;
            }
            
        }
        
        // Create path for executable
        char command[] = "";
        strcpy(command, paths->path);
        strcat(command, "/");

        strcat(command, args[left]); 

        while (paths != NULL && execv(command, args) == -1)
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
            return 1;
        }
        paths = temp;
        return 2;
    }

    wait(NULL);
    return 2;
}

int run_wish(FILE *input, int is_interactive)
{
    char *line = NULL;
    size_t len = 0;
    int has_input = 0;
    paths = (ListNode *)calloc(1, sizeof(ListNode));
    args = NULL;

    add_path_to_list("/bin", strlen("/bin") + 1);
    if (paths == NULL)
    {
        exit(1);
    }

    // ignore dummy head
    ListNode *temp = paths;
    paths = paths->next;
    free(temp);

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

        get_args(line);
        if (args == NULL)
        {
            return 1;
        }
        
        if (process_args(0) == 0)
        {
            free_array();
            exit(0);
        }
        free_array();
    }
    free(line);
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