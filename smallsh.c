#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


int allow_background=1; //1 is yes(allow background); 0 is no(do not allow background)

/* struct to store command line into. will be seperated out in parseCommand(). ground will be a 0 to indicate
foreground or a 1 to indicate background process */
struct command
{
    char *comm;
    char *arg;
    char *redirect;
    int foreground;  //1 is foreground; 0 is background
};

/*global variable set to random # as a starting point for status function to check against if a foreground
  process hasn't been performed previously*/
int  childStatCheck = 121;  


/*function that takes in the struct line for either the args or the redirect as parameter. The function counts 
the number of spaces in args and redirects to determine how many arguments and redirects the user entered. 
The count is returned to the new() function and is used to help create an array for the arguments to be used 
with exec() and also for determining which function to use for the I/O redirect*/
int numberOf(char* strchk)
{
    int length = strlen(strchk);
    int spaceCount=0;
    if (strlen(strchk)>0){
        spaceCount++;
        for (int i=0; i<=length; i++){
            if (strchk[i] == ' '){
                spaceCount++;}
        }
    }
    return spaceCount;
}


/*This function redirects I/O for two different files. The files are opened and either written to or read
based on the direction symbol provided. Parameters are the files and I/O instruction for each file.
Code is heavily based upon Exploration: Processes and I/O -- Example: Redirecting both Stdin and Stdout */
void redirectTwo(int redirInt1, char* file1, int redirInt2, char*file2 )
{
    int FD;         //file descriptor for first file in user command
    int secFD;      //file descriptor for second file in user command
    int result;

    /*the first file is opened and either read or written to depending on the instruction given in the command.
      if the redirInt is 0, the file will be read from. If it is 1, the file will be written to.*/
    if(redirInt1 == 0){
        FD = open(file1, O_RDONLY);
        if (FD == -1) { 
		    perror("source open()"); 
		    exit(1);}
    }else{
        FD = open(file1, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (FD == -1) { 
		    perror("target open()"); 
		    exit(1);}}

   /*dup is used to redirect.  redirInt variable is already set to either 0 or 1 based on user command to tell
   dup where the redirect goes*/
    result = dup2(FD, redirInt1);
    if (result == -1) { 
        perror("target dup2()"); 
        exit(2);}

    /* repeat of the code above using the 2nd file and 2nd file I/O indicator*/
    if(redirInt2 == 0){
        secFD = open(file2, O_RDONLY);
        if (secFD == -1) {
            perror("source open()");
            exit(1);}
    }else{
        secFD = open(file2, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (secFD == -1) {
            perror("target open()");
            exit(1);}
    }   
        
    result = dup2(secFD, redirInt2);
    if (result == -1) { 
        perror("target dup2()"); 
        exit(2);}
}


/*This function redirects I/O for one file. The files is opened and either written to or read
based on the direction symbol provided. Parameters are the file and I/O instruction for the file.
I couldn't initially think of a good way to put the logic in place for 1 and 2 file redirections in the same
function so I made seperate functions for each and directed the program to the function it should go to based
on how many redirects were counted in the numberOf() function.
Code is heavily based upon Exploration: Processes and I/O -- Example: Redirecting both Stdin and Stdout */
void redirect(int redirInt1, char* file1)
{
    int FD;
    int result;

    /*the file is opened and either read from or written to depending on the instruction given in the command.
      if the redirInt is 0, the file will be read from. If it is 1, the file will be written to.*/
    if(redirInt1 == 0){
        FD = open(file1, O_RDONLY);
        if (FD == -1) { 
		    perror("source open()"); 
		    exit(1);}
    }else{
        FD = open(file1, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (FD == -1) { 
		    perror("target open()"); 
		    exit(1);}
    }
		/*dup is used to redirect.  redirInt variable is already set to either 0 or 1 based on user command to tell
        dup where the redirect goes*/
        result = dup2(FD, redirInt1);
        if (result == -1) { 
		    perror("target dup2()"); 
		    exit(2);}
}


/*
This function prepares the parts of the command line for use with I/O redirection and exec() 
and then impliments exec(). Foreground vs background groups are determined based off command 
struct ground variable.Takes the command struct as an argument.
Code regarding use of exec() function heavily based on Exploration:Process API-Executing a 
New Program--Using exec() with fork().  Website used for code on allocating memory for 2d array: 
https://stackoverflow.com/questions/2614249/dynamic-memory-for-2d-char-array*/
void new(struct command *currCommand, struct sigaction sa)
{
	int childStatus;

    int numOfarg = numberOf(currCommand->arg); //number of arguments calculated for array setup
    
    //number of files to redirect determined so that this information can be sent to the correct function above.
    int numOfredirect = numberOf(currCommand->redirect)/2;  
    char redir1[10], file1[100], redir2[10], file2[100];
    int redirInt1, redirInt2;
    size_t row = numOfarg +2; //calculates number of rows needed for 2d array for exec() based on user input
    
    //allocation of memory for array rows & columns
    char **childArgs= malloc(row * sizeof(char *)); 
    for(int i = 0; i < row; i++){
    childArgs[i] = malloc(100 * sizeof(char));}

    //this block of code creates 2d array with command/arguments for exect array; first in array is the pathname
    strcpy(childArgs[0], currCommand->comm); 
    /*code below takes the line designated as arg from the original command input and seperates it out into
    seperate words in an array. Some of the code below is influenced by the original movies HW1 assignment 
    in CS344 using strtok_r*/
    char *saveptr;
    char *token;
    //printf("\nnumber of args are %d\n", numOfarg);
    if(numOfarg !=0)
    {
        token = strtok_r(currCommand->arg, " ", &saveptr); 
        strcpy(childArgs[1], token);
        for(int i=2; i <= numOfarg; i++)
        {
            token = strtok_r(NULL, " ", &saveptr);
            if(token == NULL){break;}
            strcpy(childArgs[i], token);
            //printf("\nchildArg %d is %s\n", i, childArgs[i]);
        }
    }
    childArgs[numOfarg+1] = NULL; //last place in the array is made NULL

    /* this section of code seperates out the direction symbols with the files they go with and gives them each
    their own variable. This allowed me to plug them into the code needed for file redirection in the redirect() */
    //code for getting variables for 1 file redirection
    if(numOfredirect ==1)
    {
        sscanf(currCommand->redirect, "%s %s", redir1, file1);
        if (strcmp(redir1, "<")==0){redirInt1=0;}
        else{redirInt1=1;}
    }
    //code for getting variables for 2 file redirections
    if(numOfredirect ==2)
    {
        sscanf(currCommand->redirect, "%s %s %s %s", redir1, file1, redir2, file2);
        if (strcmp(redir1, "<")==0){redirInt1=0;}
        else{redirInt1=1;}
        if (strcmp(redir2, "<")==0){redirInt2=0;}
        else{redirInt2=1;}
    }
    
    pid_t spawnPid = fork(); //gets pid for child to determine errors/send child process to seperate code for exec()
    switch(spawnPid){
        case -1:
            perror("fork()\n");
            exit(2);
            break;
        case 0:
            if(currCommand->foreground != 0 || allow_background == 1){
                sa.sa_handler=SIG_DFL;
                sigaction(SIGINT,&sa,NULL);}
            //handles file I/O redirection for 1 or 2 files
            if (numOfredirect ==1){redirect(redirInt1, file1);}
            else if (numOfredirect ==2){redirectTwo(redirInt1, file1, redirInt2, file2);}

            execvp(childArgs[0],childArgs);     //creates new process with info in array from user input.
            perror("execvp");
            exit(1);
            break;
        default:
            //if foreground process (ground = 0), then waitpid is called to run child in foreground.
            if(currCommand->foreground ==1 && allow_background==1){
                spawnPid = waitpid(spawnPid, &childStatus, 0);
                childStatCheck = childStatus;
                if (!WIFEXITED(childStatCheck)){
		            printf("Terminated by signal %d\n", WTERMSIG(childStatCheck));
                    fflush(stdout);
	            }
            }  //child status is updated in status() function here
            break;
    }
    //message is sent if child is running in the background
    //printf("\nglobal background is %d\n", allow_background);
    if(currCommand->foreground ==0  && allow_background ==1){
        printf("\nPID %d is running in background.\n", spawnPid);
        fflush(stdout);}
    free(childArgs);
}


void printStatus(int exitStatus)
{
    if(WIFEXITED(exitStatus)) 
    {printf("exit value: %d\n", WEXITSTATUS(exitStatus));}
    else 
    {printf("terminated by signal: %d\n", WTERMSIG(exitStatus));}    
}


/* function checks status of last terminated child process and prints exit status or signal used to terminate. 
Used code from Exploration: Process API - Monitoring Child Processes*/
void status()
{
    /*checks global variable to see if a process has been terminated yet. if not,returns 0. If so then status is
    checked and message printed. childStatCheck is a global variable continuously updated in new()*/ 
    if (childStatCheck == 121){printf("exit status 0");}
    else if(WIFEXITED(childStatCheck)){
        printf("Exit value %d\n", WEXITSTATUS(childStatCheck));
        fflush(stdout);
	} else{
		printf("Terminated by signal %d\n", WTERMSIG(childStatCheck));
        fflush(stdout);
	}
}


void checkBackground(){
    int status;
    int pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("child with PID %d terminated\n", pid);
        printStatus(status);
    }
}


/*funtion takes "cd" command and changes directory based on user argument. W/O argument, dir is changed to 
home directory. W/ argument, directory is changed to inputted path. code influenced by website:
https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/#:~:text=The%20getcwd%20%28%29%20function%20places%20an%20absolute%20pathname,undefined.The%20return%20value%20represent%20our%20current%20working%20directory.*/
void change(struct command *currCommand)
{
    char* home = getenv("HOME");  //gets home environment

    if (strlen(currCommand->arg) == 0){chdir(home);}
    else{chdir(currCommand->arg);}
}


/* Function evalutes the first word in the command line to determine which function should handle it next.
It is either sent to one of the 3 built in commands or to the new() function for exec().Takes current command struct
as parameter*/
void evaluateCommand(struct command *currCommand, struct sigaction sa)
{
    if((strstr(currCommand->comm, "exit")) != NULL){exit(0);}
    else if((strstr(currCommand->comm, "cd")) != NULL){change(currCommand);}
    else if((strstr(currCommand->comm, "status")) != NULL){status();}
    else{new(currCommand, sa);}
}


/*Function checks for "$$" anywhere in command and expands variable with pid in place of "$$". Takes
original command line input as parameter. Returns original line(if $$ absent) or newly expanded line.*/
char* expandVariable(char* varCheck)
{
    char* find;
    pid_t pid= getpid();
    char currpid[10]; 
    sprintf(currpid, "%d", pid);  //converts pid to char
    char* newVarCheck;     //new variable created for expansion
    int newVarCount=0;
    int length = strlen(varCheck);      //length of current variable for loop and memory allocation
    int lengthpid = strlen(currpid);    //length of current variable memory allocation
    find = strstr(varCheck, "$$");
    //checks if $$ exists in command entered
    if (find)
    {
        //creates new variable with pid extention if $$ exists
        newVarCheck = calloc((length + lengthpid) +1, sizeof(char));  //var memory for after var extention
        for (int i=0; i<=length; i++){
            if (varCheck[i] == '$' && varCheck[i+1] == '$') {
                for (int pidcount=0; pidcount<lengthpid; pidcount++){
                    newVarCheck[newVarCount] = currpid[pidcount];
                    newVarCount++;}
                i++;
                newVarCount++;}
            else{ 
                newVarCheck[newVarCount] = varCheck[i];
                newVarCount++;}
        }
        return newVarCheck;
    }
    else {return varCheck;}

    free(newVarCheck);
}


/* function parses command line input and seperates it out into a struct. This seemed the best way to keep
track of all the different parts of the command and can be easily moved between functions to extract information.
takes the command line, after any variable expansion, as the parameter.*/
struct command *parseCommand(char* commLine, struct sigaction sa)
{
    int ptr =0;
    int lineLength = strlen(commLine);  //gets length of full command line entered
    //these are used as buffers to hold lines until they are complete and can be copied into the struct.
    char *buffcomm= NULL;
    char *buffarg= NULL;
    char *buffredirect=NULL;

    struct command *currCommand = malloc(sizeof(struct command));  //memory allocation for each struct

    //copies each character from command line to buffcomm until a space is reached.
    int copy = 0;
    buffcomm = calloc(strlen(commLine) + 1, sizeof(char));  //memory allocated for buffcomm
    for (ptr; ptr < lineLength-1; ptr++){
        if (commLine[ptr] == ' ') {
            ptr++; //skips over space so that buffarg will not start with a space
            break;
        }
        buffcomm[ptr] = commLine[ptr];
    }
    currCommand->comm = calloc(strlen(buffcomm) + 1, sizeof(char));     //memory in struct is allocated
    strcpy(currCommand->comm, buffcomm);    //first word in command copied to struct comm variable
    
    /*continues character count through command line. copies each character from command line to 
    buffarg until a <,>, or & is reached.  This should get all command arguments in one line for an array later*/
    copy = 0;
    buffarg = calloc(strlen(commLine) + 1, sizeof(char)); //memory allocated for buffarg
    for (ptr; ptr < lineLength-1; ptr++){
        if (commLine[ptr] == '<'|| commLine[ptr] == '>' || (commLine[ptr] =='&' && commLine[ptr+1]!='\0')) {
            if (commLine[ptr] == '&'){ buffarg[copy-1] = '\0';}
            break;
        }
        else{buffarg[copy] = commLine[ptr];}
        copy++;
    }
    currCommand->arg = calloc(strlen(buffarg) + 1, sizeof(char));
    strcpy(currCommand->arg, buffarg);
    
    //continues through command line copying characters until a the end of the command or a & is reached.
    copy = 0;
    buffredirect = calloc(strlen(commLine) + 1, sizeof(char));
    for (ptr; ptr < lineLength; ptr++){
        if (commLine[ptr] !='&') 
        {
            buffredirect[copy] = commLine[ptr];
            currCommand->foreground = 1;  //stays foreground
            copy++;
        }
        else{
            currCommand->foreground = 0; //goes background
            break;}

    }
    currCommand->redirect = calloc(strlen(buffredirect) + 1, sizeof(char));
    strcpy(currCommand->redirect, buffredirect);
    evaluateCommand(currCommand, sa);  //sends struct command to function to evaluate where command should be processed

    free(buffarg);
    free(buffcomm);
    free(buffredirect);
    free(currCommand);
}


/*SIGTSTP signal handler. Code heavily influenced from Exploration: Signal Handling API.
Example: Custom Handlers for SIGINT, SIGUSR2, and Ignoring SIGTERM, etc. changes access to ability to 
conduct background processes based on how many times it is called. Toggles on/off. Sends message with each
toggle*/
void handle_SIGTSTP(int signo){
    if (allow_background == 1){
        char* onMessage = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, onMessage, 49);
        allow_background = 0;}
    else{
        char* offMessage = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, offMessage, 34);
        allow_background = 1;}
}

int main()
{
    //initializing sigaction struct

    struct sigaction SIGINT_action = {{0}}; 
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);
    
    struct sigaction SIGTSTP_action = {{0}};     
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


    char *buff;
    size_t buffsize = 32;
    size_t currCommand; 
    int endLoop =0;

    //Loop for command line that takes input from user and sends it to be parsed
    while( endLoop !=200)
    {
        buff = (char *)malloc(buffsize * sizeof(char));  //memory allocation for input
        fflush(stdout);
        checkBackground();
        printf("\n:");
        fflush(stdout);
        currCommand = getline(&buff, &buffsize, stdin);
        if(currCommand == -1){clearerr(stdin);continue;}
        if (buff[0]!='#' && buff[0] !=' '){
                buff = expandVariable(buff);   //command line sent for to check for $$ and potential expansion
                parseCommand(buff, SIGINT_action);}    
        endLoop++;
    }
    free(buff);
    return 0;
}
