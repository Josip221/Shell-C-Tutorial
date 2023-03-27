#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// add quoting
// redirection
// error handling

#define SHELL_RL_BUFSIZE 256
#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"

int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &shell_cd,
  &shell_help,
  &shell_exit
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int shell_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return EXIT_SUCCESS;
}

int shell_help(char **args)
{
  int i;
  printf("Inspired by Stephen Brennan's shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < shell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return EXIT_SUCCESS;
}

int shell_exit(char **args)
{
  return EXIT_SUCCESS;
}

char *shell_read_line() {
int bufsize = SHELL_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      break;
    } else if (position >= bufsize - 1) {
      // If we have exceeded the buffer size, reallocate.
      bufsize += SHELL_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    } else if (c < 32 || c > 126) {
      // If the input is outside the printable ASCII range, ignore it.
      continue;
    } else {
      buffer[position] = c;
      position++;
    }
	}

	// If the input is empty, free the buffer and return NULL.
	if (position == 0) {
		free(buffer);
		return NULL;
	}

	return buffer;
}

char **shell_split_line(char *line) {
	int bufsize = SHELL_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "shell: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, SHELL_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
		bufsize += SHELL_TOK_BUFSIZE;
		tokens = realloc(tokens, bufsize * sizeof(char*));
		if (!tokens) {
			fprintf(stderr, "shell: allocation error\n");
			exit(EXIT_FAILURE);
		}
		}

		token = strtok(NULL, SHELL_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int shell_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("shell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("shell");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return EXIT_SUCCESS;
}

int shell_execute(char ** args){
	int i;

	if (args[0] == NULL) {
		// An empty command was entered.
		return EXIT_FAILURE;
	}

	for (i = 0; i < shell_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return shell_launch(args);
}

void shell_loop() {
	char *line;
	char **args;
	int status;

	do{
		printf("> ");
		line = shell_read_line();
		printf(" %s \n", line);
		args = shell_split_line(line);
		status = shell_execute(args);

		free(line);
		free(args);
	} while(status);
}

int main(int argc, char **argv) {
	shell_loop();
	return EXIT_SUCCESS;
}