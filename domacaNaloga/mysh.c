#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_SIZE 1024 
int DEBUG_LEVEL = 0;

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

int executeExternal(int* tokenCount, char** tokens){
    printf("External command '");
    bool first = true;
    for(int i = 0; i < *tokenCount; i++){
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

Ukaz ukazi[] = {
    {"debug", "pomoc ni implementirana!", &debug},
    // {"prompt", "pomoc ni implementirana!", prompt}
};

int findBuiltin(int* tokenCount, char** tokens, bool* ozadje){
    int steviloUkazov = sizeof(ukazi) / sizeof(Ukaz);
    for(int i = 0; i < steviloUkazov; i++){
        if(strcmp(tokens[0], ukazi[i].ime) == 0){
            return executeBuiltin(ukazi[i], tokenCount, tokens, ozadje);
        }
    }
    return executeExternal(tokenCount, tokens);
}

void parseTokens(int tokenCount, char** tokens){
    bool preusmeritevIzhoda = false;
    char* pIPtr = NULL;
    bool preusmeritevVhoda = false;
    char* pVPtr = NULL;
    bool ozadje = false;
    int tknCpy = tokenCount;

    tokenCount -= 1;
    int diff = 3 < tokenCount - 1 ? 3 : tokenCount - 1;

    for(int i = 0; i < diff; i++){
        if(tokens[tokenCount][0] == '&' && strlen(tokens[tokenCount]) == 1){
            ozadje = true;
        }
        if(tokens[tokenCount][0] == '>'){
            preusmeritevIzhoda = true;
            pIPtr = tokens[tokenCount] + 1;
        }
        if(tokens[tokenCount][0] == '<'){
            preusmeritevVhoda= true;
            pVPtr = tokens[tokenCount] + 1;
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

    int status = findBuiltin(&tknCpy, tokens, &ozadje);
    // printf("STATUS: %d\n", status);
}

int tokenizeInput(char* imeLupine, char* line, char** tokens, char version){
    int tokenCount = 0;
    int lineSize = strlen(line);
    if(line[lineSize - 1] == '\n') line[lineSize - 1] = '\0';

    LineInfo li = stringCleanup(line);
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
        if(isatty(STDIN_FILENO)){
            printf("%s", imeLupine);
            fgets(line, MAX_LINE_SIZE, stdin);
            int tokenCount = tokenizeInput(imeLupine, line, tokens, 'I');
            parseTokens(tokenCount, tokens);
        }else{
            if(fgets(line, MAX_LINE_SIZE, stdin) == NULL) break;
            int tokenCount = tokenizeInput(imeLupine, line, tokens, 'N');
            parseTokens(tokenCount, tokens);
        }
        free(line);
    }
}