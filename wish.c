// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int debug = 1;
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

void free_string_array(char **array, int len)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        free(array[i]);
    }
    free(array);
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

int copy_to_args(char *arg)
{
    size++;
    if (args == NULL)
    {
        
        args = (char **)calloc(size, sizeof(char *));
    }
    args = (char **)realloc(args, size * sizeof(char *));
    if (args == NULL)
    {
        print_error();
        return 1;
    }
    args[size-1] = (char *)calloc(sizeof(arg), sizeof(char));
    if (args[size-1] == NULL)
    {
        print_error();
        free_string_array(args, size);
        return 1;
    }
    strcpy(args[size-1], arg);
    // cut off new line character, if any
    if (args[size-1][strlen(arg)-1] == '\n')
    {
        args[size-1][strlen(arg)-1] = '\0';
    }
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
        copy_to_args(arg);
    }

    if (debug)
    {
        for (int i = 0; i < size; i++)
        {
            if (args[i] == NULL)
            {
                printf("NULL");
            }
            else{
                printf("%s,", args[i]);
            }
        }
        printf("\n");
    }
    
    return 0;
}

int process_args(int left)
{
    ListNode *temp = NULL;
    int right = size;

    if (left == size)
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
    else if (fork() == 0)
    {
        right = left;
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

        // looking for output input (ls > output)
        while (right < size && strcmp(">", args[right]) != 0)
        {
            right++;
        }

        if (right != size)
        {
            if (size - right != 2)
            {
                print_error();
                return 1;
            }

            int redir = open(args[right+1], O_WRONLY | O_CREAT);
            if (redir == -1)
            {
                print_error();
                return 1;
            }

            if (dup2(redir, STDOUT_FILENO) == -1)
            {
                print_error();
                return 1;
            }

            if (dup2(redir, STDERR_FILENO) == -1)
            {
                print_error();
                return 1;
            }
            close(redir);
        }
        // Create path for executable
        char command[] = "";
        strcpy(command, paths->path);
        strcat(command, "/");
        strcat(command, args[left]); 
        
        // create copy of args 
        char **sub_args = NULL;
        right = left;

        while (right < size && strcmp(args[right], "&") != 0)
        {
            sub_args = (char **)realloc(sub_args, ((right - left) * sizeof(char *)) + 1);
            if (sub_args == NULL)
            {
                print_error();
                return 1;
            }
            sub_args[right] = (char *)calloc(sizeof(args[right]), sizeof(char));
            if (sub_args[right] == NULL)
            {
                // TODO: add way to free sub args array
                free_string_array(sub_args, right);
                print_error();
                return 1;
            }
            strcpy(sub_args[right], args[right]);
            right++;
        }

        if (right < size && strcmp(args[right], "&") == 0)
        {
            if (fork() == 0)
            {
                return process_args(right + 1);
            }
        }

        while (paths != NULL && execv(command, sub_args) == -1)
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

        free(command);
        paths = temp;
        return 2;
    }

    wait(NULL);
    return 2;
}

char *string_replace(char *orig, char *rep, char *with)
{
    char *result = NULL;
    char *temp = NULL;
    char *insert_pt = NULL;
    int len_rep = strlen(rep);
    int len_with = strlen(with);
    int len_orig = strlen(orig);
    int len_front = 0;

    temp = result = (char *)calloc(len_orig + (len_with - len_rep) + 1, sizeof(char));
    insert_pt = strstr(orig, rep);
    len_front = insert_pt - orig;
    // iterate to right where we need to replace
    temp = strncpy(temp, orig, len_front) + len_front;
    //replace
    temp = strcpy(temp, with) + len_with;
    // move orig up past the front and the replacement
    orig += len_front + len_rep;
    // copy the rest to temp
    strcpy(temp, orig);

    return result;
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
        
        if (strstr(line, ">") != NULL)
        {
            line = string_replace(line, ">", " > ");
        }

        if (strstr(line, "&") != NULL)
        {
            line = string_replace(line, "&", " & ");
        }

        get_args(line);
        if (args == NULL)
        {
            return 1;
        }

        fflush(stdout);
        
        if (process_args(0) == 0)
        {
            free_string_array(args, size);
            exit(0);
        }
        free_string_array(args, size);
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