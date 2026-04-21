#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>

// za cpcat
#define BUFFER_SIZE 1024 
#define BUFFER_ALLOC char* buffer = malloc(BUFFER_SIZE*sizeof(char));

// za proc
char PATH[100] = "/proc";

#define MAX_LINE_SIZE 1024 
int DEBUG_LEVEL = 0;
char* PROMPT = "mysh";
int STATUS = 0;

typedef struct{
    const char* ime;
    const char* pomoc;
    // kazalec na funkcijo
    int (*fPtr)(int*, char**);
} Ukaz;

// neka struktura samo za potrebe funkicje
// zato ker nimamo parov v C
typedef struct{
    char* line;
    bool onlySpace;

    bool hasComment;
    int commentPointer;
} LineInfo;

LineInfo stringCleanup(char* line){
    LineInfo li;
    char* newLine = malloc(MAX_LINE_SIZE*sizeof(char));
    int p = 0;
    int nP = 0;
    bool isOnlySpace = true;

    bool hasComment = false;
    bool foundComment = false;
    int commentPointer = 0;

    while (line[p] == ' ') p++;
    while (line[p] != '\0'){
        isOnlySpace = false;
        if(line[p] == '#' && !foundComment){
            hasComment = true;
            foundComment = true;
            commentPointer = nP;
        }
        if(line[p] == '"'){
            newLine[nP++] = line[p++];
            while(line[p] != '"' && line[p] != '\0'){
                newLine[nP++] = line[p++];
            }
            if(line[p] == '"'){
                newLine[nP++] = line[p++];
            }
        }else{
            while (line[p] != ' ' && line[p] != '"' && line[p] != '\0'){
                newLine[nP++] = line[p++];
            }
        }
        while (line[p] == ' ') p++;
        if (line[p] != '\0'){
            newLine[nP++] = ' ';
        }
    }
    newLine[nP] = '\0';

    li.line = newLine;
    li.hasComment = hasComment;
    li.commentPointer = commentPointer;
    li.onlySpace = isOnlySpace;
    return li;
}

// -------------------------------------------------------------------------

int exitCustom(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        exit(atoi(tokens[1]));
    }else{
        exit(STATUS);
    }
    return 0;
}

int debug(int* tokenCount, char** tokens){
    int tc = *tokenCount;
    if(tc > 1){
        int j = 0;
        bool pravaOblika = true;
        while(tokens[1][j] != '\0'){
            if(isalpha(tokens[1][j])){
                DEBUG_LEVEL = 0;
                pravaOblika = false;
            }
            j++;
        }
        if(pravaOblika) DEBUG_LEVEL = atoi(tokens[1]);
    }else{
        printf("%d\n", DEBUG_LEVEL);
    }
    return 0;
}

int prompt(int* tokenCount, char** tokens){
    if(*tokenCount > 1){
        if(strlen(tokens[1]) > 7){
            return 1;
        }else{
            PROMPT = tokens[1];
        }
    }else{
        printf("%s\n", PROMPT);
    }
    fflush(stdout);
    fflush(stderr);
    return 0;
}

int status(){
    printf("%d\n", STATUS);
    fflush(stdout);
    fflush(stderr);
    return STATUS;
}

int print(int* tokenCount, char** tokens){
    bool prvi = true;
    for(int i = 1; i < *tokenCount; i++){
        if(prvi){
            printf("%s", tokens[i]);
            prvi = false;
        }else{
            printf(" %s", tokens[i]);
        }
    }
    fflush(stdout);
    fflush(stderr);
    return 0;
}

int echo(int* tokenCount, char** tokens){
    bool prvi = true;
    for(int i = 1; i < *tokenCount; i++){
        if(prvi){
            printf("%s", tokens[i]);
            prvi = false;
        }else{
            printf(" %s", tokens[i]);
        }
    }
    printf("\n");
    fflush(stdout);
    fflush(stderr);
    return 0;
}

int len(int* tokenCount, char** tokens){
    int dolzina = 0;
    for(int i = 1; i < *tokenCount; i++){
        dolzina += strlen(tokens[i]);
    }
    printf("%d\n", dolzina);
    return 0;
}

