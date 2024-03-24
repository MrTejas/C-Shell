#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "colors.h"
#include "header.h"

#define MAX_NAME_SIZE 100
#define MAX_DIR_SIZE 100
#define MAX_COMMAND_SIZE 200


char* host_name;
char* user_name;
char* home_dir;
char command[MAX_COMMAND_SIZE];
char* curr_dir;
char* prev_dir;

char* getHostname()
{
    char* host = (char*)malloc((MAX_NAME_SIZE+1)*sizeof(char));
    gethostname(host,MAX_NAME_SIZE);
    return host;
}

char* getUsername()
{
    char *user = (char*)malloc((MAX_NAME_SIZE+1)*sizeof(char));
    getlogin_r(user,MAX_NAME_SIZE);
    return user;
}

char* getCurrentDirectory()
{
    char* home_dir = (char*)malloc((MAX_DIR_SIZE+1)*sizeof(char));
    home_dir=get_current_dir_name();
    return home_dir;
}

void initializeShell()
{
    printf("%s",CLR);
    host_name = getHostname();
    user_name = getUsername();
    home_dir = getCurrentDirectory();
    curr_dir = (char*)malloc((MAX_DIR_SIZE+1)*sizeof(char));
    prev_dir = (char*)malloc((MAX_DIR_SIZE+1)*sizeof(char));
    strcpy(curr_dir,home_dir);
}

void printPrompt()
{
    
    printf("%s",FBLD);
    printf("%s%s@%s%s:%s",KGRN,user_name,host_name,KWHT,KBLU);
    if(strlen(curr_dir)==0)
    {
        printf("/");
    }
    else if (strncmp(curr_dir, home_dir, strlen(home_dir)) == 0)
    {
        // printf("csdvnc %s %s",curr_dir,home_dir);
        printf("~%s", curr_dir + strlen(home_dir));
    }
    else
    {
        printf("%s",curr_dir);
    }

    printf("%s",RESET);
    printf("%s> ",KWHT);
}

int isPrefix(const char *a, const char *b)
{
    int lenA = strlen(a);
    int lenB = strlen(b);

    if (lenA > lenB) {
        return -1;
    }
    if (strncmp(a, b, lenA) == 0){
        return lenA;
    }
    else{
        return -1;
    }
}

int countArgs(char *str)
{
    int count = 0;
    bool inWord = false;

    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
            if (inWord)
            {
                ++count; 
                inWord = false;
            }
        }
        else
        {
            inWord = true;
        }
    }
    if (inWord) {
        ++count;
    }
    return count;
}

int directoryExists(char* path)
{
    DIR* dir = opendir(path);
    if (dir)
    {
        return 1;
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        return 0;
    }
    else
    {
        return -1;
    }

}

void echo(char* args)
{
    if(args==NULL)
    {
        return;
    }
    printf("%s",KWHT);

    int n = strlen(args);
    short quotes = 0;
    
    for(int i=0;i<n;i++)
    {
        // handling quotes
        if(args[i]=='\"'){
            quotes = 1-quotes;
            continue;
        }

        // handling escape sequences
        if(args[i]=='\\' && i!=n-1){
            if (args[i+1]=='n'){
                printf("\n");
                i++;
                continue;
            }
            else if (args[i+1]=='t'){
                printf("\t");
                i++;
                continue;
            }
        }

        if(quotes){
            printf("%c",args[i]);
        }
        else{
            if(args[i]==' ' || args[i]=='\t'){
                printf(" ");
                int j = i+1;
                while(j<n && (args[j]==' ' || args[j]=='\t'))
                    j++;
                i=j-1;
            }
            else{
                printf("%c",args[i]);
            }
        }
    }
    printf("\n");
}

void pwd()
{
    if(strlen(curr_dir)==0)
    {
        printf("/\n");
        return;
    }
    printf("%s\n",curr_dir);
}

