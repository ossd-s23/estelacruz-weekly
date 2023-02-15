#ifndef _ARGMANIP_H_
#define _ARGMANIP_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>

char **manipulate_args(int argc, const char *const *argv, int (*const manip)(int));

void free_copied_args(char **args, ...);

#endif
