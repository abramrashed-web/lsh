#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define HISTORY_MAX_SIZE 100

char *history[HISTORY_MAX_SIZE];
int history_count = 0;
extern char **environ;

/* Function Declarations for builtin shell commands */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

/* List of builtin commands, followed by their corresponding functions */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd",
  "echo",
  "history",
  "env"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_pwd,
  &lsh_echo,
  &lsh_history,
  &lsh_env
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/* Builtin function implementations */
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
  printf("Abram's LSH (Linux Shell)\n");
  printf("Type program names and arguments, then hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the 'man' command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_pwd(char **args)
{
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("pwd error");
  }
  return 1;
}

int lsh_echo(char **args)
{
  int i = 1;
  while (args[i] != NULL) {
    printf("%s ", args[i]);
    i++;
  }
  printf("\n");
  return 1;
}

int lsh_history(char **args)
{
  for (int i = 0; i < history_count; i++) {
    printf(" %d  %s\n", i + 1, history[i]);
  }
  return 1;
}

int lsh_env(char **args)
{
  int i = 0;
  while (environ[i] != NULL) {
    printf("%s\n", environ[i]);
    i++;
  }
  return 1;
}

void add_to_history(const char *cmd)
{
  if (cmd == NULL || strlen(cmd) == 0 || cmd[0] == '\n') {
    return;
  }
  
  char *cmd_copy = strdup(cmd);
  if (cmd_copy[strlen(cmd_copy) - 1] == '\n') {
    cmd_copy[strlen(cmd_copy) - 1] = '\0';
  }

  if (history_count < HISTORY_MAX_SIZE) {
    history[history_count] = cmd_copy;
    history_count++;
  } else {
    free(history[0]);
    for (int i = 1; i < HISTORY_MAX_SIZE; i++) {
      history[i - 1] = history[i];
    }
    history[HISTORY_MAX_SIZE - 1] = cmd_copy;
  }
}

int lsh_launch(char **args)
{
  pid_t pid;
  pid_t wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("lsh");
  } else {
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
  size_t bufsize = 0;
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("lsh: read_line");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

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
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        free(tokens_backup);
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
    add_to_history(line);
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  lsh_loop();
  return EXIT_SUCCESS;
}
