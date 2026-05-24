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
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

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

typedef struct{
    bool prVhoda;
    bool prIzhoda;
    int idxNizaVhod;
    int idxNizaIzhod;
} Preusmeritev;

int tokenizeInput(char* line, char** tokens, char* toFree);
void parseTokens(int tokenCount, char** tokens);

char** tabelaPrUkazov;
int stPrUkazov = 0;
bool cleared = false;

static int vgrajenStatus = -1;
static bool vgrajenKoncan = false;

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
        _exit(atoi(tokens[1]));
    }else{
        _exit(STATUS);
    }
    return 42;
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
        if(tokens[i] == NULL) continue;
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

int waitone(int* tokenCount, char** tokens){
    if(vgrajenKoncan){
        vgrajenKoncan = false;
        return vgrajenStatus;
    }

    pid_t pid = *tokenCount == 1 ? -1 : atoi(tokens[1]);
    int status;
    if(*tokenCount == 1){
        if(waitpid(-1,  &status, 0) < 0){
            // perror("ERROR");
            return 0;
        }
    }else{
        pid_t pid = atoi(tokens[1]);
        if(waitpid(pid, &status, 0) < 0){
            // perror("ERROR");
            return 0;
        }
    }
    if(WIFEXITED(status)){
        return WEXITSTATUS(status);
    }
    return 0;
}

int waitall(int* tokenCount, char** tokens){
    int statusZadnji = 0;
    if(vgrajenKoncan){
        vgrajenKoncan = false;
        return vgrajenStatus;
    }
    int status = 0;
    while(1){
        if(waitpid(-1, &status, 0) < 0){
            break;
        }
        if(WIFEXITED(status)){
            statusZadnji = WEXITSTATUS(status);
        }
    }
    return statusZadnji;
}

int startPipe(char* ukaz, int* novFd){
    // printf("%s\n", ukaz);
    char** tokensPipe = malloc(100 * sizeof(char*));
    ukaz = ukaz + 1;
    ukaz[strlen(ukaz) - 1] = '\0';
    int tokenCountPipe = tokenizeInput(ukaz, tokensPipe, NULL);

    int fd[2];
    pipe(fd);
    if(!fork()){
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if(tokenCountPipe > 0) parseTokens(tokenCountPipe, tokensPipe);
        _exit(0);
    }

    // Ni ravno po diagramu, lahko samo en fd prenesemo
    // drugega pa zapremo ze tukaj, isto v middlePipe
    close(fd[1]);
    *novFd = fd[0];
    free(tokensPipe);
    return 0;
}

int middlePipe(char* ukaz, int* novFd){
    // printf("  %s\n", ukaz);
    char** tokensPipe = malloc(100 * sizeof(char*));
    ukaz = ukaz + 1;
    ukaz[strlen(ukaz) - 1] = '\0';
    int tokenCountPipe = tokenizeInput(ukaz, tokensPipe, NULL);

    int fd[2];
    pipe(fd);
    if(!fork()){
        // tuki sem dal kar STDIN_FILENO : bolj pregledno kot 0 / 1
        dup2(*novFd, STDIN_FILENO);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if(tokenCountPipe > 0) parseTokens(tokenCountPipe, tokensPipe);
        _exit(0);
    }

    close(fd[1]);
    close(*novFd);
    *novFd = fd[0];
    free(tokensPipe);
    return 0;
}

int endPipe(char* ukaz, int* novFd){
    // printf("%s\n", ukaz);
    char** tokensPipe = malloc(100 * sizeof(char*));
    ukaz = ukaz + 1;
    ukaz[strlen(ukaz) - 1] = '\0';
    int tokenCountPipe = tokenizeInput(ukaz, tokensPipe, NULL);

    if(!fork()){
        dup2(*novFd, STDIN_FILENO);
        close(*novFd);
        if(tokenCountPipe > 0) parseTokens(tokenCountPipe, tokensPipe);
        _exit(0);
    }

    close(*novFd);
    free(tokensPipe);
    return 0;
}

