#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//this is gonna be a circular array
int start = 0; //points to start of information
int end = 0; //points to lastHistory + 1 aka the next position to insert 
struct history_entry *entries[HIST_MAX] = {0};

/**
This function simply prints the history, goes from start to end on the circular array
*/
void print_history() {
    /* This function should print history entries */
    int i = 0;
    int pos;
    struct history_entry *target;
    for (; i < HIST_MAX; ++i){
        //ensure that the pos is in the right position in the circular array
        pos = (start + i) % HIST_MAX;
        //makes sure that we don't go over the end point 
        if (pos == end) {
            break;
        }

        target = entries[pos];
        //if it's null then we're out of history and we break
        if (target == NULL) {
            break;
        }
        printf("[%lu|%f] %s", target->cmd_id, target->run_time, target->command);
        fflush(stdin);
    }
}

/**
Adds a new history struct to the history of the shell and updates the pointers when needed

cmd_id: The number of the command that was given
run_time: How long it took for the command to execute
command: The string which contains the command that was entered by the user
*/
void addHistory(int cmd_id, double run_time, char* command) {
    struct history_entry *entry = calloc(1, sizeof(struct history_entry));
    entry->cmd_id = cmd_id;
    entry->run_time = run_time;
    strcpy(entry->command, command);

    entries[end] = entry;
    end++;

    if (end == HIST_MAX) {
        end = 0;
    }

    if (end == start) {
        start++;
        if (start == HIST_MAX) {
            start = 0;
        }
    }
}

/**
Searches through the circular array and finds the latest command that is prefixed by the given string

pre: The prefix that all commands in the array will be compared against

return: A String containing the latest command prefixed by the given string or NULL if no match is found
*/
char* getCommandPre(char* pre) {
    //make sure that there is history in the array 
    if (end == 0) {
        return NULL;
    }

    char* result = NULL;
    int i = 0;
    int pos;
    struct history_entry *target;
    for (; i < HIST_MAX; ++i) {
        pos = (start + i) % HIST_MAX;
        if (pos == end) {
            break;
        }

        target = entries[pos];
        if (target == NULL) {
            break;
        }
        if (strncmp(target->command, pre, strlen(pre)) == 0) {
            result = target->command;
        }
    }

    return result;
}

/**
Searches through the circular array for a history struct that has the same ID as the one given

cmd_id: The ID that is being searched for

return: String containing command that matches the ID given or NULL if no match is found
*/
char* getCommand(int cmd_id) {
    if (end == 0) {
        return NULL;
    }

    char* result = NULL;
    if (cmd_id == -1) {
        result = entries[end - 1]->command;
        return result;
    }

    int i = 0;
    int pos;
    struct history_entry *target;
    for (; i < HIST_MAX; ++i) {
        pos = (start + i) % HIST_MAX;
        if (pos == end) {
            break;
        }

        target = entries[pos];
        if (target->cmd_id == cmd_id) {
            return target->command;
        }
    }

    return NULL;
}

