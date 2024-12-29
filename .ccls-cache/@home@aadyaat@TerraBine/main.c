#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>

//Input line values
typedef struct {
  char cwd[PATH_MAX];
  char * username;
  int line;
  char shell_scripts_path[PATH_MAX];
} InputLine;

// Function to safely concatenate paths
size_t safe_path_join(char *dest, size_t dest_size, const char *base, const char *append) {
    size_t base_len = strlen(base);
    size_t append_len = strlen(append);
    
    // Check if we have enough space for base + '/' + append + '\0'
    if (base_len + append_len + 2 > dest_size) {
        return 0;  // Buffer would overflow
    }
    
    // Copy base path
    strcpy(dest, base);
    
    // Add separator if needed
    if (base[base_len - 1] != '/') {
        dest[base_len] = '/';
        base_len++;
    }
    
    // Copy append part
    strcpy(dest + base_len, append);
    return strlen(dest);
}

size_t safe_string_join(char *dest, size_t dest_size, const char *base, const char *append, const char *delimiter) {
    size_t base_len = strlen(base);
    size_t append_len = strlen(append);
    size_t delimiter_len = strlen(delimiter);

    // Check if we have enough space for base + delimiter + append + '\0'
    if (base_len + delimiter_len + append_len + 1 > dest_size) {
        return 0;  // Buffer would overflow
    }

    // Copy base string
    strcpy(dest, base);

    // Add delimiter if needed
    if (delimiter_len > 0) {
        strcat(dest, delimiter);
    }

    // Append the second string
    strcat(dest, append);

    return strlen(dest);  // Return the final length of the joined string
}

// Modified initialization function
void init_shell_scripts_path(InputLine *input) {
    if (getcwd(input->cwd, sizeof(input->cwd)) == NULL) {
        perror("getcwd() error");
        return;
    }
    
    if (!safe_path_join(input->shell_scripts_path, sizeof(input->shell_scripts_path), 
                        input->cwd, "shell_cmds")) {
        fprintf(stderr, "Failed to construct shell_scripts_path\n");
        return;
    }
}

// Modified command path function
void get_builtin_cmd_path(const char *cmd, char *full_path, const char *scripts_dir) {
    char script_name[PATH_MAX];
    
    if (strcmp(cmd, "cd") == 0) {
        strncpy(script_name, "cd.sh", PATH_MAX - 1);
    } else {
        size_t cmd_len = strlen(cmd);
        if (cmd_len + 3 >= PATH_MAX) {  // +3 for ".sh" and null terminator
            fprintf(stderr, "Command name too long\n");
            return;
        }
        strcpy(script_name, cmd);
        strcat(script_name, ".sh");
    }
    
    if (!safe_path_join(full_path, PATH_MAX, scripts_dir, script_name)) {
        fprintf(stderr, "Failed to construct command path\n");
        return;
    }
}

//Macros
#define check_end(msg) (strcmp(msg, "end") == 0)
#define check_clear(msg) (strcmp(msg, "cls") == 0)
#define CMD_ARG_BUFSIZE 64;
#define CMD_BUFSIZE 30; //not used yet
#define display(msg) mvprintw(input->line++,1,"%s",msg);
#define TOK_DELIM " "

//functions

//input line function
//retuens the line inputed by user
char *write_command(InputLine *input) {
  int bufsize = CMD_BUFSIZE; 
  char *line = (char *)malloc(bufsize*sizeof(char)); //figure out dynamic alloction
  if (line == NULL) {
    return NULL;
  };

  mvprintw(input->line++,1,"%s",input->username);
  getstr(line);
  
  return line;
}

//split line into arguments function
char **split_line(char *cmd,InputLine * input) {
  int i = 0;
  int bufsize = CMD_ARG_BUFSIZE;
  char **tokens = (char **)malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    display("ERROR: alloction error in split_line: tokens");
    exit(EXIT_FAILURE);
  }
  
  token = strtok(cmd,TOK_DELIM);
  while (token != NULL ) {
    //handling comments
    if (token[0] == '#')
      break;

    tokens[i++] = token;

    if (i >= bufsize) {
      bufsize++;
      tokens = realloc(tokens, bufsize * sizeof(char *));

      if (!tokens) {
        display("ERROR: alloction error in split_line: tokens");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, TOK_DELIM);
  }
  tokens[i] = NULL;
  return (tokens);
}

