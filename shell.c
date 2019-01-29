#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>

#include <dirent.h>

#include "history.h"
#include "timer.h"

#define maxName 1024
#define maxBuf 1000

void cleanCWD(char* CWD, char* DCWD, char* home);
void signalHandler(int signo);
void execCmd(char* cmd);
void cdCmd(char* dir, char* CWD, char* home);
char* searchHist(char* token);

/**
The main function is in charge of taking in user input and dealing with it correctly

argc: The number of arguments passed in
argv: The arguments
*/
int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);

    int cmd = 0; //# of command 

    //time variables
    double start = -1;
    double end = -1;

    //getting home dir
    struct passwd *pw = getpwuid(getuid());
    char* home = pw->pw_dir; //user's home directory
    char CWD[PATH_MAX] = {0}; //the longform of the CWD
    char DCWD[PATH_MAX] = {0}; //the shortened form of the CWD, starts with ~
    getcwd(CWD, sizeof(CWD));

    char* token = NULL;
    char breakable[ARG_MAX] = {0};

    //check to see if we're getting input from stdin
    if (!isatty(fileno(stdin))) {
        char buffer[ARG_MAX];
        char buffer2[ARG_MAX];
        char* check = NULL;

        while (1) {
            start = get_time();
            check = fgets(buffer2, ARG_MAX, stdin);
            if (check == NULL) {
                break;
            }
            if (strcmp(check, " ") == 0) {
                continue;
            }
            histHack2:
            strcpy(breakable, check);
            strcpy(buffer, check);
            token = strtok(breakable, " \r\n\t");
            if (strcmp(token, "history") == 0) {
                print_history();
            } else if (token[0] == '!') {
                token = searchHist(token);
                if (token == NULL) {
                    ;
                } else {
                    check = token;
                    goto histHack2;
                }
            }else {
                printf("");
                pid_t pid = fork();
                if (pid == 0) {
                //child
                    execCmd(buffer);
                    exit(0);
                } else if (pid == -1) {
                    printf("ERR AHHHH WHAT DID YOU DO? L:%d\n", __LINE__);
                } else {
                //parent
                    int status;
                    wait(&status);
                }
            }

            end = get_time();
            addHistory(cmd, (end - start), check);
            cmd++;
        }
        return 0;
    }

    char hostname[maxName];
    hostname[maxName - 1] = '\0';
    gethostname(hostname, maxName - 1);
    //get username
    char username[maxName];
    username[maxName - 1] = '\0';
    getlogin_r(username, maxName - 1);

    char command[ARG_MAX] = {0};
    command[ARG_MAX - 1] = '\0';

    while (1) {
        command[0] = '\0';
        cleanCWD(CWD, DCWD, home);
        fflush(stdout);
        printf("[%d|%s@%s:%s]$ ", cmd, username, hostname, DCWD);
        fflush(stdout);
        fgets(command, maxBuf, stdin);
        if (strcmp(command, "\n") == 0) {
            printf("Please enter a valid command.\n");
            continue;
        }
        //goto is here cause why not
        histHack:
        strcpy(breakable, command);

        start = get_time();
        token = strtok(breakable, " \t\r\n");

        //check for exit
        if (strcmp(token, "exit") == 0) {
            end = get_time();
            addHistory(cmd, end - start, command);
            break;
        }

        if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, " \t\r\n");
            cdCmd(token, CWD, home);
        } else if (token[0] == '#') {
        //just comment case
        } else if (strcmp(token, "history") == 0) {
            print_history();
        } else if (token[0] == '!') {
            token = searchHist(token);
            if (token == NULL) {
                ;
            } else {
                strcpy(command, token);
                goto histHack;
            }
        } else { //TODO 
        //honestly no idea why this is here
            pid_t pid = fork();
            if (pid == 0) {
            //child
                execCmd(command);
                return 0;
            } else if (pid == -1) {
                printf("ERR wut %d\n", __LINE__);
            } else {
                int status;
                wait(&status);
            }
        }

        end = get_time();

        //Store the command in history once we're done
        addHistory(cmd,(end - start), command);
        cmd++;
    }

    return 0;
}

/**
This method executes a command given to it, it's here to simplify my code and
make it look pretty

cmd: The command to be executed
*/
void execCmd(char* cmd) {
    char* token = NULL;
    char* commands[ARG_MAX] = {0};
    int tokPos = 0;

    //I have to do this cause strtok messes with the original string
    char breakable[ARG_MAX] = {0};
    strcpy(breakable, cmd);

    //this bit checks to see where the pipe command is, and if there is one
    int pipePos = 0;
    int next = 0;
    int print = 0;

    while (cmd[pipePos] != '\0') {
        if (cmd[pipePos] == '|') {
            next = 1;
            break;
        }
        if (cmd[pipePos] == '>') {
            print = 1;
            break;
        }
        if (cmd[pipePos] == '#') {
            break;
        }
        pipePos++;
    }

    //get array of commands
    token = strtok(breakable, " \r\n\t");
    while (token != NULL) {
        if (!(strcmp(token, "|") && strcmp(token, ">") && strcmp(token, "#"))) {
            break;
        }
        commands[tokPos++] = token;
        token = strtok(NULL, " \r\n\t");
    }
    commands[tokPos] = '\0';

    //unique printing case
    if (print) {
        char* fileLoc = strtok(NULL, " \r\n\t");
        if (fileLoc == NULL) {
            printf("No file was given, wtf dude?\n");
            return;
        }

        pid_t pid = fork();
        if (pid == 0){
        //child
            int out = open(fileLoc, O_CREAT | O_WRONLY, 0666);
            dup2(out, STDOUT_FILENO);

            execvp(commands[0], commands);
            printf("Please enter a valid command to redirect.\n");
            exit(1);
        } else if (pid == -1) {
        //wat
            printf("ERR WHAT DID YOU DO HOW DO YOU GET A -1 PID. L:%d\n",
                     __LINE__);
        } else {
        //parent
            int status;
            wait(&status);
        }

        return;
    }

    if (!next) {
        //gotta fork cause exec kills process
        pid_t pid = fork();
        if (pid == 0) {
        //child
            execvp(commands[0], commands);
            printf("ERR Executing command failed: L:%d Cmd:%s\n",__LINE__, 
                    commands[0]);
            exit(0);
        } else if (pid == -1) {
            printf("ERR AHHHHHHH L:%d\n", __LINE__);
        } else {
        //parent
            int status;
            wait(&status);
            return;
        }
    }

    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
    //child
        close(fd[0]);
        int debug = dup2(fd[1], STDOUT_FILENO);
        execvp(commands[0], commands);
        printf("Executing command failed: L:%d Cmd:%s Debug:%d\n", __LINE__,
                 commands[0], debug);
        exit(1);
    } else if (pid == -1) {
        printf("ERR WITH FORKING WHAT HAPPENED AHHHHH\n L:%d", __LINE__);
    } else {
    //parent
        int status;
        wait(&status);
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        if (print) {
            char buf[ARG_MAX];
            strcpy(buf, "> ");
            strcat(buf, &cmd[pipePos + 1]);
            execCmd(buf);
        } else {
            execCmd(&cmd[pipePos + 1]);
        }
    }
}