int pipes(int* tokenCount, char** tokens){
    if(*tokenCount == 3){
        int fd = 0;
        startPipe(tokens[1], &fd);
        endPipe(tokens[2], &fd);
        wait(NULL);
        wait(NULL);
    }else{
        int fd = 0;
        startPipe(tokens[1], &fd);
        for(int i = 2; i < (*tokenCount - 1); i++){
            middlePipe(tokens[i], &fd);
        }
        endPipe(tokens[*tokenCount - 1], &fd);
        for(int i = 0; i < (*tokenCount - 1); i++){
            wait(NULL);
        }
    }
    return 0;
}

int weather(int* tokenCount, char** tokens){
    char* mesto = "Ljubljana";
    char* linkStart = "https://wttr.in/";
    char* linkEnd = "?format=%t@%C@%h";

    if(*tokenCount == 2){
        mesto = tokens[1];
    }
    char* link = calloc(200,sizeof(char));
    strcpy(link, linkStart);
    strcat(link, mesto);
    strcat(link, linkEnd);
    // printf("%s\n", link);

    int fd[2];
    pipe(fd);

    if(fork() == 0){
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO); 
        close(fd[1]);
        execlp("curl", "curl", "-s", link, NULL);
        exit(1);
    }
    close(fd[1]);

    char* info = calloc(200, sizeof(char));
    read(fd[0], info, 200);
    close(fd[0]);
    wait(NULL);

    // printf("%s\n", info);

    char* t = malloc(20 * sizeof(char));
    char* w = malloc(20 * sizeof(char));
    char* m = malloc(20 * sizeof(char));

    int i = 0;
    while(info[i] != '@'){
        t[i] = info[i];
        i++;
    }
    t[i] = '\0';
    i++;

    int j = 0;
    while(info[i] != '@'){
        w[j] = info[i];
        j++;
        i++;
    }
    w[j] = '\0';
    i++;

    int k = 0;
    while(info[i] != '\0'){
        m[k] = info[i];
        k++;
        i++;
    }
    m[k] = info[i];

    time_t trDatumInCas;
    time(&trDatumInCas);

    // char* datInCas = malloc(100 * sizeof(char));
    char* datInCas = ctime(&trDatumInCas);
    datInCas[strlen(datInCas) - 1] = '\0';

    char* output = malloc(200 * sizeof(char));
    sprintf(output, "%s, %s:\n\tTemperatue: %s\n\tWeather: %s\n\tHumidity: %s\n", mesto, datInCas, t, w, m);
    // printf("%s", output);
    printf("%s", output);
    fflush(stdout);

    free(t);
    free(w);
    free(m);
    free(link);
    free(info);
    free(output);
    return 0;
}

int lc(int* tokenCount, char** tokens){
    if(*tokenCount == 1){
        if(stPrUkazov > 0){
            printf("%s\n", tabelaPrUkazov[stPrUkazov - 1]);
            fflush(stdout);
        }else{
            printf("ERROR: Command history does not exist!\n");
            fflush(stdout);
            return 1;
        }
    }else if(*tokenCount == 2){
        if(tokens[1][0] == 'a' && strlen(tokens[1]) == 1){
            printf("Command history:\n");
            for(int i = 0; i < stPrUkazov; i++){
                printf("  %d: %s\n", stPrUkazov - i,  tabelaPrUkazov[i]);
            }
            fflush(stdout);
        }else if(tokens[1][0] == 'x' && strlen(tokens[1]) == 1){
            char** tokensLc = malloc(100 * sizeof(char*));
            if(stPrUkazov <= 0) return 1;
            if(strcmp("lc x", tabelaPrUkazov[stPrUkazov - 1]) == 0) return 0;
            if(strcmp("lc a", tabelaPrUkazov[stPrUkazov - 1]) == 0) return 0;
            int tokenCountLc = tokenizeInput(tabelaPrUkazov[stPrUkazov - 1], tokensLc, NULL);
            if(tokenCountLc > 0) parseTokens(tokenCountLc, tokensLc);
            free(tokensLc);
        }else{
            int idx = atoi(tokens[1]);
            int offset = stPrUkazov - 1 - idx;
            if(offset < 0){
                printf("ERROR: Index does not exist!\n");
                return 1;
            }
            printf("%s\n", tabelaPrUkazov[stPrUkazov - 1 - idx]);
            fflush(stdout);
        }
    }else if(*tokenCount == 3){
        if(tokens[1][0] == 'a' && strlen(tokens[1]) == 1){
            printf("Command history:\n");
            for(int i = 0; i < stPrUkazov; i++){
                printf("  %d: %s\n", stPrUkazov - i,  tabelaPrUkazov[i]);
            }
            fflush(stdout);
        }else if(tokens[1][0] == 'x' && strlen(tokens[1]) == 1){
            int idx = atoi(tokens[2]);
            int offset = stPrUkazov - 1 - idx;
            if(offset < 0){
                printf("ERROR: Index does not exist!\n");
                fflush(stdout);
                return 1;
            }

            char** tokensLc = malloc(100 * sizeof(char*));
            if(stPrUkazov <= 0) return 1;
            int tokenCountLc = tokenizeInput(tabelaPrUkazov[stPrUkazov - 1 - idx], tokensLc, NULL);
            if(tokenCountLc > 0) parseTokens(tokenCountLc, tokensLc);
            free(tokensLc);
        }
    }else{
        printf("ERROR: Expected maximum 2 arguments!\nGet usage information using command: help lc.\n");

    }
    return 0;
}

