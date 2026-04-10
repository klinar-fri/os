#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#define MAX_LINE_SIZE 1024 
int DEBUG_LEVEL = 0;
char* PROMPT = "mysh";
int STATUS = 0;

typedef struct{
    const char* ime;
    const char* pomoc;
    // kazalec na funkcijo
    int (*fPtr)();
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
    assert(*tokenCount <= 2);
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
    return 0;
}

int status(){
    printf("%d\n", STATUS);
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
        printf("%s\n", tokens[1] + lastSp + 1);
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
        tokens[1][lastSp] = '\0';
        printf("%s\n", tokens[1]);
        return 0;
    }else{
        // printf("ERROR! : expected : basename <path>!\n");
        return 1;
    }
}

int dirch(int* tokenCount, char** tokens){
    if(*tokenCount == 1){
        if(chdir("/") < 0){
            // perror("ERROR! ");
            return errno;
        }
    }else{
        if(chdir(tokens[1]) < 0){
            // perror("ERROR! ");
            return errno;
        }
    }
    return 0;
}

int dirwd(int* tokenCount, char** tokens){

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
};

int executeBuiltin(Ukaz u, int* tokenCount, char** tokens, bool* ozadje){
    if(DEBUG_LEVEL > 0){
        if(*ozadje){
            printf("Executing builtin '%s' in background\n", tokens[0]);
        }else{
            printf("Executing builtin '%s' in foreground\n", tokens[0]);
        }
    }
    u.fPtr(tokenCount, tokens);
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