/**
This function goes through the history and deals with the !something command

token: The command given by user, ! followed by something

return: String of the command to be executed
*/
char* searchHist(char* token) {
    if (strlen(token) == 1) {
    //if they just put in !
        printf("Please input !! or a valid number.\n");
        return NULL;
    } else if (token[1] == '!') {
    //!! case
        //copies the last cmd in history into command
        char* temp = getCommand(-1);
        if (temp == NULL) {
            printf("You have nothing in your history.\n");
            return NULL;
        } else {
            return temp;
        }
    } else {
        int pre = 0;
        int pos = 1;
        char req[ARG_MAX] = {0};
        strcpy(req, token);
        //puts everything after ! into the request and checks to see 
        //if this is just a number or a prefix string
        while (token[pos] != '\0') {
            if (token[pos] == '#') {
                break;
            }
            if (!isdigit(token[pos])) {
                pre = 1;
            }
            req[pos - 1] = token[pos]; 
            pos++;
        }
        req[pos - 1] = '\0';
        char* buf = NULL;
        //either we prefix the command or we're looking for a number
        if (pre) {
            buf = getCommandPre(req);
        } else {
            buf = getCommand(atoi(req));
        }
        if (buf == NULL) {
            printf("Not a valid request: %s\n", req);
            return NULL;
        }
        //coppies the command and goes to execute the command
        return buf;
    }

    return NULL;
}

/**
This function handles the CD command internally because that's what we gotta do

dir: The directory the user is trying to cd into
CWD: The current working directory
home: The home directory of the user
*/
void cdCmd(char* dir, char* CWD, char* home) {
    DIR *check = NULL; //use this variable to check if directories exist

    //check to see if it's a comment
    if (dir != NULL && dir[0] == '#') {
        dir = NULL;
    }

    if (dir == NULL) {
    //if nothing is given, we go to home dir
        strcpy(CWD, home);
    } else if (dir[0] == '/') {
    //checking to see if there is a / in the start
        if (strlen(dir) > 1) {
            //go to root and see if folder is there
            chdir("/");
            check = opendir(dir);
            if (check == NULL) {
               printf("%s is not a valid directory. L:%d\n", dir, __LINE__);
            } else {
                strcpy(CWD, dir);
                closedir(check);
            }
        } else {
            //we were just given / so go to root folder
            strcpy(CWD, "/");
        }
    } else if (strcmp(dir, ".") == 0) {
        //means current directory so do nothing
        ;
    } else if (strcmp(dir, "..") == 0) {
        //go back one directory
        char* last = strrchr(CWD, '/');
        if (last == NULL) {
            printf("You have hit rock... top? L:%d\n", __LINE__);
        } else {
            //removes the last folder we are in taking us to desired directory
            *last = '\0';
        }
    } else {
    //it's a folder in this directory
        //check to make sure the directory exists
        char absPath[PATH_MAX] = {0};
        strcpy(absPath, CWD);

        //make sure we don't have // anywhere in the path
        if (dir[strlen(dir)] != '/') {
            strcat(absPath, "/");
        }
        strcat(absPath, dir);
        check = opendir(absPath);
        if (check == NULL) {
            printf("%s is not a valid directory. L:%d\n", dir, __LINE__);
        } else {
            strcpy(CWD, absPath);
            closedir(check);
        }
    }

    //massive scam
    chdir(CWD);
}

/**
This method is here to clean up the CWD so that the home directory is replaced
by ~
*/
void cleanCWD(char* CWD, char* DCWD, char* home) {
    int homeLen = strlen(home);

    //check to see if CWD starts with home, if not copy all of CWD into DCWD
    if (strncmp(CWD, home, homeLen) == 0) {
        DCWD[0] = '~';
        int cPos = homeLen;
        int dPos = 1;
        while (CWD[cPos] != '\0') {
            DCWD[dPos] = CWD[cPos];
            dPos++;
            cPos++;
        }
        DCWD[dPos] = '\0';
        return;
    }

    strcpy(DCWD, CWD);
    if (strlen(DCWD) == 0) {
        DCWD[0] = '/';
        DCWD[1] = '\0';
    }
}

/**
Handles the signal
*/
void signalHandler(int signo) {
    printf("\n^C\n");
}
