# Shell

[Description]

## Group Members
- **Hubert Pilichowski**: hp22q@fsu.edu
- **Owen Proth**: oop23@fsu.edu
- **Zidan Waite**: zgw21@fsu.edu
## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Print the shell prompt in the format user@machine:currentDirectory
- **Assigned to**: Hubert Pilichowski

### Part 2: Environment Variables
- **Responsibilities**: Replace environment variables (e.g., $VAR) in the given token string
- **Assigned to**: Zidan Waite

### Part 3: Tilde Expansion
- **Responsibilities**: Replace tilde expansion in the given token string
- **Assigned to**: Zidan Waite

### Part 4: $PATH Search
- **Responsibilities**: Resolving command (PATH or direct).
- **Assigned to**: Hubert Pilichowski

### Part 5: External Command Execution
- **Responsibilities**: Executing the command from the previous part
- **Assigned to**: Hubert Pilichowski

### Part 6: I/O Redirection
- **Responsibilities**: Parse < and > out of tokens; build argv.
- **Assigned to**: Owen Proth

### Part 7: Piping
- **Responsibilities**: Execute a pipeline of commands connected by pipes, using temporary files for intermediate stages.
- **Assigned to**: Owen Proth

### Part 8: Background Processing
- **Responsibilities**: Find the & sumbol at the end of the user input and run the process in the background
- **Assigned to**: Hubert Pilichowski

### Part 9: Internal Command Execution
- **Responsibilities**: [Description]
- **Assigned to**: Zidan Waite

### Extra Credit
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown

## File Listing
```
os-shell_project1/
├── bin/
├── include/
│   ├── background.h
│   ├── environmentVariables.h
│   ├── lexer.h
│   ├── pathSearch.h
│   ├── pipeline.h
│   ├── prompt.h
│   └── redirection.h
├── obj/
├── src/
│   ├── background.c
│   ├── environmentVariables.c
│   ├── lexer.c
│   ├── main.c
│   ├── pathSearch.c
│   ├── pipeline.c
│   ├── prompt.c
│   └── redirection.c
├── .gitignore
├── Makefile
└── README.md
```
## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Hubert Pilichowski

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-19 | Worked out the logic for parts 1 and 2 |
| 2025-09-21 | Worked on parts 4 and 5 exectables |
| 2025-09-23 | Did part 8 background processes |
| 2025-09-25 | Documentation and formatting |

### Owen Proth

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-19 | Worked out the logic for parts 1 and 2 |
| 2025-09-23 | Did part 6 and 7 i/o redirection |
| 2025-09-25 | Documentation and pipeline reformatting |


### Zidan Waite

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-19 | Worked on part 3  |
| 2025-09-23 | Helped work on parts 4,5,6,7 |
| 2025-09-25 | Worked on Part 9 |
| 2025-09-25 | Documentation and formatting |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| 2025-09-19 | Hubert Pilichowski, Owen Proth, Zidan Waite | Start Project    | Finished parts 1 and 2/work further into the project  |
| 2025-09-23 | Hubert Pilichowski, Owen Proth, Zidan Waite | Get up to part 5 | Finished 3, work further into 6 and 7  |
| 2025-09-23 | Hubert Pilichowski, Owen Proth, Zidan Waite | Get up to part 8 | Finished 6,7/ document everything and begin testing  |



## Bugs // no current bugs that we know of
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Extra Credit
- **Extra Credit 1**: Finished supporting an unlimited number of pipes
- **Extra Credit 2**: Finished support for i/o redirection and piping in the same command
- **Extra Credit 3**: Supports shellception

## Considerations

