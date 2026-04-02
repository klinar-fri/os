#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

// Koliko je največja velikost enega ukaza -> vprašanje
#define MAX_LINE_SIZE 2048

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
        // TODO -> stringe ne sme odstarnit space-ov znotraj !!!
        if(line[p] == '"'){
            while (line[p] != '"' && line[p] != '\0'){
                newLine[nP++] = line[p++];
            }
            newLine[nP++] = line[p++];
        }
        while (line[p] != ' ' && line[p] != '\0'){
            newLine[nP++] = line[p++];
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

void tokenizeInput(char* imeLupine, char* line, char** tokens, char version){
    int tokenCount = 0;
    int lineSize = strlen(line);
    if(line[lineSize - 1] == '\n') line[lineSize - 1] = '\0';

    LineInfo li = stringCleanup(line);
    printf("Input line: '%s'\n", line);
    printf("Input line: '%s'\n", li.line);
    if(li.onlySpace) return;

    int i = 0;
    char* zacetek = li.line;
    while(li.line[i] != '\0'){
        if(li.hasComment && i == li.commentPointer){
            goto END;
        }
        // TODO -> za stringe da jih ne da narazen !!! 
        if(li.line[i] == ' '){
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
    for(int j = 0; j < tokenCount; j++){
        printf("Token %d: '%s'\n", j, tokens[j]);
    }
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
            tokenizeInput(imeLupine, line, tokens, 'I');
        }else{
            if(fgets(line, MAX_LINE_SIZE, stdin) == NULL) break;
            tokenizeInput(imeLupine, line, tokens, 'N');
        }
        free(line);
    }
}