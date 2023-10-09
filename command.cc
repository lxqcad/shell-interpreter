/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>

#include <string>

#include "command.h"

extern char **environ;
extern FILE *yyin;
const int _maxInternalCommands = 8;
const char *_internalCommands[8] = {"exit","cd","setenv","unsetenv","source","printenv","history","help"};
int max_bg_procs = 15, bg_pids[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int yyparse(void);
void signal_handler(int signal);
void scan_string(const char *str);
extern "C" void print_history(void);
extern "C" void read_line_print_usage(void);

#define P1_READ     0
#define P2_WRITE    1


SimpleCommand::SimpleCommand()
{
    // Create available space for 5 arguments
    _numOfAvailableArguments = 5;
    _numOfArguments = 0;
    _arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
    if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
        // Double the available space
        _numOfAvailableArguments *= 2;
        _arguments = (char **) realloc( _arguments,
                  _numOfAvailableArguments * sizeof( char * ) );
    }
    
    _arguments[ _numOfArguments ] = argument;

    // Add NULL argument at the end
    _arguments[ _numOfArguments + 1] = NULL;
    
    _numOfArguments++;
}

Command::Command()
{
    // Create available space for one simple command
    _numOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

    _numOfSimpleCommands = 0;
    _outFile = 0;
    _inFile = 0;
    _errFile = 0;
    _background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
    if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
        _numOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
             _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
    }
    
    _simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
    _numOfSimpleCommands++;
}

void
Command:: clear()
{
    for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
        for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
            free ( _simpleCommands[ i ]->_arguments[ j ] );
        }
        
        free ( _simpleCommands[ i ]->_arguments );
        free ( _simpleCommands[ i ] );
    }

    if ( _outFile ) {
        if( _errFile ) {
            if(strcmp(_outFile, _errFile) == 0) {
                _errFile = 0;
            }            
        }
        free( _outFile );
    }

    if ( _inFile ) {
        free( _inFile );
    }

    if ( _errFile ) {
        free( _errFile );
    }
    
    if ( _error ) {
        free ( _error );
    }

    _numOfSimpleCommands = 0;
    _outFile = 0;
    _inFile = 0;
    _errFile = 0;
    _error = 0;
    _append = 0;
    _background = 0;
}

void 
Command::error(const char *errStr)
{
    if( _error ) return;
    _error = (char *)malloc (sizeof(char) * strlen(errStr));
    strcpy(_error, errStr);
}

void
Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");
    
    for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
        printf("  %-3d ", i );
        for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
        }
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
        _inFile?_inFile:"default", _errFile?_errFile:"default",
        _background?"YES":"NO");
    printf( "\n\n" );
    
}

void
set_fd_to(int target_fd, int source_fd)
{
    if (-1 == dup2(source_fd, target_fd)) {
        perror("command: dup2");
        return;
    }
}

void
set_fd_to_file(int target_fd, const char *file_name, int flags)
{

    assert(NULL != file_name);
    
    int source_fd = open(file_name, flags, 0666);
    if (-1 == source_fd) {
        perror("command: creat");
        return;
    }

    set_fd_to(target_fd, source_fd);
    close(source_fd);
}


void
set_output(int target_fd, const char *file_name, int *std_io, int _iappend)
{
    assert((1 == target_fd) || (2 == target_fd));
    assert(NULL != std_io);

    if (NULL == file_name) {
        set_fd_to(target_fd, std_io[target_fd]);

    } else {
        if(_iappend == 0)
            set_fd_to_file(target_fd, file_name, O_CREAT | O_WRONLY | O_TRUNC);
        else
            set_fd_to_file(target_fd, file_name, O_CREAT | O_WRONLY | O_APPEND);
    }
}

