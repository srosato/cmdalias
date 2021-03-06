%{

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>


#include "cmdalias.h"
#include "lexer.h"

#define YYERROR_VERBOSE
#define	YYPARSE_PARAM commands

void yyerror(const char *s, ...) {
	extern int yylineno;

	va_list ap;
	va_start(ap, s);

	fprintf(stderr, "Error: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, " in '%s' on line %d\n", config_get_current_filename(), yylineno);
	fflush(stderr);
}

static int is_dir(const char *path) {
	struct stat st;

	if (lstat(path, &st) == -1) {
		return -1;
	}

	return S_ISDIR(st.st_mode);
}

static int config_pushdir(const char *dirname) {
	struct dirent *dent;
	DIR *dir;
	char fn[FILENAME_MAX];
	int len = strlen(dirname);

	if (len >= FILENAME_MAX - 1) {
		/* Name too long */
		return 0;
	}

	strcpy(fn, dirname);
	fn[len++] = '/';

	if (!(dir = opendir(dirname))) {
		/* Can't open directory */
		return 0;
	}

	while ((dent = readdir(dir))) {
		if (dent->d_name[0] == '.') continue;
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;

		strncpy(fn + len, dent->d_name, FILENAME_MAX - len);

		if (is_dir(fn)) {
			config_pushdir(fn);
			continue;
		}

		config_pushfile(fn);
	}

	if (dir) closedir(dir);

	return 1;
}

static void rtrim(char* s) {
	int len = strlen(s);
	if (len > 0) {
		char *pos = s + len - 1;
		while(pos >= s && isspace(*pos)) {
			*pos = '\0';
			pos--;
		}
	}
}

%}

%union {
	char *str;
	struct string_list_t *str_list;
	struct alias_t *alias;
	struct alias_list_t *alias_list;
	struct command_t *cmd;
	struct command_list_t *cmd_list;
	cmdalias_bool mbool;
}

%token <str>  T_NAME "Name (T_NAME)"
%token <str>  T_STR  "String (T_STR)"
%token <str>  T_CMD  "Command (T_CMD)"
%token T_INCLUDE     "Include (T_INCLUDE)"

%type <str> string_or_subcmd
%type <str_list> string_list_or_subcmd alias_name_list
%type <alias> alias
%type <alias_list> global_alias_list_or_empty alias_list_or_empty alias_list
%type <mbool> is_cmd

%%

command_list_or_empty:
		command_list
	|	/* empty */
;

command_list:
		command_list command
	|	command_list include
	|	command
	|	include
;

include:
		T_INCLUDE T_STR ';' {
			if (!(is_dir($2) ? config_pushdir($2) : config_pushfile($2))) {
				yyerror("Unable to load %s", $2);
			}
			free($2);
		}
;

command:
		alias_name_list '=' T_NAME '{' global_alias_list_or_empty alias_list_or_empty '}' end {
			command_list **cmds = (command_list **) commands;
			command *cmd = (command *) malloc(sizeof(command));
			cmd->name = $3;
			cmd->name_aliases = $1;
			cmd->global = $5;
			cmd->aliases = $6;

			*cmds = command_list_append(*cmds, cmd);
		}
	|	T_NAME '{' global_alias_list_or_empty alias_list_or_empty '}' end {
			command_list **cmds = (command_list **) commands;
			command *cmd = (command *) malloc(sizeof(command));
			cmd->name = $1;
			cmd->name_aliases = NULL;
			cmd->global = $3;
			cmd->aliases = $4;

			*cmds = command_list_append(*cmds, cmd);
		}
	|	alias_name_list '=' T_NAME ';' {
			command_list **cmds = (command_list **) commands;
			command *cmd = (command *) malloc(sizeof(command));
			cmd->name = $3;
			cmd->name_aliases = $1;
			cmd->global = NULL;
			cmd->aliases = NULL;

			*cmds = command_list_append(*cmds, cmd);
		}
;

global_alias_list_or_empty:
		'*' '{' alias_list_or_empty '}' { $$ = $3; }
	|	/* empty */ { $$ = NULL; }
;

alias_list_or_empty:
		alias_list { $$ = $1; }
	|	/* empty */ { $$ = NULL; }
;

alias_list:
		alias_list alias { $$ = alias_list_append($1, $2); }
	|	alias { $$ = alias_list_append(NULL, $1); }
;

alias:
		alias_name_list '=' is_cmd string_list_or_subcmd ';' {
			$$ = (alias *) malloc(sizeof(alias));
			$$->names		= $1;
			$$->is_cmd		= $3;
			$$->substitutes = $4;
			$$->subaliases  = NULL;
		}
	|	alias_name_list '=' is_cmd string_list_or_subcmd '{' alias_list_or_empty '}' end {
			$$ = (alias *) malloc(sizeof(alias));
			$$->names		= $1;
			$$->is_cmd		= $3;
			$$->substitutes = $4;
			$$->subaliases  = $6;
		}
	|  T_NAME '{' alias_list_or_empty '}' end {
			$$ = (alias *) malloc(sizeof(alias));
			$$->names 		= string_list_append(NULL, $1);
			$$->is_cmd		= 0;
			$$->substitutes = string_list_append(NULL, $1);
			$$->subaliases  = $3;
		}

;

alias_name_list:
		alias_name_list ',' T_NAME { $$ = string_list_append($1, $3); }
	|	T_NAME { $$ = string_list_append(NULL, $1); }
;

is_cmd:
		'!' { $$ = 1; }
	|	/* empty */ { $$ = 0; }
;

string_list_or_subcmd:
		string_list_or_subcmd string_or_subcmd { $$ = string_list_append($1, $2); }
	|	string_or_subcmd { $$ = string_list_append(NULL, $1); }
;

string_or_subcmd:
		T_STR
	|	T_NAME
	|	T_CMD {
	FILE *fp;
	char res[1035];

	fp = popen($1, "r");
	if (fp == NULL) {
		exit(EXIT_FAILURE);
	}

	if (fgets(res, sizeof(res)-1, fp) == NULL) {
		yyerror("Error while fetching result\n");
	}

	pclose(fp);

	rtrim(res);
	$$ = strdup(res);
}
;

end:
		';'
	|	/* empty */
;

%%

int config_load(const char *path, command_list **commands) {

	if (!path) {
		char buffer[255];
		snprintf(buffer, 255, "%s/.cmdalias", getenv("HOME"));
		path = buffer;
	}

	*commands = NULL;

	if (!(is_dir(path) ? config_pushdir(path) : config_pushfile(path))) {
		return 0;
	}

	if (yyparse(commands)) {
		return 0;
	}

	return 1;
}
