# shell-interpreter
A complete Linux Shell Interpreter which parses commands, handles background processes, pipes and sub shells.

The code includes a scanner and parser for the shell developed using the open source versions of Lex and Yacc (Flex and Bison).
The shell handles CTRL-C command like common shells do. You can use output of one shell command as input of another (piping).
It has a complete line editor with history of previously typed commands. It implements the "cd" command as in normal linux shells.
**Features:**
  1. IO Redirection
  2. Pipes
  3. Background Processes
  4. Zombie Processes
  5. Builtin Functions
  6. CD
  7. Extended Parsing
  8. Subshell
  9. Environment Variable Expansion
  10. Wildcards
  11. Tilde Expansion