int sum(int* tokenCount, char** tokens){
    int vsota = 0;
    for(int i = 1; i < *tokenCount; i++){
        bool pravaOblika = true;
        int j = 0;
        while(tokens[i][j] != '\0'){
            if(isalpha(tokens[i][j])){
                pravaOblika = false;
                break;
            }
            j++;
        }
        if(pravaOblika){
            vsota += atoi(tokens[i]);
        }else{
            printf("ERROR! : arguments must be whole numbers!\n");
            return 1;
        }
    }
    printf("%d\n", vsota);
    return 0;
}

// tuki ne bom preverjal pravilnosti dejanskih argumentov / operatorjev preveč
int calc(int* tokenCount, char** tokens){
    if(*tokenCount == 4){
        if(strcmp(tokens[2], "+") == 0){
            printf("%d\n", atoi(tokens[1]) + atoi(tokens[3]));
        }else if(strcmp(tokens[2], "-") == 0){
            printf("%d\n", atoi(tokens[1]) - atoi(tokens[3]));
        }else if(strcmp(tokens[2], "*") == 0){
            printf("%d\n", atoi(tokens[1]) * atoi(tokens[3]));
        }else if(strcmp(tokens[2], "/") == 0){
            printf("%d\n", atoi(tokens[1]) / atoi(tokens[3]));
        }else if(tokens[2][0] == '%'){
            printf("%d\n", atoi(tokens[1]) % atoi(tokens[3]));
        }else{
            printf("ERROR! : expected op to be one of these {'+', '-', '*', '/', '%%'}!\n");
        }
    }else{
        printf("ERROR! : expected : calc <arg1 op arg2>!\n");
        return 1;
    }
    return 0;
}

int basename(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        int lastSp = 0;
        int i = 0;
        while(tokens[1][i] != '\0'){
            if(tokens[1][i] == '/'){
                lastSp = i;
            }
            i++;
        }
        if(i == 1){
            printf("%s\n", tokens[1] + lastSp);
            fflush(stdout);
            fflush(stderr);
        }else{
            printf("%s\n", tokens[1] + lastSp + 1);
            fflush(stdout);
            fflush(stderr);
        }
        return 0;
    }else{
        // printf("ERROR! : expected : basename <path>!\n");
        return 1;
    }
}

int dirname(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        int lastSp = 0;
        int i = 0;
        while(tokens[1][i] != '\0'){
            if(tokens[1][i] == '/'){
                lastSp = i;
            }
            i++;
        }
        char* cpy = malloc(100 * sizeof(char));
        strcpy(cpy, tokens[1]);
        cpy[lastSp] = '\0';
        printf("%s\n", cpy);
        free(cpy);
        fflush(stdout);
        fflush(stderr);
        return 0;
    }else{
        // printf("ERROR! : expected : basename <path>!\n");
        return 1;
    }
}

int dirch(int* tokenCount, char** tokens){
    if(*tokenCount == 1){
        if(chdir("/") < 0){
            int koda = errno;
            perror("dirch");
            fflush(stdout);
            fflush(stderr);
            return koda;
        }
    }else{
        if(chdir(tokens[1]) < 0){
            int koda = errno;
            perror("dirch");
            fflush(stdout);
            fflush(stderr);
            return koda;
        }
    }
    return 0;
}

int dirwd(int* tokenCount, char** tokens){
    if(*tokenCount == 1){
        char dir[256];
        getcwd(dir, 256);
        int lastSp = 0;
        int i = 0;
        while(dir[i] != '\0'){
            if(dir[i] == '/'){
                lastSp = i;
            }
            i++;
        }
        if(i == 1){
            printf("%s\n", dir + lastSp);
            fflush(stdout);
            fflush(stderr);
        }else{
            printf("%s\n", dir + lastSp + 1);
            fflush(stdout);
            fflush(stderr);
        }
    }else{
        if(strcmp(tokens[1], "full") == 0){
            char dir[256];
            printf("%s\n", getcwd(dir, 256));
            fflush(stdout);
            fflush(stderr);
        }else if(strcmp(tokens[1], "base") == 0){
            char dir[256];
            getcwd(dir, 256);
            int lastSp = 0;
            int i = 0;
            while(dir[i] != '\0'){
                if(dir[i] == '/'){
                    lastSp = i;
                }
                i++;
            }
            if(i == 1){
                printf("%s\n", dir + lastSp);
                fflush(stdout);
                fflush(stderr);
            }else{
                printf("%s\n", dir + lastSp + 1);
                fflush(stdout);
                fflush(stderr);
            }
        }
    }
    return 0;
}

