#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HIST_MAX 100
#define ARG_MAX 1000

/**
This struct holds all of the information for each command 

cmd_id: The ID for the command, in this case the order in which it appeared
run_time: How long it took to execute the command
command: The String containing the command that was entered
*/
struct history_entry {
    unsigned long cmd_id;
    double run_time;
    char command[ARG_MAX];
};

void print_history();
void addHistory(int cmd_id, double run_time, char* command);
char* getCommand(int cmd_id);
char* getCommandPre(char* pre);

#endif
