/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "command.h"
#include "y.tab.hh"

////////////  Start added code ///////////

extern char *get_home_dir(char *name);
extern "C" char * read_line();
extern "C" char *str_replace(const char *rep, const char *with, char *orig);


int mygetc(FILE * f) {
 static char *p;
 char ch;

 if (!isatty(0)) {
 // stdin is not a tty. Call real getc
   return getc(f);
 }

 // stdin is a tty. Call our read_line.
 if (p==NULL || *p == 0) {
   char * s = read_line();
   p = s;
 }

 ch = *p;
 p++;

 return ch;
}

#undef getc
#define getc(f) mygetc(f)

void scan_string(const char* str)
{
    yy_switch_to_buffer(yy_scan_string(str));
}

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

/* Remove all escape characters from str and update it.
   See the detailed explanation in the
   definition. You can find it below.  */
void removeEscapeChars (char *str);

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}

">" 	{
		return GREAT;
	}

">>" 	{
		return GREAT_GREAT;
	}

">&" 	{
		return GREAT_AMP;
	}

">>&" 	{
		return GREAT_GREAT_AMP;
	}

"2>" 	{
		return TWO_GREAT;
	}

"<" 	{
		return LESS;
	}

"&" 	{
		return AMPERSAND;
	}

"|" 	{
		return PIPE;
	}
<<EOF>>                         { yyterminate(); return 0; };

~[^ \t\n]* {
	/* to handle tilde expression */
    int i, len = strlen(yytext)-1;
    
    if(len == 0) {
        char *strtext = getenv("HOME");
        yylval.string_val = (char *)malloc(sizeof(char) * strlen(strtext));
        strcpy(yylval.string_val, strtext);

        return WORD;
		free(yylval.string_val);
    }
    else {
    	char *user_home;

    	char *strtext = (char *)malloc((len+1) * sizeof(char));
        char *strtxt2 = (char *)malloc((len+1) * sizeof(char));
    	strcpy(strtext, &yytext[1]); /* remove tilde char */
        strcpy(strtxt2, strtext);
    	for (i = 0; i < len;i++) {
    		if(strtxt2[i] == '/') break;
   		}
        
        strtxt2[i] = '\0';
        if(i == 0)
        	user_home = getenv("HOME");
        else
        	user_home = get_home_dir(strtxt2);
        free(strtxt2);
        yylval.string_val = (char *)malloc(sizeof(char) * (strlen(user_home)+len+1));
        strcpy(yylval.string_val, user_home);
        strcat(yylval.string_val, &strtext[i]);

        return WORD;
        free(strtext);
		free(yylval.string_val);
    }
}

$\([^\"\n\t]*\) { 
	/* Process subshell arguments */
        int i;
		int len = strlen(yytext);
        char *strtext = (char *) malloc((len) * sizeof(char));
		/* In case the string associated with WORD token has quotes,
		   we process only text within the quotes. */
		strcpy(strtext, &yytext[2]);
        strtext[len - 3] = '\n';
		strtext[len - 2] = '\0';
        /* printf("%s", strtext); */
        strtext = Command::_currentCommand.subshell(strtext);
        strtext = str_replace("\n", " ", strtext);
        //yylval.string_val = (char *)malloc(strlen(strtext) * sizeof(char));
        //strcpy(yylval.string_val, &yylval.string_val[1]);
		//if (strlen(yylval.string_val) > 0)
		//	return WORD;
        for(i = strlen(strtext)-1; i >= 0; i--)
            myunputc(strtext[i]);
        free(strtext); 
		//free(yylval.string_val);
}

\"[^\"\n\t]*\"  { /* \"[^\"\n\t]*\" means that we
		/* Drop the quotes of quoted arguments and process them as
		   usual */
		int len = strlen(yytext);
		yylval.string_val = (char *) malloc((len - 1) * sizeof(char));
		/* In case the string associated with WORD token has quotes,
		   we process only text within the quotes. */
		strncpy(yylval.string_val, &yytext[1], len - 2);
		yylval.string_val[len - 2] = '\0';
		/* Now we can remove all escape characters like in case of usual WORD tokens
		   desribed below.  */
		removeEscapeChars(yylval.string_val);
		if (strlen(yylval.string_val) > 0)
			return WORD;
		free(yylval.string_val);
	}

\'[^\'\n\t]*\'  { /* \"[^\"\n\t]*\" means that we
		/* Drop the quotes of quoted arguments and process them as
		   usual */
		int len = strlen(yytext);
		yylval.string_val = (char *) malloc((len - 1) * sizeof(char));
		/* In case the string associated with WORD token has quotes,
		   we process only text within the quotes. */
		strncpy(yylval.string_val, &yytext[1], len - 2);
		yylval.string_val[len - 2] = '\0';
		/* Now we can remove all escape characters like in case of usual WORD tokens
		   desribed below.  */
		removeEscapeChars(yylval.string_val);
		if (strlen(yylval.string_val) > 0)
			return WORD;
		free(yylval.string_val);
	}

[^ \t\n\>\<\&\|](\\.|[^ \t\n\>\<\&\|])*  {
		/* Process usual arguments */
		yylval.string_val = strdup(yytext);
		/* We remove all escape characters of every WORD token using this line. */
		removeEscapeChars(yylval.string_val);
		if (strlen(yylval.string_val) > 0)
			return WORD;
		free(yylval.string_val);
	}

%%

/* Remove all escape characters from str and update it. */
void removeEscapeChars(char * str) {
	int len = strlen(str);
	/* processesed_command is a temprory string to store
	   non-escaped characters of the original string str. */
	char processesed_command[len + 1];
	int j = 0;
	/* len + 1 is the loop upper bound, because we also want
           to copy the null terminator of the string str. str[len + 1]
	   stores it. We want to store it to processesed_command to 
	   make it null-terminated string. */
	for(int i = 0; i < len + 1; i++) {
		if(str[i] != '\\') {
			/* In case we doesn't met an escaped character,
			   just copy every element of the original string str */
			processesed_command[j++] = str[i];
		} else {
			/* In case we met \ we should copy the following
			   character, because escaped characters come immediately after \ */
			processesed_command[j++] = str[i + 1];
			i++;
		}
	}
	/* Copy processesed_command to the original string str. And leave it. It will
	   be freed automatically, becuase it was created as an array of chars. */
	strcpy(str, processesed_command);
}
