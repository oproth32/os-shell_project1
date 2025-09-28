# Shell Manual Testing Guide

This guide walks through manual tests for each part of the shell project. For each part, you will see a sequence of scenarios followed by an error scenario.

## Part 1: Prompt
Objective: Print the shell prompt in the format `user@machine:currentDirectory> `
1. Open the shell and check that the prompt displays correctly.
2. Change directories using `cd /tmp`.
3. Change back to home using `cd`.
4. Navigate to a subdirectory inside the project folder using `cd project-1-group-10`.
5. Use `pwd` to check the current directory.
Error Scenario: Press Enter without typing a command to ensure shell does not crash.

## Part 2: Environment Variables
Objective: Replace environment variables (e.g., `$VAR`) in the token string.
1. Print the value of `$HOME`.
2. Change directory using `cd $HOME`.
3. Combine variable with a command: `echo $HOME`.
4. Set a new environment variable: `export TESTVAR=123`.
5. Print the new variable: `echo $TESTVAR`.
Error Scenario: Try an undefined variable with `echo $UNDEFINED_VAR`.

## Part 3: Tilde Expansion
Objective: Replace `~` with the user’s home directory.
1. Go to home using `cd ~`.
2. Navigate to a subdirectory using `cd ~/project-1-group-10`.
3. Use `echo ~` to print home directory.
4. Combine tilde with a subdirectory: `cd ~/project-1-group-10/src`.
5. Check current directory with `pwd`.
Error Scenario: Use a non-existing tilde path: `cd ~/nonexistent`.

## Part 4: PATH Search
Objective: Resolve commands using PATH or direct input.
1. Run `ls`.
2. Run `/bin/echo hello`.
3. Use `which ls` to check command path.
4. Run `pwd`.
5. Run `date`.
Error Scenario: Run a non-existing command `fakecmd`.

## Part 5: External Command Execution
Objective: Execute commands resolved from PATH.
1. List directory contents with `ls`.
2. Print working directory with `pwd`.
3. Show first 3 lines of a file using `head -n 3 filename`.
4. Count words in a file: `wc filename`.
5. Use `cat filename | grep something`.
Error Scenario: Execute invalid command `invalidcmd`.

## Part 6: I/O Redirection
Objective: Parse `<` and `>` and redirect input/output.
1. Write output to a file: `echo hello > testfile.txt`.
2. Read from a file: `cat < testfile.txt`.
3. Append to a file: `echo world >> testfile.txt`.
4. Redirect output to a new file: `ls > files.txt`.
5. Read redirected file: `cat files.txt`.
Error Scenario: Redirect from a missing file: `cat < nonexistent.txt`.

## Part 7: Piping
Objective: Execute a pipeline of commands using temporary files.
1. Echo and pipe to grep: `echo hello | grep h`.
2. Pipe multiple commands: `ls | grep .c`.
3. Use `cat` and `grep`: `cat testfile.txt | grep hello`.
4. Combine `ls` and `wc`: `ls | wc -l`.
5. Chain three commands: `echo hello | grep h | wc -c`.
Error Scenario: Pipe to invalid command: `echo test | fakecmd`.

## Part 8: Background Processing
Objective: Run a process in the background using `&`.
1. Sleep in background: `sleep 1 &`.
2. Run echo in background: `echo hello &`.
3. List background jobs using `jobs`.
4. Run multiple commands in background: `sleep 2 &; sleep 3 &`.
5. Check all background jobs with `jobs`.
Error Scenario: Run invalid background command: `fakecmd &`.

## Part 9: Internal Command Execution
Objective: Execute internal shell commands like `cd`, `history`, `exit`.
1. Change directory internally: `cd /tmp`.
2. Run `history` to see past commands.
3. Use `cd` to return to home.
4. Print working directory with `pwd`.
5. Run `exit` to close the shell.
Error Scenario: Run unknown internal command: `unknowninternal`.
