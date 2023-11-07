[system programming lecture]

When you enter "make" in Linux, an executable file called "myShell" will be generated. You can run MyShell by entering "./myShell".

- Phase 1
I implemented some basic shell commands. All commands except for cd, history, and exit are implemented using fork() and Execve() function. I also implemented "!!" and "!#", the functions of history. I used parse function to split the input by space.

- Phase 2
I implemented the pipeline "|" command. With phase 1 concepts and commands, I used pipe() and Dup2() function to implement multiple pipelines. I used parse function to not take into account "'" and """ characters.

- Phase 3
I implemented the "&" command at the end of input command line to run commands at the background. And I implemented jobs, bg, fg, and kill command to handle background jobs. And, I implemented SIGTSTP with sigtstp_handler to stop the foreground process and change it to background process.

There are more details in document.docx