int dirmk(int* tokenCount, char** tokens){
    if(*tokenCount >= 2){
        if(mkdir(tokens[1], 0755) < 0){
            int koda = errno;
            perror("dirmk");
            fflush(stdout);
            fflush(stderr);
            return koda;
        }
    }else{
        printf("ERROR! : expected dirmk <directory_name>!\n");
        return 1;
    }
    return 0;
}

int dirrm(int* tokenCount, char** tokens){
    if(*tokenCount >= 2){
        if(rmdir(tokens[1]) < 0){
            int koda = errno;
            perror("dirrm");
            fflush(stdout);
            fflush(stderr);
            return koda;
        }
    }
    return 0;
}

int dirls(int* tokenCount, char** tokens){
    DIR* dir;
    if(*tokenCount == 1){
        dir = opendir(".");
    }else{
        dir = opendir(tokens[1]);
    }
    struct dirent* entry;
    bool prvi = true;
    while((entry = readdir(dir)) != 0){
        if(prvi){
            printf("%s", entry->d_name);
            prvi = false;
        }else{
            printf("  %s", entry->d_name);
        }
    }
    printf("\n");
    fflush(stdout);
    fflush(stderr);
    closedir(dir);
    return 0;
}

int renameFile(int* tokenCount, char** tokens){
    if(*tokenCount == 3){
        if(rename(tokens[1], tokens[2]) < 0){
            int koda = errno;
            return koda;
        }
    }else{
        return 1;
    }
    return 0;
}

int unlinkFile(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        if(unlink(tokens[1]) < 0){
            int koda = errno;
            return koda;
        }
    }else{
        return 1;
    }
    return 0;
}

int removeFile(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        if(remove(tokens[1]) < 0){
            int koda = errno;
            return koda;
        }
    }else{
        return 1;
    }
    return 0;
}

int linkhard(int* tokenCount, char** tokens){
    if(*tokenCount == 3){
        if(link(tokens[1], tokens[2]) < 0){
            int koda = errno;
            return errno;
        }
    }else{
        return 1;
    }
    return 0;
}

int linksoft(int* tokenCount, char** tokens){
    if(*tokenCount == 3){
        if(symlink(tokens[1], tokens[2]) < 0){
            int koda = errno;
            return koda;
        }
    }else{
        return 1;
    }
    return 0;
}

int linkread(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        char* buffer = malloc(1024*sizeof(char));
        if(readlink(tokens[1], buffer, 1024) < 0){
            int koda = errno;
            return errno;
        }
        printf("%s\n", buffer);
        free(buffer);
    }else{
        return 1;
    }
    return 0;
}

int linklist(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        struct stat s;
        if(stat(tokens[1], &s) < 0){
            int koda = errno;
            return koda;
        }

        DIR* dir = opendir(".");
        struct dirent* entry;
        bool prvi = true;
        while((entry = readdir(dir)) != 0){
            struct stat is;
            if(lstat(entry->d_name, &is) < 0){
                int koda = errno;
                closedir(dir);
                return koda;
            }

            if(s.st_ino == is.st_ino && s.st_dev == is.st_dev){
                if(prvi){
                    printf("%s", entry->d_name);
                    prvi = false;
                }else{
                    printf("  %s", entry->d_name);
                }
            }
        }
        printf("\n");
        closedir(dir);
        return 0;
    }else{
        return 1;
    }
    return 0;
}


