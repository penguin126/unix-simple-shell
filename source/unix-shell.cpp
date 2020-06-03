#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LINE 80
char* LS[10][MAX_LINE/2 + 1];
int _wait[10]; 
int buffHead = 0;

// III. Creating a History Feature
char** historyViewer(char **args, int *waitE) {

    // Kiểm tra xem user có nhập mỗi "!!" hay không (args[0] == "!!")
    int i;
    if(args[1] == NULL && strcmp(args[0], "!!") == 0){
        if(buffHead > 0){
            strcpy(args[0], LS[(buffHead-1)%10][0]);
            for(i = 1; LS[(buffHead-1)%10][i] != NULL; ++i) {
                args[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                strcpy(args[i], LS[(buffHead-1)%10][i]);
            }
            args[i] = NULL;
            *waitE = _wait[(buffHead-1)%10];
        } else {
            printf("No command in history\n");
            return args;
        }
    }
    for(i = 0; i < (MAX_LINE/2 + 1) && LS[buffHead%10][i] != NULL; ++i){
        free(LS[buffHead%10][i]);
    }
    for(i = 0; args[i] !=NULL ; ++i) {
        LS[buffHead%10][i] = args[i];
    }
    LS[buffHead%10][i] = args[i];
    _wait[buffHead%10] = *waitE;

    return LS[(buffHead++)%10];
}
void outRed(char **argsf, char **args, int &vt_o){
    for(int i=0; i < vt_o; i++){
            argsf[i] = args[i];
        }
        int out = creat(args[vt_o + 1], 0644);
        dup2(out, STDOUT_FILENO);
        close(out);
        execvp(argsf[0], argsf);    
}
void inRed(char **argsfi, char **args, int &vt_i){
    int in;
    for(int i=0; i < vt_i; i++){
            argsfi[i] = args[i];
           }
        if ((in = open(args[vt_i + 1], O_RDONLY)) < 0) { 
            fprintf(stderr, "Can't open this file\n");
        }
        dup2(in, STDIN_FILENO);  
        close(in);                
        execvp(argsfi[0], argsfi);
}
int pipeExec(char **argv1, char **argv2){
    int pipefds[2];
        pid_t pid;
    if(pipe(pipefds) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pipe(pipefds);
    pid = fork();
    if(pid == -1){
        exit(EXIT_FAILURE);
        return 1;
    }
    if(pid == 0){
        dup2(pipefds[1],STDOUT_FILENO);  
        close(pipefds[0]);               
        execvp(argv1[0], argv1);
        exit(EXIT_SUCCESS);
        return 0;
    }else{
        dup2(pipefds[0],STDIN_FILENO);  
        close(pipefds[1]);               
        execvp(argv2[0], argv2);
        exit(EXIT_SUCCESS);
        return 0;
     }   
}
    
int main(void)
{
    int should_run = 1;		
    while (should_run){   
        printf("osh>");
        fflush(stdout);
    	char *args[MAX_LINE/2 + 1];	
        char cmdLine[MAX_LINE/2 + 1];
        char *argv1[MAX_LINE/2 + 1] = {0};
        char *argv2[MAX_LINE/2 + 1] = {0};
        char *argsf[MAX_LINE/2 + 1] = {0};
        char *argsfi[MAX_LINE/2 + 1] = {0};
        char *buffer = cmdLine;
        int index = 0;
        if(scanf("%[^\n]%*1[\n]",cmdLine) < 1) {
            if(scanf("%1[\n]",cmdLine) < 1) {
                return 1;
            }
            continue;
        }

        // phân tích câu lệnh để thực thi
        while(*buffer == ' ' || *buffer=='\t')
        {
            buffer++;
        }
        while(*buffer != '\0')
        {
            char *temp = (char*)malloc((MAX_LINE+1)*sizeof(char));
            args[index] = (char*)malloc((MAX_LINE+1)*sizeof(char));
            int ret = sscanf(buffer,"%[^ \t]", args[index]); 
            buffer += strlen(args[index]);
            if(ret < 1){ // nếu không có ký tự trong command (command = NULL) return 1;
                printf("Invalid command\n");
                return 1;
            }
            ret = sscanf(buffer,"%[ \t]",temp);
            if(ret > 0)
                buffer += strlen(temp);
            index++;
            free(temp);
        }
        int _wait2 = 1;
        if(strlen(args[index-1]) == 1 && args[index-1][0] == '&') {
            _wait2 = 0;
            free(args[index - 1]);
            args[index - 1] = NULL;
        } else {
            args[index] = NULL;
        }
	// check "<"
        bool checkI = false;
        int vt_i;
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], "<") == 0)
            {
                checkI = true;
                vt_i = i;
            }
            continue;
        }
	// check ">"
        bool checkO = false;
        int vt_o;
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], ">") == 0)
            {
                checkO = true;
                vt_o = i;
            }
            continue;
        }
        // check "|"
        bool checkPipe = false;
        int vt;
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], "|") == 0)
            {
                checkPipe = true;
                vt = i;
            }
            continue;
        }
        if(checkPipe == true){
            // V. Communication via a Pipe
            for (int i = 0; i < vt; i++)
            {
                  argv1[i] = args[i];
            }
            int z = 0;
            for (int i = vt + 1; args[i] != NULL; i++)
            {
                  argv2[z] = args[i];
                  z++;
            } 
            if (pipeExec(argv1, argv2) == 0)
            {
                break;
            }        
        // IV. Redirecting Input and Output   
        }else if(checkO == true){
            outRed(argsf, args, vt_o);
        }else if(checkI == true){
            inRed(argsfi, args, vt_i);
        }else{
            //II. Executing Command in a Child Process 
            char **argsExecute = historyViewer(args, &_wait2);
            pid_t pid = fork();
            if(pid<0) {
                return 1;
            } else if(pid==0) {
                if(execvp(argsExecute[0], argsExecute)) {
                    printf("Invalid command\n");
                    return 1;
                }
            } else {
                wait(NULL);
            }    
        }   
    }
	return 0; 
}