
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include <errno.h>
#include "command.h"
#include "tokenizer.h"


void printError(const char * msg ,SimpleCommand *currentSimpleCommand ,Command &currentCommand){
  fprintf(stderr,"%s" ,msg);
  for (int j = 0; j < currentSimpleCommand->_numberOfArguments; j++)
       {
	 free(currentSimpleCommand->_arguments[j]);
	}
        free(currentSimpleCommand->_arguments);
        delete currentSimpleCommand;
	currentCommand.clear();
	free(currentCommand._simpleCommands); 
}

void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    if (tokens[0].type == TOKEN_EOF){
	return;
	}
SimpleCommand *currentSimpleCommand = new SimpleCommand();

for (size_t i = 0 ; i < tokens.size() ; i++)
    {
      if (tokens[i].type == TOKEN_EOF )
	{
	if (currentSimpleCommand->_numberOfArguments != 0){ 
	 currentCommand.insertSimpleCommand(currentSimpleCommand);
	 currentCommand.execute();
	}
	else {
    if (currentCommand._numberOfSimpleCommands == 0)
        printError("Syntax error: missing command\n",currentSimpleCommand, currentCommand);
    else
        printError("Syntax error: missing command after |\n",currentSimpleCommand, currentCommand);
    return;
}
	}
	else if (tokens[i].type == TOKEN_PIPE)
	{
	if (currentSimpleCommand->_numberOfArguments != 0)
	{
	currentCommand.insertSimpleCommand(currentSimpleCommand);
	currentSimpleCommand = new SimpleCommand();
	}
	 else {
    if (currentCommand._numberOfSimpleCommands == 0)
         printError("Syntax error: missing command\n",currentSimpleCommand, currentCommand);
    else 
       printError("Syntax error: missing command after |\n",currentSimpleCommand, currentCommand);
    return;
}

	}
	else {
	if (tokens[i].type == TOKEN_REDIRECT){
	if(tokens[i+1].type == TOKEN_COMMAND || tokens[i+1].type ==  TOKEN_ARGUMENT){
	free(currentCommand._outFile);
	currentCommand._outFile = strdup(tokens[i+1].value.c_str());
	i++;
	}
	else {
	printError("File name is missing after > \n", currentSimpleCommand, currentCommand);
	return; 
	}
	}
	else if (tokens[i].type == TOKEN_INPUT) {
	if(tokens[i+1].type == TOKEN_COMMAND || tokens[i+1].type ==  TOKEN_ARGUMENT){
	free(currentCommand._inputFile);
	currentCommand._inputFile = strdup(tokens[i+1].value.c_str());
        i++;
	}
	else {
	printError("File name is missing after < \n", currentSimpleCommand, currentCommand);
	return; 
	}
	}
	else if (tokens[i].type == TOKEN_APPEND ){
	if (tokens[i+1].type == TOKEN_COMMAND || tokens[i+1].type ==  TOKEN_ARGUMENT)
	{
	free(currentCommand._outFile);
	currentCommand._outFile = strdup(tokens[i+1].value.c_str());
        currentCommand._append = 1;
	i++;
	}
	else {
	printError("File name is missing after >> \n", currentSimpleCommand, currentCommand);
	return; 
	}
	}
	else if (tokens[i].type == TOKEN_ERROR ){
	if (tokens[i+1].type == TOKEN_COMMAND || tokens[i+1].type ==  TOKEN_ARGUMENT){
	free(currentCommand._errFile);
	currentCommand._errFile = strdup(tokens[i+1].value.c_str());
        i++;
	}
	else {
	printError("File name is missing after 2> \n", currentSimpleCommand, currentCommand);
	return; 
	}
	}
	else if (tokens[i].type == TOKEN_REDIRECT_AND_ERROR){ 
	if (tokens[i+1].type == TOKEN_COMMAND || tokens[i+1].type ==  TOKEN_ARGUMENT){
        free(currentCommand._outFile);
	currentCommand._outFile = strdup(tokens[i+1].value.c_str());
        currentCommand._append = 1;
	currentCommand._out_error = 1;
	i++;
        }
        else {
        printError("File name is missing after >>& \n", currentSimpleCommand, currentCommand);
        return; 
        }
        }
	else if (tokens[i].type == TOKEN_BACKGROUND){
	if (tokens[i+1].type == TOKEN_EOF){
	currentCommand._background = 1;
	}
	else{
        printError( "& should be last in line \n", currentSimpleCommand, currentCommand);	
	return; 
	}
	}
	else if(tokens[i].type == TOKEN_COMMAND || tokens[i].type ==  TOKEN_ARGUMENT) {
	if (tokens[i].value.find_first_of("*?") != std::string::npos){
	glob_t g;
	int x = glob(tokens[i].value.c_str(), 0, NULL, &g);
	if (x == 0){
	for (size_t j = 0; j < g.gl_pathc; j++)
        currentSimpleCommand->insertArgument(strdup(g.gl_pathv[j]));
	globfree(&g);
	}
	else {
	printError("No match. \n", currentSimpleCommand, currentCommand);
	return;
	}
	}
	else {
	currentSimpleCommand->insertArgument(strdup(tokens[i].value.c_str()));
	  }
	}
      }
    }
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
   _arguments[0] = NULL;
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error =0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        delete _simpleCommands[i];
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error= 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
	   printf("\n");
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _outFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

