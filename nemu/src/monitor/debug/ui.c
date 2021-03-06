#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){

	int step;
	if(args == NULL) step = 1;
	else sscanf(args,"%d", &step);
	cpu_exec(step);

	return 0;
}

static int cmd_info(char *args){
	if(args[0] == 'r'){
		int i;
		for(i = R_EAX; i <= R_EDI; i++){
			printf("$%s\t0x%08x\n", regsl[i], reg_l(i));
		}
	printf("$eip\t0x%08x\n", cpu.eip);
	}

	else if (!strcmp (args, "w")) {

		print_watchpoints();

	}

	else fprintf(stderr, "Unknown info command '%s'\n", args);
	return 0;
}

static int cmd_x(char *args){

	if(args == NULL){
	printf("Your commond is wrong.\n");
	return 0;
	}

	int num, exprs;
	sscanf(args, "%d%x", &num, &exprs);
	
	int i;
	for(i = 0; i < num; i++){
		printf("0x%8x 0x%x\n", exprs + i*5, swaddr_read(exprs + i*5, 4));
	}
	return 0;
}

static int cmd_p (char *args) {
	bool success;
	int i = expr(args, &success);
	printf ("%d\n", i);
	return 0;	
}

static int cmd_w (char *args) {
	if (strlen(args) > MAX_LENGTH_OF_EXPR) {
		fprintf(stderr, "Expression is too long\n");
		return 0;
	}
	bool success;
	uint32_t res = expr(args, &success);

	if (!success) return 0;

	WP *wp = new_wp();
	strcpy (wp -> expr, args);
	wp -> value = res;

	printf ("Watchpoint %d: %s\n", wp -> NO, wp -> expr);
		return 0;

}

static int cmd_d (char *args) {
	//extract the first argument
	char *arg = strtok (NULL, " ");

	if (arg == NULL) {
		//no given arguments
		fprintf (stderr, "Need a number. Try 'help d'\n");
	}

	else {
		int number = atoi(arg);
		free_wp(number);
	}
	return 0;

}
static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Single-step execution", cmd_si},
	{"info", "Print program status", cmd_info},
	{"x", "Scan the memory", cmd_x},
	{"p", "Print value of expression EXPR", cmd_p},
	{"w", "Set an watchpoint for an expression", cmd_w},
	{"d", "Delete a watchpoint", cmd_d}
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}



void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
