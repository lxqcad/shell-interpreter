#define _GNU_SOURCE 
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static int
cmpstringp(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp(3) arguments are "pointers
       to char", hence the following cast plus dereference */

   return strcmp(* (char * const *) p1, * (char * const *) p2);
}

char *str_replace(const char *rep, const char *with, char *orig) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = (char *)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

int checkIfDir(struct dirent *de)
{
        int is_dir;
    #ifdef _DIRENT_HAVE_D_TYPE
        if (de->d_type != DT_UNKNOWN && de->d_type != DT_LNK) {
           // don't have to stat if we have d_type info, unless it's a symlink (since we stat, not lstat)
           is_dir = (de->d_type == DT_DIR);
        } else
    #endif
        {  // the only method if d_type isn't available,
           // otherwise this is a fallback for FSes where the kernel leaves it DT_UNKNOWN.
           struct stat stbuf;
           // stat follows symlinks, lstat doesn't.
           stat(de->d_name, &stbuf);              // TODO: error check
           is_dir = S_ISDIR(stbuf.st_mode);
        }

        return (is_dir);
}

char ** readdirents(char *dirName, char *regularExp, char **returnString, int *valcount, int bnReturnDirsOnly)
{
    struct dirent *pDirent;
    DIR *pDir;
    int bnAddDirName, count = *valcount;

    if(!strcmp(dirName, ".")) {
        bnAddDirName = 0;
    }
    else {
        bnAddDirName = strlen(dirName) + 1;
    }
     
	char *regExpTemp = (char *)malloc(strlen(regularExp)+3);
	sprintf(regExpTemp, "^%s$", regularExp );
    
	char *regExpComplete = str_replace(".", "\\.", regExpTemp) ;

	free(regExpTemp);
	regExpTemp = regExpComplete;
    regExpComplete = str_replace("*", ".*", regExpTemp) ;
	free(regExpTemp);
	regExpTemp = regExpComplete;
    regExpComplete = str_replace("?", "(.{1,1})", regExpTemp) ;
	free(regExpTemp);

	regex_t re;	

	if(regcomp( &re, regExpComplete,  REG_EXTENDED|REG_NOSUB) != 0) {
		fprintf( stderr, "Bad regular expresion \"%s\"\n", regExpComplete );
        free(regExpComplete);
		return( returnString );
    }

	regmatch_t match;

    pDir = opendir (dirName);
    if (pDir == NULL) {
        fprintf (stderr,"Cannot open directory '%s'\n", dirName);
        free(regExpComplete);
		return( returnString );
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        if(strcmp(regularExp, ".*") == 0) { 
            if(pDirent->d_name[0] != '.') continue;
        }
        else {
            if(pDirent->d_name[0] == '.') continue;
        }
        if(bnReturnDirsOnly) {
            if(!checkIfDir(pDirent)) continue;
        }
		if(regexec( &re, pDirent->d_name, 1, &match, 0 ) == 0) {
            returnString = (char **)realloc((char **)returnString, (count+2) * sizeof(char*));
            
        	returnString[count] = (char *)malloc(250); //(strlen(pDirent->d_name) + bnAddDirName) * sizeof(char)
            if(bnAddDirName) {
                strcpy(returnString[count], dirName);
                if(dirName[strlen(dirName)-1] != '/')
                    strcat(returnString[count], "/");
                strcat(returnString[count], pDirent->d_name);
            }
            else {
                strcpy(returnString[count], pDirent->d_name);
            }
            //printf("count = %d\n", count);
            count++;
        	returnString[count] = NULL;
        }
    }
        
	closedir (pDir);

	regfree(&re);
	free(regExpComplete);
    *valcount = count;
    return returnString;
}

char **process_wildcards(char *regularExp)
{
	char **returnString = NULL, **dirString = NULL, **exprStrings = NULL;
    int count = 0, expcount = 0, ret, len;
    char *strExp, *dirName, *lastString;
    
    len = strlen(regularExp);
	exprStrings = (char **)malloc(1 * sizeof(char *));
    exprStrings[0] = (char *)malloc(len * sizeof(char));
    expcount = 1;
    strcpy(exprStrings[0], regularExp);
    
    while(expcount > 0) {
        int i = 0, j = 0, k = 0, bnFound = 0;
        expcount--; // Pop entry from stack
        len = strlen(exprStrings[expcount]);
        char *str2process = exprStrings[expcount];
        while(i < len) {
            if(str2process[i] == '/') {
                if(bnFound) {
                    //j = i;
                    break;
                }
                k = i;
            }
            if(str2process[i] == '?' || str2process[i] == '*') {
                bnFound = 1;
            }
            i++;
        }
        //printf("Len = %d, i = %d, k = %d, bnFound = %d\n", len, i, k, bnFound);
        dirName = (char *) malloc (len * sizeof(char));
        strExp = (char *) malloc (len * sizeof(char));
        if(k == 0 && str2process[0] != '/') {
            strcpy(dirName, ".");
            strcpy(strExp, str2process);
        }
        else if(k == 0 && str2process[0] == '/') {
            strcpy(dirName, "/");
            strcpy(strExp, &str2process[k+1]);
            strExp[i-k-1] = '\0';
        }
        else {
            strcpy(dirName, str2process);
            dirName[k] = '\0';
            strcpy(strExp, &str2process[k+1]);
            strExp[i-k-1] = '\0';
        }
        if(i < len) {
            lastString = (char *)malloc((len-i+1) * sizeof(char));
            strcpy(lastString, &str2process[i]);
            //printf("dirName=%s, strExp=%s, LastString = %s\n", dirName, strExp, lastString);
            ret = 0;
            dirString = NULL;
            dirString = readdirents(dirName, strExp, dirString, &ret, 1);

            if(ret) {
                exprStrings = (char **)realloc((char **)exprStrings, (expcount+ret+1) * sizeof(char *));

                for(j = 0; j < ret; j++) {
                    exprStrings[expcount] = (char *)realloc((char *)exprStrings[expcount], ( strlen(dirString[j]) + (len-i+2) ) * sizeof(char));
                    strcpy(exprStrings[expcount], dirString[j]);
                    strcat(exprStrings[expcount], lastString);
                    free(dirString[j]);
                    expcount++;
                }
                free(dirString);
            }
            free(lastString);
        }
        else {
            //printf("dirName=%s, strExp=%s \n", dirName, strExp);
            ret = count;
            returnString = readdirents(dirName, strExp, returnString, &ret, 0);
            if(ret > count) count = ret;
            free(exprStrings[expcount]);
        }
        free(dirName);
        free(strExp);    
    }
   
   
    if(count == 0) {
        returnString = (char **)realloc(returnString, (2) * sizeof(char*));
        returnString[0] = (char *)malloc(strlen(regularExp) * sizeof(char));
        strcpy(returnString[0], regularExp);
        returnString[1] = NULL;
    }
    else {
        qsort(returnString, count, sizeof(char *), cmpstringp);
    }
    //free(orgString);
	return( returnString );
}