void
Command::execute()
{
    int c = 0;
    // Don't do anything if there are no simple commands
    if ( _numOfSimpleCommands == 0 ) {
        prompt();
        return;
    }

    // handle internal commands
    for (c = 0; c < _maxInternalCommands; c++) {
        if(!strcmp(_simpleCommands[0]->_arguments[0], _internalCommands[c]))
            break;
    }
    switch(c) {
    case 0:
        printf("Good bye!!\n");
        exit(0);
    case 1:
        if(_simpleCommands[0]->_numOfArguments < 2)
            chdir(getenv("HOME"));
        else
            if(chdir(_simpleCommands[0]->_arguments[1]) != 0) {
                fprintf(stderr, "shell: can't cd to %s\n", _simpleCommands[0]->_arguments[1]);
            }
        break;
    case 2:
        if(_simpleCommands[0]->_numOfArguments != 3) {
            fprintf(stderr, "command: setenv - Incorrect number of arguments\n");
        }
        else {
            if(setenv(_simpleCommands[0]->_arguments[1], _simpleCommands[0]->_arguments[2], 1) != 0)
                perror("command: setenv");
        }    	
        break;
    case 3:
        if(_simpleCommands[0]->_numOfArguments != 2) {
            fprintf(stderr, "command: unsetenv - Incorrect number of arguments\n");
        }
        else {
            if(unsetenv(_simpleCommands[0]->_arguments[1])!=0)
                perror("command: unsetenv");
        }
        break;
    }
    if (c <= 3 || _error ) {
        if( _error )
            fprintf(stderr, "%s\n", _error);
        clear();
        prompt();
        return;
    }
    // Print contents of Command data structure
    //print();

    int std_io[3] = {0};
    int pid;
    int i = 0;
    int j = 0, k = 1;
    char *s = *environ;

    for (int i = 0; i < 3; i++) {
        std_io[i] = dup(i);
        if (-1 == std_io[i]) {
            perror("command: dup");
            exit(2);
        }
    }

    int (* fdpipe)[2] = NULL;
    if (_numOfSimpleCommands > 1) {
        fdpipe = (int (*)[2])
        calloc((_numOfSimpleCommands - 1) * 2, sizeof(**fdpipe));
            if (NULL == fdpipe) {
            perror("command: calloc");
            exit(2);
        }
    }

    if (NULL != _inFile) {
        set_fd_to_file(STDIN_FILENO, _inFile, O_RDONLY);
    }

    for ( i = 0; i < _numOfSimpleCommands; i++ ) {
        if (i < (_numOfSimpleCommands - 1)) {
            if (0 != pipe(fdpipe[i])) {
                perror("command: pipe");
                exit(2);
            }
        }

        if (i < (_numOfSimpleCommands - 1)) {

            set_fd_to(STDOUT_FILENO, fdpipe[i][STDOUT_FILENO]);
            close(fdpipe[i][STDOUT_FILENO]);

        } else {
            set_output(STDOUT_FILENO, _outFile, std_io, _append);
            set_output(STDERR_FILENO, _errFile, std_io, _append);
        }

        if (i > 0) {
            set_fd_to(STDIN_FILENO, fdpipe[i - 1][STDIN_FILENO]);
            close(fdpipe[i - 1][STDIN_FILENO]);
        }

        pid = fork();

        if (-1 == pid) {
            perror("command: fork");
            exit(2);
        }
        // child
        if (0 == pid) {

            for (j = 0; j < 3; j++) {
                close(std_io[j]); // changed i to j
            }

            for (j = 4; j < _maxInternalCommands; j++) {
                if(!strcmp(_simpleCommands[i]->_arguments[0], _internalCommands[j]))
                    break;
            }
        
            switch(j) {
            case 4: // source
                execvp( _pathToShell, _simpleCommands[i]->_arguments);
                perror("command: execvp");
                exit(2);
            case 5: // printenv
                k = 1;

                for (; s; k++) {
                    printf("%s\n", s);
                    s = *(environ+k);
                }
                break;
            case 6: // command history
                print_history();
                break;
            case 7: // help
                read_line_print_usage();
                break;
            case 8: // default
                execvp( _simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

                // execvp shouldn't return
                perror("command: execvp");
                exit(2);
            }
            exit(0);
        }
    }

    int status = 0, lp = _numOfSimpleCommands-1;
    if (!_background) {
        if (-1 == waitpid(pid, &status, 0)) {
            perror("shell: waitpid");
            exit(2);
        }
        if (WIFEXITED(status))
            status = WEXITSTATUS(status);    
        _lastStatus = status;
    }
    else {
        _lastPID = pid;
        for(i = 0;i < max_bg_procs;i++) {
            if(bg_pids[i] == 0) {
                bg_pids[i] = pid;
                break;
            }
        }
    }
    
    _lastArgument = (char *)realloc(_lastArgument, sizeof(char) * strlen(_simpleCommands[lp]->_arguments[_simpleCommands[lp]->_numOfArguments-1]));
    strcpy(_lastArgument, _simpleCommands[lp]->_arguments[_simpleCommands[lp]->_numOfArguments-1]);

    for (i = 0; i < 3; i++) {
        set_fd_to(i, std_io[i]);
        close(std_io[i]);
    }

    free(fdpipe);

    // Clear to prepare for next command
    clear();
    
    // Print new prompt
    prompt();
}

// Shell implementation

void
Command::prompt()
{
    char *str_prompt = getenv("PROMPT");

    if(isatty(STDIN_FILENO) == 1) {
        if(str_prompt == NULL)
            printf("myshell>");
        else
            printf("%s",str_prompt);
    }
    fflush(stdout);
}
// Invoke Subshell
char *
Command::subshell(char *cmdText)
{
    int fd[2];
    int  status, len;
    char *args[3];
    pid_t pid;

    // create all the descriptor pairs we need
    if (pipe(fd) < 0)
    {
        perror("Failed to allocate pipes");
        exit(EXIT_FAILURE);
    }
    if ((pid = fork()) < 0)
    {
        perror("Failed to fork process");
        return NULL;
    }

    // if the pid is zero, this is the child process
    if (pid == 0)
    {
        args[0] = (char *)&_pathToShell[0]; //(char *)malloc(strlen(_pathToShell) * sizeof(char));
        args[1] = (char *)cmdText;
        args[2] = NULL;
        // Child. Start by closing descriptors we
        //  don't need in this process
        close(STDOUT_FILENO);  //closing stdout
        dup2(fd[P2_WRITE], STDOUT_FILENO);         //replacing stdout with pipe write 

        close(fd[P1_READ]); 

        execvp( _pathToShell, args);
        perror("command: execvp");
        exit(2);

    }
    // Parent. close unneeded descriptors
    close(fd[P2_WRITE]);

    // now wait for a response
    cmdText = (char *)realloc(cmdText, 2048 * sizeof(char));
    len = read(fd[P1_READ], cmdText, 2048);
    if (len < 0)
    {
        perror("Parent: failed to read value from pipe");
        exit(EXIT_FAILURE);
    }
    else if (len == 0)
    {
        // not an error, but certainly unexpected
        fprintf(stderr,"Parent of child(%d): Read EOF from pipe", pid);
    }

    // close down remaining descriptors
    close(fd[P1_READ]);

    // wait for child termination
    //kill(pid, SIGKILL);
    if (-1 == waitpid(pid, &status, 0)) {
        perror("shell: waitpid");
    }
    
    return cmdText;
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int main(int argc, char* argv[])
{
    struct sigaction sa;

    if(argc > 1) {
        if( access( argv[1], F_OK ) != -1 ) {
            yyin = fopen(argv[1], "r");
            yyparse();
            fclose(yyin);
        }
        else {
            scan_string(argv[1]);
            yyparse();
        }
        exit(0);
    }

    /* For handling CTRL+C command */
    sa.sa_handler = &signal_handler;
    sigemptyset(&sa.sa_mask);
    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGINT"); // Should not happen
    }
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGCHLD"); // Should not happen
    }
    
    Command::_currentCommand._pathToShell = argv[0];
    
    if( access( ".shellrc", F_OK ) != -1 ) {
        char *srcCmd = (char *)malloc(15);
        char *srcArg = (char *)malloc(15);
        // .shellrc file exists
        strcpy(srcCmd, "source");
        strcpy(srcArg, ".shellrc");
        Command::_currentSimpleCommand = new SimpleCommand();
        Command::_currentSimpleCommand->insertArgument(srcCmd);
        Command::_currentSimpleCommand->insertArgument(srcArg);
        Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
        Command::_currentCommand.execute();
    }
    Command::_currentCommand.prompt();
    yyparse();

    return 0;
}

void signal_handler(int signal) 
{
    pid_t pid;
    int status, i;

    /* EEEEXTEERMINAAATE! */
    switch(signal) {
    case SIGINT:
        printf("\n");
        Command::_currentCommand.prompt();
        break;
    case SIGCHLD:
        while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            for(i = 0;i < max_bg_procs;i++) {
                if(bg_pids[i] == pid) {
                    printf("\n[%d] exited.\n", pid);
                    bg_pids[i] = 0;
                    break;
                }
            }
        }
        break;
    }
}