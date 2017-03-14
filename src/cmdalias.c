#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>

#include "cmdalias.h"
#include "lexer.h"

static void display_usage() {
	puts("Usage: ./cmdalias [OPTION] -- <command> [args...] ");
	puts("  -c, --config=CONF       Configuration file or directory (default: ~/.cmdalias)");
	puts("  -i, --init              Init");
	puts("  -h, --help              Display this help");
	puts("  -V, --version           Display version");
	puts("      --check-config      Check the configuration file");
	puts("");
	exit(EXIT_FAILURE);
}

static void display_version() {
	puts("CmdAlias " CMDALIAS_VERSION " (c)2017 Adoy.net");
	exit(EXIT_FAILURE);
}

static void check_config(const char *configFile) {
	int exit_status;
	cmdalias_config config;
	cmdalias_config_init(&config);
	if (cmdalias_config_load(configFile, &config)) {
		puts("Syntax OK");
		exit_status = EXIT_SUCCESS;
	} else {
		exit_status = EXIT_FAILURE;
	}
	cmdalias_config_destroy(&config);
	exit(exit_status);
}

static void cmdalias_bash_init(const char *configFile) {
	int exit_status;
	cmdalias_config config;
	cmdalias_config_init(&config);
	if (cmdalias_config_load(configFile, &config)) {
		command_list *cmd_l = config.commands;
		string_list *name_aliases;
		while (cmd_l) {
			if (configFile) {
				fprintf(stdout, "alias %s=\"cmdalias -c %s -- %s\";\n", cmd_l->command->name, configFile, cmd_l->command->name);
			} else {
				fprintf(stdout, "alias %s=\"cmdalias -- %s\";\n", cmd_l->command->name, cmd_l->command->name);
			}
			name_aliases = cmd_l->command->name_aliases;
			while (name_aliases) {
				if (configFile) {
					fprintf(stdout, "alias %s=\"cmdalias -c %s -- %s\";\n", name_aliases->data, configFile, cmd_l->command->name);
				} else {
					fprintf(stdout, "alias %s=\"cmdalias -- %s\";\n", name_aliases->data, cmd_l->command->name);
				}
				name_aliases = name_aliases->next;
			}
			cmd_l = cmd_l->next;
		}
		exit_status = EXIT_SUCCESS;
	} else {
		exit_status = EXIT_FAILURE;
	}
	cmdalias_config_destroy(&config);
	exit(exit_status);
}

static int cmdalias(const char *configFile, int argc, char **argv) {
	int exit_status;
	cmdalias_config config;
	cmdalias_config_init(&config);
	if (cmdalias_config_load(configFile, &config)) {

		if (0 == argc) {
			cmdalias_config_destroy(&config);
			display_usage();
		}

		exit_status = argc == 1 ? execvp(argv[0], argv) : alias_execute(&config, argc, argv);
	} else {
		exit_status = EXIT_FAILURE;
	}
	cmdalias_config_destroy(&config);
	return exit_status;
}

int main(int argc, char **argv)
{
	int longIndex, opt;
	static const char *optString = "c:h?Vi";
	static const struct option longOpts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V'},
		{ "config", required_argument, NULL, 'c' },
		{ "check-config", no_argument, NULL, 0 },
		{ "init", no_argument, NULL, 0 },
		{ NULL, no_argument, NULL, 0 }
	};

	struct {
		char *config_file; /* -c */
		int check_config;  /* --check-config */
		int init; /* --init */
	} cmdalias_args;

	cmdalias_args.config_file  = NULL;
	cmdalias_args.check_config = 0;
	cmdalias_args.init         = 0;

	opt = getopt_long(argc, argv, optString, longOpts, &longIndex);

	while ( opt != -1 ) {
		switch ( opt ) {
			case 'c':
				cmdalias_args.config_file = optarg;
				break;
			case 'h':   /* fall-through is intentional */
			case '?':
				display_usage();
				break;
			case 'i':
				cmdalias_args.init = 1;
				break;
			case 'V':
				display_version();
				break;
			case 0:
				if (strcmp("check-config", longOpts[longIndex].name) == 0) {
					cmdalias_args.check_config = 1;
				} else if (strcmp("init", longOpts[longIndex].name) == 0) {
					cmdalias_args.init = 1;
				}
				break;
			default:
				/* You won't actually get here. */
				break;
		}

		opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
	}

	if (cmdalias_args.check_config) {
		check_config(cmdalias_args.config_file);
	}

	if (cmdalias_args.init) {
		cmdalias_bash_init(cmdalias_args.config_file);
	}

	exit(cmdalias(cmdalias_args.config_file, argc - optind, argv + optind));
}
