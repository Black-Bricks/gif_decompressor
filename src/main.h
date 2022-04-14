#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define LOGLEVEL_DEBUG      4
#define LOGLEVEL_INFO       3
#define LOGLEVEL_WARN       2
#define LOGLEVEL_ERROR      1
#define LOGLEVEL_OFF        0
#define LOGLEVEL            LOGLEVEL_OFF

#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V

#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter format LOG_RESET_COLOR "\n"

#define LOGD(format,__VA_ARGS__...) do { \
        if(LOGLEVEL >= LOGLEVEL_DEBUG) printf(LOG_FORMAT(D, format), ##__VA_ARGS__); \
    } while(0)
#define LOGI(format,__VA_ARGS__...) do { \
        if(LOGLEVEL >= LOGLEVEL_INFO) printf(LOG_FORMAT(I, format), ##__VA_ARGS__); \
    } while(0)
#define LOGW(format,__VA_ARGS__...) do { \
        if(LOGLEVEL >= LOGLEVEL_WARN) printf(LOG_FORMAT(W, format), ##__VA_ARGS__); \
    } while(0)
#define LOGE(format,__VA_ARGS__...) do { \
        if(LOGLEVEL >= LOGLEVEL_ERROR) printf(LOG_FORMAT(E, format), ##__VA_ARGS__); \
    } while(0)

#endif // MAIN_H