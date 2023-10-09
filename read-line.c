/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 1024
#define MAX_HISTORY_LENGTH 8

extern void tty_raw_mode(void);
extern void tty_normal_mode(void);
extern char **process_wildcards(char *regularExp);

// Buffer where line is stored
int line_length, cursor_pos;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
int history_index = 0;
char history[MAX_HISTORY_LENGTH][MAX_BUFFER_LINE];
int history_length = 0; // = sizeof(history) / sizeof(char *);

void print_history(void)
{
    int i;
    for(i = 0;i < history_length; i++)
        fprintf(stdout,"%s\n",history[i]);
}

void read_line_print_usage()
{
    char *usage = "\n"
                  " ctrl-?       Print usage.\n"
                  " Backspace    Removes the character at the position before the cursor.\n"
                  " Delete       Deletes character at the cursor position.\n"
                  " Home         Move the cursor to the beginning of the line.\n"
                  " End          Move the cursor to the end of the line.\n"
                  " Left arrow   Move the cursor to the left and allow insertion at that position.\n"
                  " Right arrow  Move the cursor to the right and allow insertion at that position..\n"
                  " Up arrow     See last command in the history.\n"
                  " Down arrow   See next command in the history.\n"
                  " Tab key      Find files beginning with text at the cursor.\n"
                  " history      Print all commands in the history.\n";

    write(1, usage, strlen(usage));
}

/*
 * Input a line with some basic editing.
 */
void shiftone_up()
{
    int i;
    for(i = 0; i < MAX_HISTORY_LENGTH-1; i++)
        strcpy(history[i], history[i+1]);
}

void find_common_factor(char *str1, char *comm)
{
    int i = 0;
    while(str1[i] != 0 && comm[i] != 0 && str1[i] == comm[i]) i++;
    comm[i] = '\0';
}

void expand_tab()
{
    int i = cursor_pos, c = 0;
    char reg_ex[MAX_BUFFER_LINE], str_com[MAX_BUFFER_LINE];
    char **result;

    reg_ex[0] = '\0';

    while(i > 0) {
        if(line_buffer[i] == '\'' || line_buffer[i] == '\"' || line_buffer[i] == ' ')
            break;
        i--;
    }
    if(i > 0) {
        i++;
    }
    strcpy(reg_ex, &line_buffer[i]);
    strcat(reg_ex, "*");
    
    result = process_wildcards(reg_ex);
    str_com[0] = '\0';
    if(result != NULL) {
        if(result[c] != NULL) {
            strcpy(str_com, result[c]);
            c++;
        }
        while(result[c] != NULL) {
            find_common_factor(result[c], str_com);
            c++;
        }
        if(cursor_pos == line_length && str_com[0] != 0) {
            strcpy(&line_buffer[i], str_com);

            for(c = 0; c < line_length-i; c++)
                putc(8, stdout);
            printf("%s", str_com);
            //write(1, &str_com[0], strlen(str_com)-1);
            line_length = strlen(line_buffer);
            cursor_pos = line_length;
        }
    }
}
 
