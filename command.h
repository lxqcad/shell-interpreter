#ifndef command_h
#define command_h

#include <unistd.h>

#include <list>
using namespace std;

// Command Data Structure
struct SimpleCommand {
	// Available space for arguments currently preallocated
	int _numOfAvailableArguments;

	// Number of arguments
	int _numOfArguments;
	char ** _arguments;
	
	SimpleCommand();
	void insertArgument( char * argument );
};
struct Command {
	int _numOfAvailableSimpleCommands;
	int _numOfSimpleCommands;
	SimpleCommand ** _simpleCommands;
	char * _outFile;
	char * _inFile;
	char * _errFile;
	char * _pathToShell;
    char * _error;
	int _background = 0;
	int _lastStatus = 0;
    int _append = 0;
	pid_t _lastPID = 0;
	char * _lastArgument = NULL;

	void prompt();
    void error(const char *);
	void print();
	void execute();
	void clear();
	char *subshell(char *);
	
	Command();
	void insertSimpleCommand( SimpleCommand * simpleCommand );

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
