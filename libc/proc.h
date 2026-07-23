#ifndef PROC_H
#define PROC_H

/* control de procesos */
void exit(int status);
int getpid(void);
void yield(void);
int spawn(const char *path, int argc, const char **argv);
int waitpid(int pid);

#endif