int clearHistory(int* tokenCount, char** tokens){
    int fd = open("history.log", O_TRUNC);
    if(fd < 0){
        int koda = errno;
        perror("ERROR");
        return koda;
    }
    cleared = true;
    stPrUkazov = 0;
    close(fd);
    return 0;
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
    {"waitone", "Help for the waitone command :\n -> waitone <pid>: waits for the child with the given PID\n                   if PID is empty, we wait for a random child process\n                   if the child does not exist, the exit status is 0, else the exit status is that of the child process\n", &waitone},
    {"waitall", "Help for the waitall command :\n -> waitall: waits for all children processes\n", &waitall},
    {"pipes", "Help for the pipes command :\n -> pipes <\"level_1\" \"level_2\" \"level_3\" ...>: make command pipelines (Don't forget \"\") [example]:\n    pipes \"cat /etc/passwd\" \"cut -d: -f7\" \"sort\" \"uniq -c\"\n     is equivalent to this pipeline in bash:\n    cat /etc/passwd | cut -d: -f7 | sort | uniq -c\n", &pipes},
    {"weather", "Help for the weather command :\n -> weather <City> : tells you the weather in specific City {default is : Ljubljana} (calls the wttr.in/City API)\n", &weather},
    {"lc", "Help for the lc command :\n -> lc <mode_option index_option>: display the last used command\n     mode: 'a' lists all recent commands\n     mode: 'x' executes the last command or command at index\n     index can specify history offset (0 - last, 1 - one before last, end so on)\n     Note that arguments are optional!\n", &lc},
    {"clearhistory", "Help for the clearhistory command :\n -> clearhistory : clears the shell history\n", &clearHistory},
};

void loadHistory(){
    FILE* fp = fopen("history.log", "a");
    while(true){
        char* str = malloc(256 * sizeof(char));
        if(fgets(str, 256, fp) == NULL){
            break;
        }
        tabelaPrUkazov[stPrUkazov] = str;
        tabelaPrUkazov[stPrUkazov][strlen(tabelaPrUkazov[stPrUkazov]) - 1] = '\0';
        stPrUkazov++;
    }
    fclose(fp);
}

void sigchildHandler(int signum){
    int pid, status, serrno;
    serrno = errno;
    while(1){
        pid = waitpid(-1, &status, WNOHANG);
        if(pid < 0){
            // perror ("waitpid");
            break;
        }
        if(pid == 0) break;
        //notice_termination (pid, status);
    }
    errno = serrno;
}

