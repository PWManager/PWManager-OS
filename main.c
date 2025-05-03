// PWManager OS

#define TRUE       1
#define FALSE      0

void put(char c) {
    __asm__ volatile ("movb %0, %%al" : : "r" (c));
}

void puts(char *s) {
    int i;
    for (i = 0; *(s + i) != '\0'; ++i) {
        put(*(s + i));
    }
}

char get() {
    char c;
    __asm__ volatile ("movb %%al, %0" : "=r" (c));
    return c;
}

char gets(char *s) {
    int i;
    for (i = 0; (s[i] = get()) != '\n'; ++i);
    return *(s + i) = '\0';
}

int _start() {
    char command[100];
    put('>>');
    gets(command);

    if (command == "help") {
        puts("help");
    } else {
        puts("Wrong command!");
    }
}