//command argument execution
void execute_args(char **cmd_args, InputLine *input) {
    const char *builtin_cmds[] = {"ls", "cd", "mv", "pwd", "delete", "cat", "gcc", "touch"};
    char exec_path[PATH_MAX];
    char **exec_args = NULL;
    int is_builtin = 0;
    const char *original_cmd = NULL;
    
    // Initialize pipe and path
    int Pipe_PtoC[2];
    if (pipe(Pipe_PtoC) == -1) {
        perror("pipe failed");
        return;
    }

    // Determine command type and set up execution path
    if (strncmp(cmd_args[0], "./", 2) == 0) {
        // Handle executable file
        if (!realpath(cmd_args[0] + 2, exec_path)) {
            mvprintw(input->line++, 1, "Failed to resolve path: %s", cmd_args[0] + 2);
            close(Pipe_PtoC[0]);
            close(Pipe_PtoC[1]);
            return;
        }
        
        if (access(exec_path, X_OK) == -1) {
            if (errno == ENOENT) {
                mvprintw(input->line++, 1, "No such file: %s", cmd_args[0]);
            } else {
                mvprintw(input->line++, 1, "Permission denied: %s", cmd_args[0]);
            }
            close(Pipe_PtoC[0]);
            close(Pipe_PtoC[1]);
            return;
        }
        exec_args = cmd_args;
        exec_args[0] = exec_path;
    } else {
        // Handle built-in commands
        for (size_t i = 0; i < sizeof(builtin_cmds) / sizeof(builtin_cmds[0]); i++) {
            if (strcmp(cmd_args[0], builtin_cmds[i]) == 0) {
                is_builtin = 1;
                original_cmd = builtin_cmds[i];
                
                char abs_scripts_path[PATH_MAX];
                if (!realpath(input->shell_scripts_path, abs_scripts_path)) {
                    fprintf(stderr, "Failed to resolve shell scripts path\n");
                    close(Pipe_PtoC[0]);
                    close(Pipe_PtoC[1]);
                    return;
                }
                
                // Construct script path
                char script_name[PATH_MAX];
                snprintf(script_name, sizeof(script_name), "%s.sh", cmd_args[0]);
                if (!safe_path_join(exec_path, sizeof(exec_path), abs_scripts_path, script_name)) {
                    fprintf(stderr, "Failed to construct script path\n");
                    close(Pipe_PtoC[0]);
                    close(Pipe_PtoC[1]);
                    return;
                }
                
                // Set execute permissions
                struct stat st;
                if (stat(exec_path, &st) == 0) {
                    mode_t new_mode = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
                    if (chmod(exec_path, new_mode) != 0) {
                        perror("Failed to set execute permissions");
                        close(Pipe_PtoC[0]);
                        close(Pipe_PtoC[1]);
                        return;
                    }
                }
                
                // Create new argument array
                int arg_count = 0;
                while (cmd_args[arg_count] != NULL) arg_count++;
                
                exec_args = malloc((arg_count + 1) * sizeof(char *));
                if (!exec_args) {
                    perror("malloc failed");
                    close(Pipe_PtoC[0]);
                    close(Pipe_PtoC[1]);
                    return;
                }
                
                exec_args[0] = strdup(exec_path);
                if (!exec_args[0]) {
                    free(exec_args);
                    perror("strdup failed");
                    close(Pipe_PtoC[0]);
                    close(Pipe_PtoC[1]);
                    return;
                }
                
                for (int j = 1; j < arg_count; j++) {
                    exec_args[j] = strdup(cmd_args[j]);
                    if (!exec_args[j]) {
                        for (int k = 0; k < j; k++) {
                            free(exec_args[k]);
                        }
                        free(exec_args);
                        perror("strdup failed");
                        close(Pipe_PtoC[0]);
                        close(Pipe_PtoC[1]);
                        return;
                    }
                }
                exec_args[arg_count] = NULL;
                break;
            }
        }
        
        if (!is_builtin) {
            mvprintw(input->line++, 1, "Command not found: %s", cmd_args[0]);
            close(Pipe_PtoC[0]);
            close(Pipe_PtoC[1]);
            return;
        }
    }

    // Execute command using single fork
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        if (is_builtin) {
            for (int j = 0; exec_args[j] != NULL; j++) {
                free(exec_args[j]);
            }
            free(exec_args);
        }
        close(Pipe_PtoC[0]);
        close(Pipe_PtoC[1]);
        return;
    }

    if (pid == 0) { // Child process
        close(Pipe_PtoC[0]);
        dup2(Pipe_PtoC[1], STDOUT_FILENO);
        close(Pipe_PtoC[1]);

        execv(exec_path, exec_args);
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(Pipe_PtoC[1]);
        
        if (is_builtin && strcmp(original_cmd, "cd") == 0) {
            char new_cwd[PATH_MAX];
            ssize_t nbytes = read(Pipe_PtoC[0], new_cwd, sizeof(new_cwd) - 1);
            if (nbytes > 0) {
                new_cwd[nbytes] = '\0';
                char *newline_pos = strchr(new_cwd, '\n');
                if (newline_pos) *newline_pos = '\0';
                
                if (chdir(new_cwd) == 0) {
                    if (getcwd(input->cwd, sizeof(input->cwd)) != NULL) {
                        // Keep the original shell_scripts directory
                        char *username = getenv("USER");
                        if (username) {
                            safe_string_join(input->username, PATH_MAX, username, input->cwd, ":");
                            strncat(input->username, "$ ", PATH_MAX - strlen(input->username) - 1);
                        }
                    }
                } else {
                    mvprintw(input->line++, 1, "Failed to change directory\n");
                }
            }
        } else {
            char buffer[1024];
            ssize_t nbytes;
            while ((nbytes = read(Pipe_PtoC[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[nbytes] = '\0';
                display(buffer);
            }
        }

        close(Pipe_PtoC[0]);
        int status;
        waitpid(pid, &status, 0);

        // Clean up if using built-in command
        if (is_builtin) {
            for (int j = 0; exec_args[j] != NULL; j++) {
                free(exec_args[j]);
            }
            free(exec_args);
        }
    }
}
int main(int argc, char **argv) {
  //init screen
  initscr();
  start_color();
  
  //intializing input line
  InputLine *input = (InputLine *)malloc(sizeof(InputLine));
  if (input == NULL) {
    endwin();
    return -1;
  }
  
  //getting the current working directory
  if (getcwd(input->cwd, sizeof(input->cwd)) == NULL) {
    perror("getcwd() error");
    return 1;
  }
  
  // Initialize shell scripts path
  init_shell_scripts_path(input);

  char *username = getenv("USER");

  //adding the user and current directory
  size_t username_len = strlen(username) + strlen(input->cwd) + 5;
  input->username = (char *)malloc(username_len*sizeof(char));
  if (input->username == NULL) {
      perror("malloc() error");
      return 1;
  }

  int len = snprintf(input->username, username_len, "%s:%s$ ", username, input->cwd);
  if (len < 0 || (size_t)len >= username_len) {
      perror("snprintf() error");
      free(input->username);
      return 1;
  }

  input->line = 0;

  if(can_change_color()) {
    init_color(COLOR_BLUE,0,0,300);
  }

  //define color parts
  init_pair(1, COLOR_WHITE, COLOR_BLUE);

  //set up full screen with the color
  bkgd(COLOR_PAIR(1));

  //Hello display method
  attron(COLOR_PAIR(1)); 
  mvprintw(input->line++,1,"Hello World, Welcome to TerraBine");
  attroff(COLOR_PAIR(1));

  //main messaging area
  while(1) {
    attron(COLOR_PAIR(1)); //turn on the color init_pair
    char * cmd = write_command(input);

    if(cmd == NULL) {
      break;
    }
    
    //for end cmd
    if(check_end(cmd)){
      free(cmd);
      break;
    }
    
    //for clr cmd
    else if(check_clear(cmd)) {
      clear();
      input->line = 0;
    }
    
    //command execution call
    else if(strcmp(cmd,"") != 0) {
      //spliting the input line into command and its arguments
      char **cmd_args;
      //char *result;

      cmd_args = split_line(cmd,input);
      execute_args(cmd_args,input);
      //mvprintw(input->line++,1,"%s",msg);
    }
    
    free(cmd);
    attroff(COLOR_PAIR(1));
    refresh();
    //pause the screen output
    //getch();
  }
  
  free(input->username);
  free(input);
  endwin();
  return 0;
}