int cpcat(int* tokenCount, char** tokens){
    // stdin -> stdout
    BUFFER_ALLOC
    int vhodniDeskriptor = STDIN_FILENO;
    int izhodniDeskriptor = STDOUT_FILENO;

    if(*tokenCount == 2){
        // datoteka -> stdout
        vhodniDeskriptor = open(tokens[1], O_RDONLY);
        if(vhodniDeskriptor < 0){
            int koda = errno;
            // perror("ERROR [odpiranje arg_1]");
            perror("cpcat");
            free(buffer);
            return koda;
        }
    }else if(*tokenCount == 3){
        // stdin -> datoteka
        if(strcmp(tokens[1], "-") == 0){
            izhodniDeskriptor = open(tokens[2], O_CREAT | O_WRONLY | O_TRUNC , 0644);
            if(izhodniDeskriptor < 0){
                int koda = errno;
                // perror("ERROR [odpiranje arg_2]");
                perror("cpcat");
                free(buffer);
                return koda;
            }
        }else{
            // datoteka a -> datoteka b
            vhodniDeskriptor = open(tokens[1], O_RDONLY);
            if(vhodniDeskriptor < 0){
                int koda = errno;
                // perror("ERROR [odpiranje arg_1] (ste morda mislili '-' namesto '_')");
                perror("cpcat");
                free(buffer);
                return koda;
            }

            izhodniDeskriptor = open(tokens[2], O_CREAT | O_WRONLY | O_TRUNC , 0644);
            if(izhodniDeskriptor < 0){
                int koda = errno;
                // perror("ERROR [odpiranje]");
                perror("cpcat");
                free(buffer);
                return koda;
            }
        }
    }else if(*tokenCount > 3){
        printf("ERROR! : too many arguments, expected : cpcat <source destination>\n");
        return 1;
    }

    int prebraniBajti = 0;
    char lastChar = '\0';
    while ((prebraniBajti = read(vhodniDeskriptor, buffer, BUFFER_SIZE)) > 0) {
        if (write(izhodniDeskriptor, buffer, prebraniBajti) != prebraniBajti) {
            perror("cpcat");
            free(buffer);
            return errno;
        }
        lastChar = buffer[prebraniBajti - 1];
    }

    if (prebraniBajti == 0 && lastChar != '\0' && lastChar != '\n') {
        if (write(izhodniDeskriptor, "\n", 1) < 0) {
            perror("cpcat");
            free(buffer);
            return errno;
        }
    }

    if(prebraniBajti < 0){
        int koda = errno;
        // perror("ERROR [branje arg_2]");
        perror("cpcat");
        free(buffer);
        return koda;
    }

    if(vhodniDeskriptor > 2) close(vhodniDeskriptor);
    if(izhodniDeskriptor > 2) close(izhodniDeskriptor);
    free(buffer);
    return 0;
}

int pid(int* tokenCount, char** tokens){
    printf("%d\n", getpid());
    return 0;
}

int ppid(int* tokenCount, char** tokens){
    printf("%d\n", getppid());
    return 0;
}

int uid(int* tokenCount, char** tokens){
    printf("%d\n", getuid());
    return 0;
}

int euid(int* tokenCount, char** tokens){
    printf("%d\n", geteuid());
    return 0;
}

int gid(int* tokenCount, char** tokens){
    printf("%d\n", getgid());
    return 0;
}

int egid(int* tokenCount, char** tokens){
    printf("%d\n", getegid());
    return 0;
}

int sysinfo(int* tokenCount, char** tokens){
    struct utsname info;
    uname(&info);
    printf("Sysname: %s\nNodename: %s\nRelease: %s\nVersion: %s\nMachine: %s\n", info.sysname, info.nodename, info.release, info.version, info.machine);
    return 0;
}



int proc(int* tokenCount, char** tokens){
    if(*tokenCount == 2){
        if(access(tokens[1], F_OK|R_OK) < 0){
            return 1;
        }
        strcpy(&PATH[0], tokens[1]);
        return 0;
    }
    printf("%s\n", PATH);
    fflush(stdout);
    fflush(stderr);
    return 0;
}

int sort(int idx, int* tab){
    for(int i = 0; i < idx - 1; i++){
        for(int j = 0; j < idx - i - 1; j++){
            if(tab[j + 1] < tab[j]){
                int tmp = tab[j];
                tab[j] = tab[j + 1];
                tab[j + 1] = tmp;
            }
        }
    }
}

int pids(int* tokenCount, char** tokens){
    // const char* myPath = "../../../../proc";
    int* pidTab = malloc(1000 * sizeof(int));
    DIR* dir = opendir(PATH);
    struct dirent* entry;
    int idx = 0;
    while((entry = readdir(dir)) != 0){
        bool pid = true;
        for(int i = 0; i < strlen(entry->d_name); i++){
            if(entry->d_name[i] > '9' || entry->d_name[i] < '0'){
                pid = false;
                break;
            }
        }
        if(pid){
            // printf("%s\n", entry->d_name);
            pidTab[idx] = atoi(entry->d_name);
            idx++;
        }
    }
    // bubble sort O(n^2).. ok
    sort(idx, pidTab);
    for(int j = 0; j < idx; j++){
        printf("%d\n", pidTab[j]);
        fflush(stdout);
    }
    free(pidTab);
    closedir(dir);
    return 0;
}

