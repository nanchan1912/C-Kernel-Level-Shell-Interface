#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/fcntl.h>
#include<pwd.h>
#include<string.h>
#include <ctype.h> // For isspace
#include "a3.h"

int spacer(char c){
    if(isspace(c)){
        return 1;
    }
    return 0;
}

//"&&", ">>", "|", "&", "<", ">", ";"
int special(char *s){
    if(!s){
        return 0;
    }
    else if(strcmp(s, "&&")==0){
        return 1;
    }
    else if(strcmp(s, ">>")==0){
        return 1;
    }
    else if(strcmp(s, "<<")==0){
        return 1;
    }
    else if(strcmp(s, "|")==0){
        return 1;
    }
    else if(strcmp(s, "&")==0){
        return 1;
    }
    else if(strcmp(s, "<")==0){
        return 1;
    }
    else if(strcmp(s, ">")==0){
        return 1;
    }
    else if(strcmp(s, ";")==0){
        return 1;
    }
    return 0;
}

// Global variable definitions for tokens and token_count
char tokens[1000][1000];
int token_count = 0;

void part_part(char* input) {
    token_count = 0;
    int len = strlen(input);
    int i = 0;
    
    while (i < len) {
        // Skip leading whitespace
        while (i < len && spacer(input[i])) {
            i++;
        }
        if (i >= len) break;
        
        // Handle multi-character tokens first
        if (i + 1 < len) {
            if (input[i] == '&' && input[i+1] == '&') {
                strcpy(tokens[token_count++], "&&");
                i += 2;
                continue;
            } else if (input[i] == '>' && input[i+1] == '>') {
                strcpy(tokens[token_count++], ">>");
                i += 2;
                continue;
            } else if (input[i] == '<' && input[i+1] == '<') {
                strcpy(tokens[token_count++], "<<");
                i += 2;
                continue;
            }
        }
        
        // Handle single-character tokens
        if (input[i] == '|' || input[i] == ';' || input[i] == '&' || input[i] == '<' || input[i] == '>') {
            tokens[token_count][0] = input[i];
            tokens[token_count][1] = '\0';
            token_count++;
            i++;
            continue;
        }
        
        // Handle regular names/words
        int start = i;
        while (i < len && !spacer(input[i]) && input[i] != '|' && input[i] != ';' && input[i] != '&' && input[i] != '<' && input[i] != '>') {
            i++;
        }
        strncpy(tokens[token_count], input + start, i - start);
        tokens[token_count][i - start] = '\0';
        token_count++;
    }
}

char* current_token(int pos){
    if(pos >= token_count) {
        return NULL; 
    }
    return tokens[pos];
}

int name(char* token){
    if(special(token) == 1){
        return 0;
    }
    return 1;
}

int input(int pos){
    if((pos > 0) && (strcmp(current_token(pos), "<")== 0 ) ||(strcmp(current_token(pos), "<<")== 0 ) && name(current_token(pos + 1))){
        return 1;
    }
    return 0;
}

int output(int pos){
    if( (pos > 0) && ((strcmp(current_token(pos), ">")== 0 ) ||(strcmp(current_token(pos), ">>")== 0 ) ) && name(current_token(pos + 1))){
        return 1;
    }
    return 0;
}

int pos = 0;

int atomic_grp(){
    if(!name(current_token(pos))){
        return 0;
    }
    pos++; // first name can be consumed

    while(current_token(pos) != NULL ){
        if(name(current_token(pos))){
            pos++; // consume all names
        }
        else if(input(pos)){
            pos = pos +2;
        }
        else if(output(pos)){
            pos = pos + 2;
        }
        else{
            break;
        }
    } 
    return 1;
}

int cmd_group(){
    if(!atomic_grp()){
        return 0;
    }
    while(current_token(pos) != NULL && (strcmp(current_token(pos), "|")==0)){
        pos++;
        if(!atomic_grp()){
            return 0;
        }
    }
    return 1;
}
int shell_cmd(){
    if(!cmd_group()){
        return 0;
    }
    while(current_token(pos) != NULL){
        if(strcmp(current_token(pos), "&&") == 0 || strcmp(current_token(pos), ";") == 0){ // Add || strcmp(current_token(pos), ";") == 0
            pos++;
            if(!cmd_group()){
                return 0;
            }
        } 
        else if(strcmp(current_token(pos), "&") == 0){
            pos++;
            if(current_token(pos) != NULL){
                if(!cmd_group()){
                    return 0;
                }
            }
            break;
        }
        else {
            break;
        }
    }
    return 1;
}
