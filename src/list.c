#include <stdlib.h>
#include "cmdalias.h"

static void free_alias(alias *a)
{
	string_list_free_all(a->names);
	string_list_free_all(a->substitutes);
	alias_list_free_all(a->subaliases);
	free(a);
}

static void free_command(command *cmd)
{
	free(cmd->name);
	string_list_free_all(cmd->name_aliases);
	alias_list_free_all(cmd->global);
	alias_list_free_all(cmd->aliases);
	free(cmd);
}
static string_list *string_list_last(string_list *list) {
	while(list && list->next) {
		list = list->next;
	}

	return list;
}

string_list *string_list_append(string_list *list, char *data) {
	string_list *last = string_list_last(list);

	string_list *new_item = (string_list *) malloc(sizeof(string_list));
	new_item->data = data;
	new_item->next = NULL;

	if (last) {
		last->next = new_item;
		return list;
	}

	return new_item;
}

void string_list_free_all(string_list *list) {
	string_list *next, *item;

	if(!list) return;

	item = list;
	do {
		next = item->next;
		free(item->data);
		free(item);
		item = next;
	} while(next);
}

alias_list *alias_list_append(alias_list *list, alias *a) {
	alias_list *new_item = (alias_list *) malloc(sizeof(alias_list));

	new_item->alias = a;
	new_item->next  = list;

	return new_item;
}

void alias_list_free_all(alias_list *list) {
	alias_list *next, *item;

	if(!list) return;

	item = list;
	do {
		next = item->next;
		free_alias(item->alias);
		free(item);
		item = next;
	} while(next);
}

command_list *command_list_append(command_list *list, command *cmd) {
	command_list *new_item = (command_list *) malloc(sizeof(command_list));

	new_item->command = cmd;
	new_item->next    = list;
	return new_item;
}

void command_list_free_all(command_list *list) {
	command_list *next, *item;

	if(!list) return;

	item = list;
	do {
		next = item->next;
		free_command(item->command);
		free(item);
		item = next;
	} while(next);
}
