#ifndef _CMDALIAS_H_
#define _CMDALIAS_H_

#include "config.h"

typedef char cmdalias_bool;

typedef struct string_list_t {
	char *data;
	struct string_list_t *next;
} string_list;

typedef struct alias_t {
	string_list *names;
	cmdalias_bool is_cmd;
	string_list *substitutes;
	struct alias_list_t *subaliases;
} alias;

typedef struct alias_list_t {
	struct alias_t *alias;
	struct alias_list_t *next;
} alias_list;

typedef struct command_t {
	char *name;
	string_list *name_aliases;
	alias_list *global;
	alias_list *aliases;
} command;

typedef struct command_list_t {
	struct command_t *command;
	struct command_list_t *next;
} command_list;


string_list *string_list_append(string_list *, char *);
void string_list_free_all(string_list *);

alias_list *alias_list_append(alias_list *, alias *);
void alias_list_free_all(alias_list *);

command_list *command_list_append(command_list *, command *);
void command_list_free_all(command_list *);

int alias_execute(command_list *, int, char **);

#ifdef CMDALIAS_DEBUG
#include <stdio.h>
#define debug_msg(A, ... ) printf(A, ##__VA_ARGS__)
#else
#define debug_msg(A, ... )
#endif /* DEBUG */

#endif /* _SLIST_H_ */
