#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

// runs a nonblocking get char function that returns -1 if there are no characters
// otherwise returns the character from stdin
char nonblockgetchar() {
    int flags = fcntl(0, 3, 0);
    fcntl(0, 4, flags | 2048);
    char tmp = getchar();
    fcntl(0, 4, flags ^ 2048);
    return tmp;
}

// this function takes in a character array of size 17 and runs the getchar function
// once if finds a M or m, it returns the size of the letters in the array
// The M,m  letters represent the end of the mouse input string. 
int proccessgetchar(char* arr) {
    int offset = 0;
    char a;

    // runs and infinite loop that keep reading input until a character is found
    // once that happens it starts populating the passed in array and terminates
    // once the end of the mouse string is found
    while (1==1) {
        a = nonblockgetchar();    
        if (a == -1)
            return 0;
        if (a == 'M' || a == 'm') {
            arr[offset] = a;
            return offset+1;
        }
        arr[offset] = a;
        offset++;
    }
}

// function that resets the terminal to normal after executing everything
void closer() {
    system("echo '\e[?1003l\e[?1015l\e[?1006l'");
    system("stty echo");
    system("stty icanon");
}

int main() {
    // initializes the terminal to allow for scanning mouse input strings
    system("stty -icanon");
    system("stty -echo");
    system("echo '\e[?1003h\e[?1015h\e[?1006h'");
    atexit (closer);

    // initializes all variables used in the code
    // the mouse string looks like \>]opCode;row;col;M 
    char* array = malloc(sizeof(char)*17);
    char* pch, *nsp;
    int size, opCode, row, col;
    char type;
    
    while (1==1) {
        // continuously runs until a string of size > 0 is returned
        size = proccessgetchar(array);
        if (size == 0)
            continue;
        nsp = array + sizeof(char)*3;

        // uses strtok to break apart the returned string into it's respective parts
        type = array[size-1];
        pch = strtok (nsp, ";");
        opCode = atoi(pch);
        pch = strtok (NULL, ";");
        col = atoi(pch);
        pch = strtok (NULL, "mM");
        row = atoi(pch);

        printf ("%d %d %d %c\n", opCode, col, row, type);
    }
    return 0;
}