int pinfo(int* tokenCount, char** tokens){
    const char* myPath = "../../../../proc";
    int* pidTab = malloc(1000 * sizeof(int));
    DIR* dir = opendir(PATH);
    struct dirent* entry;
    int idx = 0;
    while((entry = readdir(dir)) != 0){
        bool pid = true;
        for(int i = 0; i < strlen(entry->d_name); i++){
            if(entry->d_name[i] > '9' || entry->d_name[i] < '0'){
                pid = false;
                break;
            }
        }
        if(pid){
            pidTab[idx] = atoi(entry->d_name);
            idx++;
        }
    }
    // bubble sort O(n^2).. ok
    sort(idx, pidTab);
    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    for(int j = 0; j < idx; j++){
        char* currentStat = malloc(100*sizeof(char));
        char* pathCpy = malloc(100*sizeof(char));
        strcpy(pathCpy, PATH);
        sprintf(currentStat, "/%d/stat", pidTab[j]);
        strcat(pathCpy, currentStat);
        // printf("%s\n", pathCpy);

        FILE* fptr = fopen(pathCpy, "r");
        if(!fptr){
            printf("ERROR!\n");
            return -1;
        }
        int pid;
        int ppid;
        char* name = malloc(100* sizeof(char));
        char state;
        fscanf(fptr, "%d %s %c %d", &pid, name, &state, &ppid);

        name[strlen(name) - 1] = '\0';
        printf("%5d %5d %6c %s\n", pid, ppid, state, name + 1);

        free(name);
        free(currentStat);
        free(pathCpy);
        fclose(fptr);
        fflush(stdout);
    }
    free(pidTab);
    closedir(dir);
}
// -------------------------------------------------------------------------