void cd(char* args)
{
    char slash[] = "/";
    if(args==NULL){
        strcpy(prev_dir,curr_dir);
        strcpy(curr_dir,home_dir);
        return;
    }
    if(countArgs(args)>1){
        printf("cd: too many arguments.\n");
        return;
    }

    if (strcmp(args,"-")==0)
    {
        char* temp = (char*)malloc((MAX_DIR_SIZE+1)*sizeof(char));
        printf("%s\n",prev_dir);
        strcpy(temp, curr_dir);
        strcpy(curr_dir,prev_dir);
        strcpy(prev_dir,temp);
        return;
    }
    else if(strcmp(args,".")==0)
    {
        return;
    }
    else if (strcmp(args,"..")==0)
    {
        strcpy(prev_dir,curr_dir);
        if (strcmp(curr_dir,slash)!=0)
        {
            char *lastSlash = strrchr(curr_dir, '/');
            if (lastSlash != NULL)
            {
                *lastSlash = '\0';
            }
        }
        return;
    }
    else if (strcmp(args,"/")==0)
    {
        strcpy(prev_dir,curr_dir);
        strcpy(curr_dir,"");
        return;
    }
    char path[MAX_DIR_SIZE];
    strcpy(path, curr_dir); 
    strcat(path,slash);
    strcat(path, args); 
    if(directoryExists(path))
    {
        strcpy(prev_dir,curr_dir);
        strcpy(curr_dir,path);
    }
    else
    {
        printf("%s : No such directory\n",path);
    }
}

char* getLastCommand()
{
    FILE* fp = fopen("./history.txt","r");
    if(fp==NULL)
    {
        perror("Error opening File!\n");
        return NULL;
    }

    char line[256];
    char* lastLine = (char*)malloc(sizeof(char)*(MAX_COMMAND_SIZE+1));

    while (fgets(line, sizeof(line), fp) != NULL) {
        strcpy(lastLine, line); // Copy the current line to lastLine buffer
    }

    fclose(fp);
    return lastLine;
}

void writeHistory(char* command)
{   
    // do not write duplicate commands in history
    if(strcmp(getLastCommand(),command)==0)
    {
        return;
    }
    FILE* fp = fopen("./history.txt", "r");
    if (fp == NULL) {
        perror("Error Opening File!\n");
        return;
    }
    
    int count_line = 0;
    int ch; // Use int to correctly store EOF and characters
    
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') {
            count_line++;
        }
    }
    fclose(fp);
    if(count_line<20)
    {
        fp = fopen("./history.txt","a");
        if(fp==NULL)
        {
            perror("Error opening file!\n");
            return;
        }
        fprintf(fp,"%s",command);
        fclose(fp);
    }
    else
    {
        FILE* fp = fopen("./history.txt","r");
        if(fp==NULL)
        {
            perror("Error Opening File!\n");
            return;
        }
        char line[1000]; 
        char *commands[19]; 
        int i = 0;

        while (fgets(line, sizeof(line), fp) != NULL) {
            if (i > 0)
            {
                commands[i - 1] = malloc(strlen(line) + 1);
                strcpy(commands[i - 1], line);
            }
            i++;
        }
        fclose(fp);
        fp = fopen("./history.txt","w");
        if(fp==NULL){
            perror("Error opening file!\n");
            return;
        }
        for(int x=0;x<19;x++)
        {
            fprintf(fp,"%s",commands[x]);
        }
        fprintf(fp,"%s",command);
        fclose(fp);
    }

}

void printHistory()
{
    FILE* fp = fopen("./history.txt","r");
    if(fp==NULL)
    {
        perror("Cannot open file : ERROE!\n");
            return;
    }

    int count = 0;
    char line[MAX_COMMAND_SIZE];
    char command_history[20][MAX_COMMAND_SIZE];

    while (fgets(line, sizeof(line), fp) != NULL && count < 20)
    {
        line[strcspn(line, "\n")] = '\0';
        strcpy(command_history[count], line);
        count++;
    }

    fclose(fp);
    int n=count;
    if(n<=10)
    {
        for (int i=0;i<10;i++)
        {
            printf("%s\n",command_history[i]);
        }
    }
    else
    {
        for(int i=n-10;i<n;i++)
        {
            printf("%s\n",command_history[i]);
        }
    }

}

void createNewDirectory(char *name)
{
    char slash[] = "/";
    char path[MAX_DIR_SIZE+MAX_NAME_SIZE];
    strcpy(path,curr_dir);
    strcat(path,slash);
    strcat(path,name);

    int status = mkdir(path, S_IRWXU);
    if (status !=0)
    {
        perror("Error creating directory");
    }
}

void deleteDirectoryRecursively(char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat fileStat;

    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)))
    {
        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);

        if (stat(filePath, &fileStat) == 0)
        {
            if (S_ISDIR(fileStat.st_mode))
            {
                // Recursive call to delete sub-directory
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                {
                    deleteDirectoryRecursively(filePath);
                }
            }
            else
            {
                // Delete regular files
                if (remove(filePath) != 0)
                {
                    perror("Error deleting file");
                }
            }
        }
    }

    closedir(dir);

    // Remove the directory itself after its contents are deleted
    if (rmdir(path) != 0) {
        perror("Error deleting directory");
    }
}

