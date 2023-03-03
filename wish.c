// Code by William Brashear 
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

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

void print_paths() 
{
    ListNode *curr  = paths;

    printf("Print paths: ");
    while (curr != NULL)
    {
        printf("%s, ", curr->path);
        curr = curr->next;
    }
    printf("\n");

    return;
}

void free_string_array(char **array, int len)
{
    if (array == NULL)
    {
        return;
    }
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
    args[size-1] = (char *)calloc(strlen(arg) + 1, sizeof(char));
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

    while ((arg = strsep(&arguments, "\n\t ")) != NULL)
    {        
        if (strcmp(arg, "") == 0)
            continue;
        if (debug)
            printf("%s arg\n", arg);
        copy_to_args(arg);
    }

    if (debug)
    {
        for (int i = 0; i < size; i++)
        {
        
            printf("%s,", args[i]);
        }
        printf("\n");
        printf("%d\n", size);
    }
    
    return 0;
}

int process_args(int left)
{
    ListNode *temp = NULL;
    ListNode *temp2 = NULL;
    char *command = NULL;
    int right = size;
    char **sub_args = NULL;
    pid_t wpid;
    int status = 0;
    int sub_args_size = 0;
    int redir = STDOUT_FILENO;
    FILE *temp_file = NULL;

    if (left >= size)
    {
        return 2;
    }
    
    if (strcmp("cd", args[left]) == 0)
    {
        if (right - left < 2 || right - left - 1 > 2)
        {
            print_error();
            return 1;
        }

        if (chdir(args[right-1]) == -1)
        {
            if (debug)
                printf("invalid cd path\n");
            print_error();
            return 1;
        }
    }
    else if (strcmp("path", args[left]) == 0)
    {
        deallocate_List(paths);
        paths = (ListNode *)calloc(1, sizeof(ListNode));
        temp = paths;
        
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
        temp2 = paths;
        paths = paths->next;
        free(temp2);
        if (debug)
            print_paths();
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
    else if (strcmp("&", args[left]) == 0)
    {
        process_args(left + 1);
    }
    else if (strcmp(">", args[left]) == 0)
    {
        print_error();
        return 1;
    }
    else
    {
        right = left;
        temp = paths;
        while (paths != NULL && access(paths->path, X_OK) == -1)
        {
            paths = paths->next;
        }

        if (paths == NULL)
        {
            if (debug)
            {
                printf("Invalid path access\n");
            }
            print_error();
            paths = temp;
            return 1;
        }

        // Create path for executable
        command = (char *)calloc(strlen(paths->path) +
                        + strlen("/") + strlen(args[left]) + 1, sizeof(char));
        strcpy(command, paths->path);
        strcat(command, "/");
        strcat(command, args[left]); 
        
        // create copy of args 
        sub_args = NULL;
        sub_args_size = 0;

        while (right < size && strcmp(args[right], "&") != 0 && strcmp(args[right], ">") != 0)
        {
            sub_args_size = right - left + 2;
            sub_args = (char **)realloc(sub_args, ((sub_args_size) * sizeof(char *)));
            if (sub_args == NULL)
            {
                if (debug)
                {
                    printf("Malloc failed sub args\n");
                }
                command == NULL ? NULL : free(command);
                print_error();
                return 1;
            }
            sub_args[right] = (char *)calloc(strlen(args[right]) + 1, sizeof(char));
            if (sub_args[right] == NULL)
            {
                command == NULL ? NULL : free(command);
                sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                print_error();
                return 1;
            }
            strcpy(sub_args[right], args[right]);
            right++;
        }

        if (debug)
        {
            printf("Sub args\n");
            for (int i = 0; i < sub_args_size; i++)
            {
                if (sub_args[i] != NULL)
                {
                    printf("%s,", sub_args[i]);
                }
                else
                    printf("NULL");
            }
            printf("\n");
        }

        if (fork() == 0)
        {   
            if (right < size && strcmp(args[right], ">") == 0)
            {
                if (right + 1 >= size || (right + 2 < size && strcmp(args[right+2], "&") != 0))
                {
                    print_error();
                    if (debug)
                        printf("error: invalid next file\n");
                    command == NULL ? NULL : free(command);
                    sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                    return 1;
                }

                temp_file = fopen(args[right+1], "w");
                if (temp_file != NULL)
                {
                    fclose(temp_file); // empty the file
                }

                redir = open(args[right+1], O_RDWR |O_CREAT,S_IRWXU|S_IRWXG|S_IRWXO, 0777);
                if (redir == -1)
                {
                    if (debug)
                        printf("redir file failed to open\n");
                    print_error();
                    command == NULL ? NULL : free(command);
                    sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                    return 1;
                }

                if (dup2(redir, STDOUT_FILENO) == -1)
                {
                    if (debug)
                        printf("dup2 failed to redirect stdout\n");
                    print_error();
                    close(redir);
                    command == NULL ? NULL : free(command);
                    sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                    return 1;
                }

                if (dup2(redir, STDERR_FILENO) == -1)
                {
                    if (debug)
                        printf("dup2 failed to redirect stderr\n");
                    print_error();
                    close(redir);
                    command == NULL ? NULL : free(command);
                    sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                    return 1;
                }
                close(redir);
            }
            temp = paths;
            int exec_ret = execv(command, sub_args);
            
            while (paths != NULL && exec_ret == -1)
            {
                
                paths = paths->next;
                while (paths != NULL && access(paths->path, X_OK) == -1)
                {
                    paths = paths->next;
                }
                // Create path for executable
                if (paths != NULL)
                {
                    free(command);
                    command = (char *)calloc(strlen(paths->path) +
                        + strlen("/") + strlen(args[left]) + 1, sizeof(char));
                    strcpy(command, paths->path);
                    strcat(command, "/");
                    strcat(command, args[left]); 
                }
                exec_ret = execv(command, sub_args);
            }
            
            if (paths == NULL)
            {
                if (debug)
                {
                    printf("No valid path\n");
                }
                print_error();
                command == NULL ? NULL : free(command);
                sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
                paths = temp;
                return 1;
            }

            command == NULL ? NULL : free(command);
            sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
            paths = temp;
            return 2;
        }
        paths = temp;
        if (right < size && strcmp(args[right], ">") == 0)
        {
            while (right < size && strcmp(args[right], "&") != 0)
            {
                right++;
            }
        }

        if (right < size && strcmp(args[right], "&") == 0)
        {
            process_args(right + 1);
        }
        command == NULL ? NULL : free(command);
        sub_args == NULL ? NULL : free_string_array(sub_args, sub_args_size + 2);
    }
    
    while ((wpid = wait(&status)) > 0);
    return 2;
}

char *string_replace(char *orig, char *rep, char *with)
{
    char *result = NULL;
    int i = 0;
    int count = 0;
    int orig_len = strlen(orig);
    int rep_len = strlen(rep);
    int with_len = strlen(with);

    for (int i = 0; i < orig_len; i++)
    {
        if (strstr(&orig[i], rep) == &orig[i])
        {
            count++;
            i += rep_len - 1;
        }
    }

    result = (char *)calloc(orig_len + count * (with_len - rep_len) + 1, sizeof(char));

    while (*orig)
    {
        if (strstr(orig, rep) == orig)
        {
            strcpy(&result[i], with);
            i += with_len;
            orig += rep_len;
        }
        else
        {
            result[i++] = *orig++;
        }
    }

    result[i] = '\0';
    return result;
}

int run_wish(FILE *input, int is_interactive)
{
    char *line = NULL;
    size_t len = 0;
    int has_input = 0;
    ListNode *temp = NULL;
    char *temp2 = NULL;
    paths = (ListNode *)calloc(1, sizeof(ListNode));
    args = NULL;

    add_path_to_list("/bin", strlen("/bin") + 1);
    if (paths == NULL)
    {
        print_error();
        exit(1);
    }

    // ignore dummy head
    temp = paths;
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
            temp2 = line;
            line = string_replace(line, ">", " > ");
            free(temp2);
        }

        if (strstr(line, "&") != NULL)
        {
            temp2 = line;
            line = string_replace(line, "&", " & ");
            free(temp2);
        }

        get_args(line);
        if (args == NULL)
        {
            continue;
        }

        if (process_args(0) == 0)
        {
            free_string_array(args, size);
            break;
        }
        free_string_array(args, size);
    }
    deallocate_List(paths);
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