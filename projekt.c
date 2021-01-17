#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h> 
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a "

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_ls(char **args);
int lsh_pwd();
int lsh_cat(char *args[]);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "ls",
  "pwd",
  "cat"
};
int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_ls,
  &lsh_pwd,
  &lsh_cat
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}



/*
  Builtin function implementations.
*/

int lsh_cat( char *args[]) {
	errno = 0;
	FILE *fp;
	int maxline = 1024;
	char line[maxline];

	if (args[1] != NULL) {
		if ((fp = fopen(args[1], "rb")) == NULL) {
			puts("ERROR: ");
		}
	}
	else {
		puts("ERROR: ");
	}
	while (fgets(line, maxline, fp)) {
		printf("%s", line);
	}
	fclose(fp);
	return 1;
}

char *getcwd(char *buf, size_t size);

int lsh_pwd() {
   char cwd[PATH_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       puts( cwd);
   } else {
       perror("getcwd() error");
       return 1;
   }
   return 1;
}

int lsh_ls(char **args) 
{ 
    errno = 0;
	struct dirent **contents;
	int content_count;

	if (args[1] == NULL) {
		if ((content_count = scandir("./", &contents, NULL, alphasort)) < 0) {
			puts("ERROR");
		}
	}
	else if (args[1] != NULL) {
		if ((content_count = scandir(args[1], &contents, NULL, alphasort)) < 0) {
			puts("ERROR");
		}
	}
	else{
		puts("ERROR");
	}

	int i;
	for (i = 0; i < content_count; i++) {
		puts(contents[i]->d_name);
	}

	return 1;
}

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

/*

*/

int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;
  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

int lsh_execute(char **args)
{
  int i;
  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }
  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return lsh_launch(args);
}

char *lsh_read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We recieved an EOF
    } else  {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void lsh_loop(void)
{
  char *line;
  char **args;
  int status;
  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);
    free(line);
    free(args);
  } while (status);
}



int main(int argc, char **argv)
{
  // Load config files, if any.
  // Run command loop.
  lsh_loop();
  // Perform any shutdown/cleanup.
  return EXIT_SUCCESS;
}