void deleteDirectory(char *args)
{
    char *type;
    char *name;
    char slash[] = "/";
    char path[MAX_DIR_SIZE+MAX_NAME_SIZE];

    type = strtok(args, " ");
    name = strtok(NULL, " ");


    if(name==NULL)
    {    
        strcpy(path,curr_dir);
        strcat(path,slash);
        strcat(path,type);

        struct stat fileStat;
        if (stat(path, &fileStat) == 0)
        {
            if (S_ISREG(fileStat.st_mode))
            {
                if (remove(path) != 0)
                {
                    perror("Error deleting file");
                }
                return;
            }
        }

        int status = rmdir(path);
        if (status != 0)
        {
            perror("Error");
            return;
        }
    }

    strcpy(path,curr_dir);
    strcat(path,slash);
    strcat(path,name);

    if (strcmp(type,"-r")==0)
    {
        deleteDirectoryRecursively(path);
    }
}

void list()
{
    char DIR_CLR[20] = KNRM;
    struct dirent *entry;
    struct stat fileStat;
    DIR* dp;
    if(strlen(curr_dir)==0)
    {
        dp = opendir("/");
    }
    else
    {
        dp = opendir(curr_dir);
    }

    if (dp == NULL) {
        perror("Error opening directory");
        return;
    }

    printf("%s%s %-32s%-20s%-10s%-15s%s\n",FBLD,KMAG,"Name", "Last Modified", "Permiss.", "Size",KNRM);

    while ((entry = readdir(dp))) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char filePath[1024];
            snprintf(filePath, sizeof(filePath), "%s/%s", curr_dir, entry->d_name);

            if (stat(filePath, &fileStat) == 0) {
                char permissions[11];
                strftime(permissions, sizeof(permissions), "%Y-%m-%d %H:%M:%S", localtime(&fileStat.st_mtime));
                
                if (S_ISDIR(fileStat.st_mode))
                {
                    strcpy(DIR_CLR,KBLU);
                }
                else if (fileStat.st_mode & S_IXUSR)
                {
                    strcpy(DIR_CLR,KGRN);
                }
                else
                {
                    strcpy(DIR_CLR,KNRM);
                }

                char sizeStr[20];
                if (S_ISDIR(fileStat.st_mode)) {
                    snprintf(sizeStr, sizeof(sizeStr), "-");
                } else {
                    snprintf(sizeStr, sizeof(sizeStr), "%ld bytes", (long)fileStat.st_size);
                }

                printf("%s %-30s %-1s %-20s%-10o%-15s\n",DIR_CLR, entry->d_name,KNRM, permissions, fileStat.st_mode & 0777, sizeStr);
            }
        }
    }

    closedir(dp);

}

void createFile(char* name)
{
    if (mkdir(curr_dir, 0777) == 0)
    {
        printf("Directory does not exist!\n");
        return;
    }

    // Create the full path including the file name
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", curr_dir, name);
    // Create the file inside the directory
    FILE *file = fopen(filePath, "w");
    if (file != NULL)
    {
        fclose(file);
    }
    else
    {
        perror("Error creating file");
    }
}

int main()
{
    initializeShell();
    while(1)
    {
        printPrompt();
        fgets(command, MAX_COMMAND_SIZE, stdin);
        char *cmd;
        char *args;
        writeHistory(command);

        cmd = strtok(command, " \n");
        if (cmd==NULL)
            continue;
        args = strtok(NULL, "\n");
                
        if(strcmp(cmd,"echo")==0){
            echo(args);
        }
        else if (strcmp(cmd,"pwd")==0){
            pwd();
        }
        else if (strcmp(cmd,"cd")==0){
            cd(args);   
        }
        else if (strcmp(cmd,"history")==0){
            printHistory();
        }
        else if (strcmp(cmd,"exit")==0){
            break;
        }
        else if (strcmp(cmd,"clear")==0 || strcmp(cmd,"c")==0){// alias for clear->"c"
            printf("%s",CLR);
        }
        else if (strcmp(cmd,"ls")==0){
            list();
        }
        else if (strcmp(cmd,"mkdir")==0){
            createNewDirectory(args);
        }
        else if (strcmp(cmd,"rm")==0){
            deleteDirectory(args);
        }
        else if (strcmp(cmd,"touch")==0){
            createFile(args);
        }
        else
        {
            printf("no such command.\n");
        }
    }
    return 0;
}
