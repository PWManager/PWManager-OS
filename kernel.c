#include <stdint.h>
#include <stddef.h>

// --- Определения ---
#define DEFAULT_TEXT_COLOR 0x07
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
unsigned char current_color = DEFAULT_TEXT_COLOR;

// --- Inline ASM: Порты ---
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    if (*str == '-') { // Если отрицательное число
        sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// --- Multiboot Header ---
__attribute__((section(".multiboot"), used))
volatile uint32_t multiboot_header[] = {
    0x1BADB002,
    0x0,
    -(0x1BADB002)
};

// --- VGA ---
volatile char* video_memory = (volatile char*) 0xB8000;
int cursor_x = 0;
int cursor_y = 0;

void update_cursor() {
    unsigned short pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// --- Wait Function ---
void wait_simple(uint32_t ms) {
    uint32_t iterations = ms * 1000000;  // 1 миллисекунда ~ 1 миллион итераций
    for (uint32_t i = 0; i < iterations; i++) {
        __asm__ volatile("nop");  // ничего не делаем, просто тратим время
    }
}

// --- Screen and VGA ---
void scroll() {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_memory[((y - 1) * VGA_WIDTH + x) * 2] = video_memory[(y * VGA_WIDTH + x) * 2];
            video_memory[((y - 1) * VGA_WIDTH + x) * 2 + 1] = video_memory[(y * VGA_WIDTH + x) * 2 + 1];
        }
    }

    // Очистить последнюю строку
    for (int x = 0; x < VGA_WIDTH; x++) {
        video_memory[((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2] = ' ';
        video_memory[((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2 + 1] = current_color;
    }

    if (cursor_y > 0)
        cursor_y--;
}

void clear_screen() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = current_color;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void set_color(unsigned char color) {
    current_color = color;
    // Мягкая смена цвета
    for (int step = 0; step <= 10; step++) {
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            video_memory[i * 2 + 1] = color;
        }
        for (volatile int j = 0; j < 10000; j++) { __asm__ __volatile__("nop"); }
    }
}

void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            video_memory[(cursor_y * VGA_WIDTH + cursor_x) * 2] = ' ';
            video_memory[(cursor_y * VGA_WIDTH + cursor_x) * 2 + 1] = current_color;
        }
    } else {
        video_memory[(cursor_y * VGA_WIDTH + cursor_x) * 2] = c;
        video_memory[(cursor_y * VGA_WIDTH + cursor_x) * 2 + 1] = current_color;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_cursor();
}

void print(const char* str) {
    for (int i = 0; str[i] != 0; i++) {
        print_char(str[i]);
    }
}

// --- Keyboard ---
unsigned char keyboard_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',

};

char get_key() {
    unsigned char status;
    char key = 0;
    do {
        status = inb(0x64);
        if (status & 0x01) {
            key = inb(0x60);
            if (key & 0x80) {
                // отпускание
            } else {
                return keyboard_map[key];
            }
        }
    } while (1);
}

// --- Utils ---
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    } else {
        return *(unsigned char *)s1 - *(unsigned char *)s2;
    }
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// --- System ---
void reboot() {
    outb(0x64, 0xFE);
hang:
    __asm__ __volatile__("hlt");
    goto hang;
}

void shutdown() {
    // Выключаем процессор
    __asm__ __volatile__("cli; hlt");
}

void kernel_panic() {
    print("System error has occur!\n");
    print("Press any key to reboot...");
    while (1) {
        if (get_key() != 0) {
            reboot();
        }
    }
}

// --- Shell ---
void shell() {
    char buffer[128];
    int index = 0;
    print("> ");
    while (1) {
        char c = get_key();
        if (c == '\n') {
            buffer[index] = '\0';
            print_char('\n');

            if (index == 0) {
                // пусто
            } else if (strcmp(buffer, "help") == 0) {
                print("Available commands:\n");
                print("help      - show this message\n");
                print("clear     - clear the screen\n");
                print("about     - about this OS\n");
                print("reboot    - reboot the system\n");
                print("shutdown  - power off the CPU\n");
                print("color XY  - set background/text color\n");
                print("wait X    - wait for X milliseconds\n");
            } else if (strcmp(buffer, "clear") == 0) {
                clear_screen();
            } else if (strcmp(buffer, "about") == 0) {
                print("PWManager OS. Written by Michael\n");
            } else if (strcmp(buffer, "reboot") == 0) {
                print("Rebooting...\n");
                reboot();
            } else if (strcmp(buffer, "shutdown") == 0) {
                break;
            } else if (strncmp(buffer, "color ", 6) == 0) {
                char* arg = buffer + 6;
                if (strcmp(arg, "-r") == 0) {
                    set_color(DEFAULT_TEXT_COLOR);
                    print("Color reset to default.\n");
                } else if (strlen(arg) == 2) {
                    int bg = hex_char_to_int(arg[0]);
                    int fg = hex_char_to_int(arg[1]);
                    if (bg == -1 || fg == -1) {
                        print("Invalid color code.\n");
                    } else if (bg == fg) {
                        kernel_panic();
                    } else {
                        unsigned char color = (bg << 4) | fg;
                        set_color(color);
                        print("Color changed.\n");
                    }
                }
            } else if (strncmp(buffer, "wait ", 5) == 0) {
                uint32_t ms = atoi(&buffer[5]);
                wait_simple(ms);
            } else {
                print("Unknown command\n");
            }
            index = 0;
            print("> ");
        } else if (c == '\b') {
            if (index > 0) {
                index--;
                print_char('\b');
                print_char(' ');
                print_char('\b');
            }
        } else {
            if (index < sizeof(buffer) - 1) {
                buffer[index] = c;
                index++;
                print_char(c);
            }
        }
    }
}

// --- Main ---
void _start() {
    clear_screen();
    print("Welcome to PWManager OS!\n");
    shell();
}