char *read_line()
{
    // Set terminal in raw mode
    char ch, ch1, ch2;
    int i;
    tty_raw_mode();

    line_length = 0;
	cursor_pos = 0;

    // Read one line until enter is typed
    while(1) {
        // Read one character in raw mode.
        ch = getc(stdin);
        // read(0, &ch, 1);

        if(ch == 10) {
            // <Enter> was typed. Return line

            // Print newline
            write(1, &ch, 1);

            break;
        } else if(ch == 31) {
            // ctrl-?
            strcpy(line_buffer, "help");
            line_length = 4;
            break;
        } else if(ch == 8 || ch == 127) {
            // <backspace> Remove previous character in buffer.

            if(cursor_pos > 0) {
                if(cursor_pos < line_length) {
                    strcpy(&line_buffer[cursor_pos-1], &line_buffer[cursor_pos]);
                }

                // Go back one character
                ch = 8;
                write(1, &ch, 1);

                // Do echo
                write(1, &line_buffer[cursor_pos-1], line_length-cursor_pos);

                // Write a space to erase the last character read
                ch = ' ';
                write(1, &ch, 1);

                for(i = 0; i <= line_length-cursor_pos; i++)
                    putc(8, stdout);

                // Go back one character
                //ch = 8;
                //write(1, &ch, 1);

                // Remove one character from buffer
                line_length--;
                cursor_pos--;
                line_buffer[line_length] = 0;
            }
        } else if(ch == 9) {
            // TAB Key
            expand_tab();
        } else if(ch == 27) {
            // Escape sequence. Read two chars more
            // read(0, &ch1, 1);
            // read(0, &ch2, 1);
            ch1 = getc(stdin);
            ch2 = getc(stdin);
            if(ch1 == 91) { 
                if(ch2 == 65) {
                    // Up arrow. Print previous command line in history.
                    if(history_index > 0) {
                        // Erase old line
                        // Print backspaces
                        int i = 0;
                        for(i = 0; i < line_length; i++) {
                            ch = 8;
                            write(1, &ch, 1);
                        }

                        // Print spaces on top
                        for(i = 0; i < line_length; i++) {
                            ch = ' ';
                            write(1, &ch, 1);
                        }

                        // Print backspaces
                        for(i = 0; i < line_length; i++) {
                            ch = 8;
                            write(1, &ch, 1);
                        }

                        // Copy line from history
                        history_index--;
                        strcpy(line_buffer, history[history_index]);
                        line_length = strlen(line_buffer);
                        cursor_pos = line_length;

                        // echo line
                        write(1, line_buffer, line_length);
                        
                    }
                }

                else if(ch2 == 66) {
                    // Down arrow. Print next command line in history.
                    if(history_index < history_length) {
                        // Erase old line
                        // Print backspaces
                        int i = 0;
                        for(i = 0; i < line_length; i++) {
                            ch = 8;
                            write(1, &ch, 1);
                        }

                        // Print spaces on top
                        for(i = 0; i < line_length; i++) {
                            ch = ' ';
                            write(1, &ch, 1);
                        }

                        // Print backspaces
                        for(i = 0; i < line_length; i++) {
                            ch = 8;
                            write(1, &ch, 1);
                        }

                        // Copy line from history
                        history_index++;
                        strcpy(line_buffer, history[history_index]);
                        line_length = strlen(line_buffer);
                        cursor_pos = line_length;

                        // echo line
                        write(1, line_buffer, line_length);
                    }
                }
                else if(ch2 == 68) {
                    // Left Arrow Key pressed
                    if(cursor_pos > 0) {
                        cursor_pos--;
                        ch = 8;
                        write(1, &ch, 1);
                    }
                }
                else if(ch2 == 67) {
                    // Right Arrow key pressed
                    if(cursor_pos < line_length) {
                        ch = line_buffer[cursor_pos];
                        write(1, &ch, 1);
                        cursor_pos++;
                    }
                }
                else if(ch2 == 72) {
                    // Home Key
                    if(cursor_pos > 0) {
                        for(i = 0; i < cursor_pos; i++)
                            putc(8, stdout);
                        
                        cursor_pos = 0;
                    }
                }
                else if(ch2 == 70) {
                    // End Key
                    if(cursor_pos < line_length) {
                        write(1, &line_buffer[cursor_pos], line_length-cursor_pos);
                        cursor_pos = line_length;
                    }
                }
                else if(ch2 == 51) {
                    ch2 = getc(stdin);
                    // Delete key pressed
                    if(cursor_pos < line_length) {
                        strcpy(&line_buffer[cursor_pos], &line_buffer[cursor_pos+1]);

                        // Do echo
                        write(1, &line_buffer[cursor_pos], line_length-cursor_pos);

                        // Write a space to erase the last character read
                        ch = ' ';
                        write(1, &ch, 1);

                        for(i = 0; i < line_length-cursor_pos; i++)
                            putc(8, stdout);

                        // Remove one character from buffer
                        line_length--;
                        line_buffer[line_length] = 0;
                    }
                }
            }   

        } else if(ch >= 32) {
            // If max number of character reached return.
            if(line_length == MAX_BUFFER_LINE - 2)
                break;

            // add char to buffer.
            if(cursor_pos < line_length) {
                strcpy(&line_buffer[cursor_pos+1], &line_buffer[cursor_pos]);
            }
            line_buffer[cursor_pos] = ch;

            // Do echo
            write(1, &line_buffer[cursor_pos], line_length-cursor_pos+1);
            for(i = 0; i < line_length-cursor_pos; i++)
                putc(8, stdout);
            line_length++;
            cursor_pos++;
            line_buffer[line_length] = 0;
        }
    }

    // Copy history to last available location or restart index from 0
    line_buffer[line_length] = 0;
    strcpy(history[history_length], line_buffer);
    if(history_length < MAX_HISTORY_LENGTH-1)
        history_length++;
    else  
        shiftone_up();

    history[history_length][0] = 0;
    history_index = history_length;
    
    // Add eol and null char at the end of string
    line_buffer[line_length] = 10;
    line_length++;
    line_buffer[line_length] = 0;

    tty_normal_mode();
    return line_buffer;
}