int executeBuiltin(Ukaz u, int* tokenCount, char** tokens, bool* ozadje, Preusmeritev* pr){
    if(DEBUG_LEVEL > 0){
        if(*ozadje){
            printf("Executing builtin '%s' in background\n", tokens[0]);
        }else{
            printf("Executing builtin '%s' in foreground\n", tokens[0]);
        }
    }

    int shranjenVhod = dup(STDIN_FILENO);
    int shranjenIzhod = dup(STDOUT_FILENO);

    if(pr->prVhoda){
        int fd = open(tokens[pr->idxNizaVhod], O_RDONLY, 0644);
        dup2(fd, STDIN_FILENO);
        close(fd);
        *tokenCount = *tokenCount - 1;
    }
    if(pr->prIzhoda){
        int fd = open(tokens[pr->idxNizaIzhod], O_CREAT | O_WRONLY | O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        *tokenCount = *tokenCount - 1;
    }

    if(!*ozadje){
        int koda = u.fPtr(tokenCount, tokens);
        dup2(shranjenVhod, STDIN_FILENO);
        dup2(shranjenIzhod, STDOUT_FILENO);
        close(shranjenVhod);
        close(shranjenIzhod);
        return koda;
    }

    // ali deluje v ozdaju, potreben test

    if(strcmp(tokens[0], "exit") == 0){
        int koda = STATUS;
        if(*tokenCount >= 2 && tokens[1] != NULL){
            koda = atoi(tokens[1]);
        }
        vgrajenStatus = koda;
        vgrajenKoncan = true;
        return koda; 
    }

    pid_t pid = fork();
    if(pid < 0) return -1;
    if(pid == 0){
        int koda = u.fPtr(tokenCount, tokens);
        _exit(koda);
    }

    return 0;
}

int executeExternal(int* tokenCount, char** tokens, int* endingModifiers, bool* ozadje, Preusmeritev* pr){
    if(DEBUG_LEVEL > 0){
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
    }

    fflush(stdout);
    int pid = fork();
    if(pid < 0){
        printf("ERROR!: fork error\n");
        return -1;
    }else if(pid == 0){
        if(pr->prVhoda){
            int fd = open(tokens[pr->idxNizaVhod], O_RDONLY, 0644);
            dup2(fd, STDIN_FILENO);
            close(fd);
            tokens[pr->idxNizaVhod] = NULL;
        }
        if(pr->prIzhoda){
            int fd = open(tokens[pr->idxNizaIzhod], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            tokens[pr->idxNizaIzhod] = NULL;
        }
        if(execvp(tokens[0], tokens) < 0){
            int koda = errno;
            perror("exec");
            exit(127);
        }
    }
    // razlika med izvajanjem v ozadju in ospredju je sledeča:
    // če izvajamo v ospredju, potem starš čaka na konec ukaza
    // če v ozadju pa starš ne čaka
    if(!*ozadje){
        int status;
        if(waitpid(pid, &status, 0) < 0){
            perror("ERROR (cakanje v ospredju)");
        }
        if(WIFEXITED(status)){
            return WEXITSTATUS(status);
        }
    }

    return 0;
}


int findBuiltin(int* tokenCount, char** tokens, bool* ozadje, int* endingModifiers, Preusmeritev* pr){
    int steviloUkazov = sizeof(ukazi) / sizeof(Ukaz);
    int stUkDup = steviloUkazov;
    for(int i = 0; i < steviloUkazov; i++){
        if(strcmp(tokens[0], "help") == 0){
            if(*tokenCount == 2 && strcmp(tokens[1], "help") != 0){
                for(int j = 0; j < stUkDup; j++){
                    if(strcmp(tokens[1], ukazi[j].ime) == 0){
                        printf("%s", ukazi[j].pomoc);
                        return 0;
                    }
                }
                printf("Command %s is not builtin!\n", tokens[1]);
                return 1;
            }else{
                // mal slabo ker je hard-codan, ukaz[4] je help za help
                printf("%s", ukazi[4].pomoc);
                printf("Builtin commands:");
                for(int i = 0; i < steviloUkazov; i++){
                    if(i % 3 == 0) printf("\n");
                    if(i == steviloUkazov -  1){
                        printf("%8s ", ukazi[i].ime);
                    }else{
                        printf("%8s, ", ukazi[i].ime);
                    }
                }
                printf("\nFor external command help use: man <command_name>, or man man, to open the Linux manual.\n");
                return 0;
            }
        }
        if(strcmp(tokens[0], ukazi[i].ime) == 0){
            return executeBuiltin(ukazi[i], tokenCount, tokens, ozadje, pr);
        }
    }
    return executeExternal(tokenCount, tokens, endingModifiers, ozadje, pr);
}

void parseTokens(int tokenCount, char** tokens){
    char* pIPtr = NULL;
    char* pVPtr = NULL;
    bool ozadje = false;
    int tknCpy = tokenCount;

    Preusmeritev pr;
    pr.prIzhoda = false;
    pr.prVhoda = false;
    pr.idxNizaVhod = 0;
    pr.idxNizaIzhod = 0;
    
    int endingModifiers = 0;

    int idx = tokenCount - 1;
    int diff = idx + 1 < 3 ? idx + 1 : 3;

    for(int i = 0; i < diff; i++){
        if(tokens[idx][0] == '&' && strlen(tokens[idx]) == 1){
            ozadje = true;
            endingModifiers++;
            tokens[idx] = NULL;
        }else if(tokens[idx][0] == '>'){
            pIPtr = tokens[idx] + 1;
            endingModifiers++;
            tokens[idx] = tokens[idx] + 1;
            pr.prIzhoda = true;
            pr.idxNizaIzhod = idx;
        }else if(tokens[idx][0] == '<'){
            pVPtr = tokens[idx] + 1;
            endingModifiers++;
            tokens[idx] = tokens[idx] + 1;
            pr.prVhoda = true;
            pr.idxNizaVhod = idx;
        }else break;
        idx--;
    }

    if(DEBUG_LEVEL > 0){
        if(pr.prVhoda){
            printf("Input redirect: '%s'\n", pVPtr);
        }
        if(pr.prIzhoda){
            printf("Output redirect: '%s'\n", pIPtr);
        }
        if(ozadje){
            printf("Background: 1\n");
        }
    }

    if(!ozadje){
        STATUS = findBuiltin(&tknCpy, tokens, &ozadje, &endingModifiers, &pr);
    }else{
        findBuiltin(&tknCpy, tokens, &ozadje, &endingModifiers, &pr);
    }
}

int tokenizeInput(char* line, char** tokens, char* toFree){
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

    if(tokenCount > 0 && strcmp(tokens[0], "pipes") != 0){
        // tle odstranimo še " "
        for(int i = 0; i < tokenCount; i++){
            char* ptr = tokens[i];
            if(*ptr == '"'){
                int len = strlen(ptr);
                ptr[len - 1] = '\0';
                tokens[i] = ptr += 1;
            }
        }
    }

    if(DEBUG_LEVEL > 0){
        for(int j = 0; j < tokenCount; j++){
            printf("Token %d: '%s'\n", j, tokens[j]);
        }
    }
    
    tokens[tokenCount] = NULL;
    return tokenCount;
}

int main(int argc, char** argv){
    char* imeLupine = "mysh> ";

    // Tabela tokenov
    char** tokens = malloc(MAX_LINE_SIZE*sizeof(char*));

    tabelaPrUkazov = malloc(1000 * sizeof(char*));
    stPrUkazov = 0;
    loadHistory();

    int datotekaZgodovina = open("history.log", O_CREAT | O_WRONLY | O_APPEND, 0644);

    signal(SIGCHLD, sigchildHandler);

    while(true){
        char* line = malloc(MAX_LINE_SIZE*sizeof(char));
        char* toFree = NULL;
        if(isatty(STDIN_FILENO)){
            printf("%s", imeLupine);
            fgets(line, MAX_LINE_SIZE, stdin);
            int tokenCount = tokenizeInput(line, tokens, toFree);
            if(tokenCount > 0) parseTokens(tokenCount, tokens);
        }else{
            if(fgets(line, MAX_LINE_SIZE, stdin) == NULL) break;
            // printf("-%s", line);
            int tokenCount = tokenizeInput(line, tokens, toFree);
            if(tokenCount > 0) parseTokens(tokenCount, tokens);
        }

        if(cleared == false){
            tabelaPrUkazov[stPrUkazov] = malloc(1000 * sizeof(char));
            strcpy(tabelaPrUkazov[stPrUkazov], line);

            // zgodovina iz datoteke
            write(datotekaZgodovina, tabelaPrUkazov[stPrUkazov], strlen(tabelaPrUkazov[stPrUkazov]));
            write(datotekaZgodovina, "\n", 1);

            stPrUkazov++;
            if(stPrUkazov == 990){
                clearHistory(0, NULL);
            }
            free(line);
            if(toFree != NULL) free(toFree);
        }else{
            cleared = false;
        }
    }


    for(int i = 0; i < stPrUkazov; i++){
        // printf("%s\n", tabelaPrUkazov[i]);
        free(tabelaPrUkazov[i]);
    }
    free(tabelaPrUkazov);
    close(datotekaZgodovina);

    return STATUS;
}