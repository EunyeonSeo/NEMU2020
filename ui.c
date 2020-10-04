static int cmd_si(char *args){
	int step;
	if(args == NULL) step = 1;
	else sscanf(args, "%d", &step);
	cpu_exec(step);
	return 0;
}