Ukaz ukazi[] = {
    {"debug", "Help for the debug command (active when debug > 0) :\n -> 'debug'         : print the current debug level\n -> 'debug' <level> : change the current debug level\n", &debug},
    {"prompt", "Help for the prompt command : \n -> 'prompt'         : print the current prompt\n -> 'debug' <string> : change the current prompt to string\n", &prompt},
    {"status", "Help for the status command : \n -> 'status' : print the status of the last executed command\n ", &status},
    {"exit", "Help for the exit command : \n -> 'exit'          : close the shell with the last executed command exit status\n -> 'exit' <status> : exit the shell with the given exit status\n", &exitCustom},
    {"help", "Help for the help command :\n -> help <command_name> : display the help info for the specified command\n", NULL},
    {"print", "Help for the print command :\n -> print <args...> : print the given arguments to stdout without newline\n", &print},
    {"echo", "Help for the echo command :\n -> <echo args...> : print the given arguments to stdout and add newline\n", &echo},
    {"len", "Help for the len command :\n -> len <args...> : print the length of the arguments (total string length (number of characters))\n", &len},
    {"sum", "Help for the sum command :\n -> sum <args...> : print the sum of the arguments (the arguments must be whole numbers, it will fail on type error) \n", &sum},
    {"calc", "Help for the calc command :\n -> calc <arg1 op arg2> : calculate the given operation ('+', '-', '*', '/', '%%') between the two arguments\n", &calc},
    {"basename", "Help for the basename command :\n -> basename <path> : prints the base name of the given path (like command basename in bash)\n", &basename},
    {"dirname", "Help for the dirname command :\n -> dirname <path> : prints the dir name of the given path (like command dirname in bash)\n", &dirname},
    {"dirch", "Help for the dirch command :\n -> dirch <directory_path> : change the current working directory (root if not specified) \n", &dirch},
    {"dirwd", "Help for the dirwd command :\n -> dirwd <mode> : print the current working directory\n     modes : 'full' (print the whole path)\n           : 'base' (print the basename, this is default)\n", &dirwd},
    {"dirmk", "Help for the dirmk command :\n -> dirmk <directory_name> : make a directory with the given name\n", &dirmk},
    {"dirrm", "Help for the dirrm command :\n -> dirrm <directory_name> : remove the directory with the given name\n", &dirrm},
    {"dirls", "Help for the dirls command :\n -> dirls <directory_name> : print the names of the files in the given directory (default is cwd)\n", &dirls},
    {"rename", "Help for the rename command :\n -> rename <current_name new_name> : rename the file\n", &renameFile},
    {"unlink", "Help for the unlink command :\n -> unlink <name> : unlink the given file (remove only the directory entry)\n", &unlinkFile},
    {"remove", "Help for the remove command :\n -> remove <name> : remove the given file or directory\n", &removeFile},
    {"linkhard", "Help for the linkhard command :\n -> linkhard <destination name> : creates a hard link with the given name and destination\n", &linkhard},
    {"linksoft", "Help for the linksoft command :\n -> linksoft <destination name> : creates a symbolic (soft) link with the given name and destination\n", &linksoft},
    {"linkread", "Help for the linkread command :\n -> linkread <name> : print the destination of the symbolic (soft) link\n", &linkread},
    {"linklist", "Help for the linklist command :\n -> linklist <name> : print all hard links to a given file name in the current working directory\n", &linklist},
    {"cpcat", "Help for the cpcat command :\n -> cpcat <source destination> : cp & cat combined (see cpcat.c)\n", &cpcat},
    {"pid", "Help for the pid command :\n -> pid : print the shell process ID (pid)\n", &pid},
    {"ppid", "Help for the ppid command :\n -> pid : print the shell parent process ID (ppid)\n", &ppid},
    {"uid", "Help for the uid command :\n -> uid: print the user ID (uid) of the user, who is the owner of the shell process\n", &uid},
    {"euid", "Help for the euid command :\n -> euid: print the effective user ID (euid) of the user, who is the owner of the shell process\n", &euid},
    {"gid", "Help for the gid command :\n -> gid: print the group ID (gid), which the process is a part of\n", &gid},
    {"egid", "Help for the egid command :\n -> egid: print the effective group ID (egid), which the process is a part of\n", &egid},
    {"sysinfo", "Help for the sysinfo command :\n -> sysinfo: print the sysname, nodename, release, version and machine fields of the system information.\n", &sysinfo},
    {"proc", "Help for the proc command :\n -> proc <path>: set the path to the procfs data system, no argument: print the current path (default is /proc), command detects if the path does not exist and fails!\n", &proc},
    {"pids", "Help for the pids command :\n -> pids: print the PID-s of the current processes from the procfs data system\n", &pids},
    {"pinfo", "Help for the pinfo command :\n -> pinfo: print the info of the current prcesses (PID, PPID, STATE, NAME) from the stat file in procfs\n", &pinfo},
};

int executeBuiltin(Ukaz u, int* tokenCount, char** tokens, bool* ozadje){
    if(DEBUG_LEVEL > 0){
        if(*ozadje){
            printf("Executing builtin '%s' in background\n", tokens[0]);
        }else{
            printf("Executing builtin '%s' in foreground\n", tokens[0]);
        }
    }
    return u.fPtr(tokenCount, tokens);
}

int executeExternal(int* tokenCount, char** tokens, int* endingModifiers){
    printf("External command '");
    bool first = true;
    for(int i = 0; i < *tokenCount - *endingModifiers; i++){
        if(first){
            printf("%s", tokens[i]);
            first = false;
        }else{
            printf(" %s", tokens[i]);
        }
    } 
    printf("'\n");
    return 0;
}


int findBuiltin(int* tokenCount, char** tokens, bool* ozadje, int* endingModifiers){
    int steviloUkazov = sizeof(ukazi) / sizeof(Ukaz);
    int stUkDup = steviloUkazov;
    for(int i = 0; i < steviloUkazov; i++){
        if(strcmp(tokens[0], "help") == 0){
            if(*tokenCount == 2){
                for(int j = 0; j < stUkDup; j++){
                    if(strcmp(tokens[1], ukazi[j].ime) == 0){
                        printf("%s", ukazi[j].pomoc);
                        return 0;
                    }
                }
                printf("Command %s is not builtin!\n", tokens[1]);
                return 1;
            }else{
                // mal slabo ker je hard-codan, ukaz[5] je help za help
                printf("%s", ukazi[4].pomoc);
                return 0;
            }
        }
        if(strcmp(tokens[0], ukazi[i].ime) == 0){
            return executeBuiltin(ukazi[i], tokenCount, tokens, ozadje);
        }
    }
    return executeExternal(tokenCount, tokens, endingModifiers);
}