static int logFile = -1 ; 

void Command::execute()
{
    auto cleaning = [this]() {
    clear();
    free(_simpleCommands);
};
    print();
    fflush(stdout);
    if (_numberOfSimpleCommands == 1){
    char **a = _simpleCommands[0]->_arguments;
    if (strcmp(a[0],"exit") == 0){
    printf("Good bye!!\n");
    cleaning();
    exit(0);
    }
    if (strcmp(a[0],"cd") == 0){
    char *dir = a[1] ? a[1] : getenv("HOME");
    if (dir == NULL || chdir(dir) < 0) perror("cd");
    cleaning();
    return; 
    }
    }
    int errFd = -1;
    if (_errFile) {
        errFd = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (errFd < 0) {
            perror(_errFile);
            cleaning();
            return;
        }
    }
    int pipes[_numberOfSimpleCommands][2];
    int childrenPid[_numberOfSimpleCommands];
    for (int i = 0 ; i < _numberOfSimpleCommands - 1 ; i++) 
    if(pipe(pipes[i]) == -1){
    perror("Pipe");
    if (errFd != -1) close(errFd); 
    for (int k = 0; k <= i; k++) {
    close(pipes[k][0]);
    close(pipes[k][1]);
    }
    cleaning();
    return;
    }
    for (int i = 0 ; i < _numberOfSimpleCommands ; i++){
    pid_t x = fork();
    if (x == 0) {
    if (i > 0) dup2(pipes[i-1][0],0);
    if (i < _numberOfSimpleCommands -1 ) dup2(pipes[i][1],1);
    for (int k = 0; k < _numberOfSimpleCommands - 1; k++) {
    close(pipes[k][0]);
    close(pipes[k][1]);
    }
    if (_outFile && i == _numberOfSimpleCommands - 1){
    int file = open(_outFile, O_WRONLY | O_CREAT | (_append ? O_APPEND : O_TRUNC),0644);
    if (file < 0) {
    perror(_outFile);
    _exit(1);
    }
    if(_out_error == 1){
    dup2(file,1);
    dup2(file,2);
    }
    else dup2(file,1);
    close(file);
    }
    if (_inputFile && i == 0 ) {
    int file = open(_inputFile, O_RDONLY ,0644);
    if (file < 0) {
    perror(_inputFile);
    _exit(1);
    }
    dup2(file,0);
    close(file);
    }
    if (errFd != -1) {
        dup2(errFd, 2);
        close(errFd);
    }
    signal(SIGINT, SIG_DFL);
    execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
    perror(_simpleCommands[i]->_arguments[0]);
    _exit(1);
    }
    else if (x == -1) {
    perror("Fork");
    if (errFd != -1) close(errFd); 
    cleaning();
    return;
    }
    else {
    childrenPid[i] = x ;
    }
   
}
for (int i = 0; i < _numberOfSimpleCommands - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
}
if (errFd != -1) close(errFd);
if (_background == 0){
        if (waitpid(childrenPid[_numberOfSimpleCommands - 1], NULL, 0) > 0) {
            char buf[64];
            int n = snprintf(buf, sizeof buf, "Child process %d terminated\n", (int)childrenPid[_numberOfSimpleCommands - 1]);
            write(logFile, buf, n);
        }
    }
cleaning();
}
void Command::prompt()
{
    printf("myshell>");
    fflush(stdout);
    std::string input;
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;


static void chSigHandler(int){
int prevErrno = errno;
int status; pid_t pid;
while ((pid = waitpid(-1,&status,WNOHANG)) > 0){
char buf[64];
int n = snprintf(buf, sizeof buf, "Child process %d terminated\n", (int)pid);
write(logFile,buf,n);
}
errno = prevErrno;
}

int main() {
    std::string input;
    signal(SIGINT, SIG_IGN);
    logFile = open("myshell.log", O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    struct sigaction chSig;
    chSig.sa_handler = chSigHandler;
    sigemptyset(&chSig.sa_mask);
    chSig.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &chSig, NULL);
    while (true) {
        printf("myshell> "); fflush(stdout);
        if (!std::getline(std::cin, input)) { printf("\n"); break; }
        auto tokens = tokenize(input);
        parse(tokens);   
    }
}
