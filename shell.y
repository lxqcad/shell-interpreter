/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

/* See shell.l for the desription of tokens: WORD,  */
%token	<string_val> WORD

%token GREAT GREAT_GREAT GREAT_AMP GREAT_GREAT_AMP TWO_GREAT LESS AMPERSAND NEWLINE PIPE END_OF_FILE

// Every 
%union	{
		char   *string_val;
	}

%{
#include "command.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
void yyerror(const char * s);
int yylex();

/* Expand all occurrences of strings of the form ${var} */
char * expandEnvVariables(char * str);

/* Return the result of the expanding and free var, in case
   var in the environment table and NULL, otherwise. */
char * expandSingleEnvVar(char * var);
extern "C" char ** process_wildcards(char *regularExp);

%}

%%

goal:
	commands
	;

commands:
	command
	| commands command
	;

command: simple_command
	;

simple_command:
	command_and_args pipes_seq iomodifier_opts background NEWLINE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE
    | END_OF_FILE
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word argument_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

pipes_seq:
	pipes_seq PIPE command_and_args
	| /* can be empty */
	;

argument_list:
	argument_list argument
	| /* can be empty */
	;

argument:
	WORD {
		$1 = expandEnvVariables($1);
        if(strchr($1, '*') != NULL || strchr($1, '?') != NULL) {
        	char **retStr = process_wildcards($1);
            if(retStr != NULL) {
            	int c = 0;
                while(retStr[c] != NULL) {
		    		/*printf("   Yacc: insert argument \"%s\"\n", retStr[c]); */
            		Command::_currentSimpleCommand->insertArgument( retStr[c] );
                    c++;
                }
            }
        }
        else {
			Command::_currentSimpleCommand->insertArgument( $1 );
        }
	}
	;

command_word:
	WORD {
		//printf("   Yacc: insert command \"%s\"\n", $1);
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opts:
	iomodifier_opt
	| iomodifier_opts iomodifier_opt
	| /* can be empty */
	;

iomodifier_opt:
	GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
        if(Command::_currentCommand._outFile)
            Command::_currentCommand.error("Ambiguous output redirect.");
        else
            Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		//printf("   Yacc: insert input \"%s\"\n", $2);
        if(Command::_currentCommand._inFile)
            Command::_currentCommand.error("Ambiguous input redirect.");
        else
            Command::_currentCommand._inFile = $2;
	}
    | TWO_GREAT WORD {
		//printf("   Yacc: insert error \"%s\"\n", $2);
        if(Command::_currentCommand._errFile)
            Command::_currentCommand.error("Ambiguous error redirect.");
        else
            Command::_currentCommand._errFile = $2;
	}
	| GREAT_AMP WORD {
		//printf("   Yacc: insert error \"%s\"\n", $2);
        if(Command::_currentCommand._outFile)
            Command::_currentCommand.error("Ambiguous output redirect.");
        else
            Command::_currentCommand._outFile = $2;
        if(Command::_currentCommand._errFile)
            Command::_currentCommand.error("Ambiguous error redirect.");
        else
            Command::_currentCommand._errFile = $2;
	}
	| GREAT_GREAT  WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
        if(Command::_currentCommand._outFile)
            Command::_currentCommand.error("Ambiguous output redirect.");
        else
            Command::_currentCommand._outFile = $2;
        Command::_currentCommand._append = 1;
	}
	| GREAT_GREAT_AMP WORD {
		//printf("   Yacc: insert error \"%s\"\n", $2);
        if(Command::_currentCommand._outFile)
            Command::_currentCommand.error("Ambiguous output redirect.");
        else
            Command::_currentCommand._outFile = $2;
        if(Command::_currentCommand._errFile)
            Command::_currentCommand.error("Ambiguous error redirect.");
        else
            Command::_currentCommand._errFile = $2;
        Command::_currentCommand._append = 1;
	}
	;

background:
	AMPERSAND {
		//printf("   Yacc: Execute in background\n");
        if(Command::_currentCommand._background)
            Command::_currentCommand.error("Ambiguous background process.");
        else
            Command::_currentCommand._background = true;
	}
	| /* can be empty */
	;
 
%%

char * expandEnvVariables (char * str) {
	const char *regex_string = "[$][{]([^} \t]*)[}]";
	size_t max_matches = strlen(str);
	size_t max_groups = 2;

	regex_t regex_compiled;
	regmatch_t group_array[max_groups];
	char *cursor = str;

	if (regcomp(&regex_compiled, regex_string, REG_EXTENDED))
		return NULL;

	char *exp_str = (char*) malloc((strlen(str) * 10 + 1)  * sizeof(char));
	int max_exp_str_len = strlen(str) * 2;

	int exp_str_offset = 0;
	for (int m = 0; m < max_matches; m++) {
		if (regexec(&regex_compiled, cursor, max_groups, group_array, 0))
			break;  // No more matches

		strncpy(&exp_str[exp_str_offset], cursor, group_array[0].rm_so);
		exp_str_offset += group_array[0].rm_so;
		int group_length = group_array[1].rm_eo - group_array[1].rm_so;
		char *var = (char*) malloc((group_length + 1) * sizeof(char));
		strncpy(var, cursor + group_array[1].rm_so, group_length);
		var[group_length] = '\0'; // modified here : removed + 1
		char *var_exp = expandSingleEnvVar(var);
		strncpy(&exp_str[exp_str_offset], var_exp, strlen(var_exp));
		exp_str_offset += strlen(var_exp);
		cursor += group_array[0].rm_eo;
		free(var_exp);
	}

	if (strlen(cursor) > 0) {
		strncpy(&exp_str[exp_str_offset], cursor, strlen(cursor));
		exp_str_offset += strlen(cursor);
	}
	regfree(&regex_compiled);
	exp_str[exp_str_offset] = '\0';
	free(str);
	return exp_str;
}

char * expandSingleEnvVar(char * var) {
	if (!strcmp(var, "SHELL")) {
    	free(var);
    	var=realpath(Command::_currentCommand._pathToShell,NULL);
		return var;
    }
	if (!strcmp(var, "$")) {
		var = (char*)realloc(var, 10 * sizeof(char));
		snprintf(var, 10,"%ld",(long) getpid());
		return var;
	}
	if (!strcmp(var, "?")) {
		var = (char*)realloc(var, 10 * sizeof(char));
		snprintf(var, 10,"%d",Command::_currentCommand._lastStatus);
		return var;
    }
	if (!strcmp(var, "!")) {
		var = (char*)realloc(var, 10 * sizeof(char));
		snprintf(var, 10,"%d",Command::_currentCommand._lastPID);
		return var;
    }
	if (!strcmp(var, "_")) {
    	if(Command::_currentCommand._lastArgument == NULL) {
        	strcpy(var, " ");
        }
        else {
			var = (char*)realloc(var, strlen(Command::_currentCommand._lastArgument) * sizeof(char));
			strcpy(var, Command::_currentCommand._lastArgument);
        }
		return var;
    }

	char* table_value = getenv(var);
    //printf("\nVarValue[%s]=%s\n", var, table_value);
	if (table_value) {
		free(var);
		return strdup(table_value);
	}
	return var;
}

void
yyerror(const char * s)
{
	fprintf(stderr,"YACC:%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