void parseTokens(int tokenCount, char** tokens){
    bool preusmeritevIzhoda = false;
    char* pIPtr = NULL;
    bool preusmeritevVhoda = false;
    char* pVPtr = NULL;
    bool ozadje = false;
    int tknCpy = tokenCount;
    
    int endingModifiers = 0;

    tokenCount -= 1;
    int diff = 3 < tokenCount - 1 ? 3 : tokenCount - 1;

    for(int i = 0; i < diff; i++){
        if(tokens[tokenCount][0] == '&' && strlen(tokens[tokenCount]) == 1){
            ozadje = true;
            endingModifiers++;
        }
        if(tokens[tokenCount][0] == '>'){
            preusmeritevIzhoda = true;
            pIPtr = tokens[tokenCount] + 1;
            endingModifiers++;
        }
        if(tokens[tokenCount][0] == '<'){
            preusmeritevVhoda= true;
            pVPtr = tokens[tokenCount] + 1;
            endingModifiers++;
        }
        tokenCount -= 1;
    }

    if(DEBUG_LEVEL > 0){
        if(preusmeritevVhoda){
            printf("Input redirect: '%s'\n", pVPtr);
        }
        if(preusmeritevIzhoda){
            printf("Output redirect: '%s'\n", pIPtr);
        }
        if(ozadje){
            printf("Background: 1\n");
        }
    }

    if(!ozadje){
        STATUS = findBuiltin(&tknCpy, tokens, &ozadje, &endingModifiers);
    }else{
        findBuiltin(&tknCpy, tokens, &ozadje, &endingModifiers);
    }
}

int tokenizeInput(char* imeLupine, char* line, char** tokens, char* toFree){
    int tokenCount = 0;
    int lineSize = strlen(line);
    if(line[lineSize - 1] == '\n') line[lineSize - 1] = '\0';

    LineInfo li = stringCleanup(line);
    toFree = li.line;
    if(DEBUG_LEVEL > 0) printf("Input line: '%s'\n", line);
    if(li.onlySpace) return 0;

    int i = 0;
    char* zacetek = li.line;
    bool niz = false;
    while(li.line[i] != '\0'){
        if(li.hasComment && i == li.commentPointer){
            goto END;
        }
        if(li.line[i] == '"'){
            niz = !niz;
        }else if (li.line[i] == ' ' && !niz){
            li.line[i] = '\0';
            tokens[tokenCount] = zacetek;
            tokenCount++;
            // nov zacektek bo ' ' znak ki je na i + 1
            zacetek = li.line + i + 1;
        }
        i++;
    }
    tokens[tokenCount] = zacetek;
    tokenCount++;

    END:
    // tle odstranimo še " "
    for(int i = 0; i < tokenCount; i++){
        char* ptr = tokens[i];
        if(*ptr == '"'){
            int len = strlen(ptr);
            ptr[len - 1] = '\0';
            tokens[i] = ptr += 1;
        }
    }
    if(DEBUG_LEVEL > 0){
        for(int j = 0; j < tokenCount; j++){
            printf("Token %d: '%s'\n", j, tokens[j]);
        }
    }
    return tokenCount;
}

int main(int argc, char** argv){
    char* imeLupine = "mysh> ";

    // Tabela tokenov
    char** tokens = malloc(MAX_LINE_SIZE*sizeof(char*));

    while(true){
        char* line = malloc(MAX_LINE_SIZE*sizeof(char));
        char* toFree = NULL;
        if(isatty(STDIN_FILENO)){
            printf("%s", imeLupine);
            fgets(line, MAX_LINE_SIZE, stdin);
            int tokenCount = tokenizeInput(imeLupine, line, tokens, toFree);
            if(tokenCount > 0) parseTokens(tokenCount, tokens);
        }else{
            if(fgets(line, MAX_LINE_SIZE, stdin) == NULL) break;
            // printf("-%s", line);
            int tokenCount = tokenizeInput(imeLupine, line, tokens, toFree);
            if(tokenCount > 0) parseTokens(tokenCount, tokens);
        }
        free(line);
        if(toFree != NULL) free(toFree);
    }
    return STATUS;
}