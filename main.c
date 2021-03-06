#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <curl/curl.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define THREAD_COUNT 2
#define MIN_BUFF_SIZE (1024 >> 1)
#define MED_BUFF_SIZE (1024 << 2)
#define MAX_BUFF_SIZE (1024 << 10)

typedef struct {
    char script[MIN_BUFF_SIZE];
    char cookies[MIN_BUFF_SIZE];
    char proxy[MIN_BUFF_SIZE];
    bool check_mode;
    bool quiet;
    bool logger;
} global_config;

typedef struct {
    char func[MIN_BUFF_SIZE];
    char url[MED_BUFF_SIZE];
    char html[MAX_BUFF_SIZE];
    char nexturl[MED_BUFF_SIZE];
    char files[MED_BUFF_SIZE];
    bool stop;
} global_script;

static bool shutdown_flag = false;
static FILE *logger = NULL;
static global_config config = {{0}, {0}, {0}, false, false, false};

static void print_log(const char *format, ...) {
    if(!config.quiet || config.logger) {
        char message[MED_BUFF_SIZE];
        char buff[MIN_BUFF_SIZE];
        time_t now = time(NULL);
        va_list ap;
        strftime(buff, sizeof(buff), "%m-%d %H:%M:%S", localtime(&now));
        va_start(ap, format);
        vsnprintf(message, sizeof(message), format, ap); 
        va_end(ap);
        if(!config.quiet)
            printf("[%s] %s\n", buff, message);
        if(config.logger && logger != NULL)
            fprintf(logger, "[%s] %s\n", buff, message);
    }
}

static size_t _curl_write_buff(const char *data, size_t size, size_t count, void *buff) {
    if(buff == NULL) return 0;
    size_t len = size * count;
    strncat(buff, data, len);
    return len;
}

static size_t _curl_write_file(const void *data, size_t size, size_t count, FILE *file) {
    if(file == NULL) return 0;
    size_t len = fwrite(data, size, count, file);
    return len;
}

static size_t (*curl_write_buff)(const char *data, size_t size, size_t count, void *buff) = _curl_write_buff;
static bool curl_buff(const char *url, char *buff) {
    if(buff == NULL) return false; else *buff = 0;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(strstr(url, "https://") != NULL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    if(strlen(config.cookies) != 0)
        curl_easy_setopt(curl, CURLOPT_COOKIE, config.cookies);
    if(strlen(config.proxy) != 0)
        curl_easy_setopt(curl, CURLOPT_PROXY, config.proxy);
    if(strstr(config.proxy, "http://") != NULL && strstr(url, "http://") == NULL)
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
#if defined(__linux) || defined(__linux__) || defined(linux)
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_buff);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff);
    bool ret =  curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return ret == 0;
}

static size_t (*curl_write_file)(const void *data, size_t size, size_t count, FILE *file) = _curl_write_file;
static bool curl_file(const char *url, FILE *file) {
    if(file == NULL) return false;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(strstr(url, "https://") != NULL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    if(strlen(config.cookies) != 0)
        curl_easy_setopt(curl, CURLOPT_COOKIE, config.cookies);
    if(strlen(config.proxy) != 0)
        curl_easy_setopt(curl, CURLOPT_PROXY, config.proxy);
    if(strstr(config.proxy, "http://") != NULL && strstr(url, "http://") == NULL)
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
#if defined(__linux) || defined(__linux__) || defined(linux)
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#endif
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    bool ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return ret == 0;
}

static bool execute_script(lua_State *L, const char *func, char *url, char *html, char *nexturl, char *files, bool *stop) {
    if(lua_getglobal(L, func) != LUA_TFUNCTION) {
        print_log("* error: %s is nil or not a function", func);
        lua_pop(L, 1);
        return false;
    }
    lua_pushstring(L, url);
    lua_pushstring(L, html);
    if(lua_pcall(L, 2, 3, 0)) {
        print_log("* error: a runtime exception occurs in function %s", func);
        lua_pop(L, 1);
        return false;
    }
    if(!lua_isboolean(L, -1) || !lua_isstring(L, -2) || !lua_isstring(L, -3)) {
        print_log("* error: function %s returns nil or incorrect type", func);
        lua_pop(L, 3);
        return false;
    }
    *stop = lua_toboolean(L, -1);
    strcpy(files, lua_tostring(L, -2));
    strcpy(nexturl, lua_tostring(L, -3));
    lua_pop(L, 3);
    return true;
}

static void *handle_loop(void *param) {
    global_script script = {"parse_html", {0}, {0}, {0}, {0}, false};
    strncpy(script.url, param, sizeof(script.url));
    if(strlen(config.script) == 0) {
        if(curl_buff(script.url, script.html))
            printf("%s\n", script.html);
        else
            print_log("* error: %s is not reachable", script.url);
        pthread_exit(NULL);
        return NULL;
    }
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if(luaL_dofile(L, config.script)) {
        print_log("* error: %s can not be loaded", config.script);
        lua_close(L);
        pthread_exit(NULL);
        return NULL;
    }
    while(!script.stop && !shutdown_flag) {
        if(strlen(script.nexturl) != 0)
            strncpy(script.url, script.nexturl, sizeof(script.url));
        *(script.html) = 0;
        if(curl_buff(script.url, script.html)) {
            *(script.nexturl) = 0;
            *(script.files) = 0;
            script.stop = false;
            if(!execute_script(L, script.func, script.url, script.html, script.nexturl, script.files, &script.stop))
                break;
            print_log("> url: %s", script.url);
            print_log("< nexturl: %s, stop: %d", script.nexturl, script.stop);
            print_log("< files: %s", script.files);
            if(!config.check_mode) {
                char *fileurl, *filename;
                char *token = strtok(script.files, " ");
                while(token != NULL) {
                    fileurl = token;
                    token = strtok(NULL, " ");
                    if(token != NULL) {
                        filename = token;
                        FILE *file = fopen(filename, "wb");
                        if(file != NULL) {
                            if(curl_file(fileurl, file))
                                print_log("+ done: %s", filename);
                            else
                                print_log("+ error: %s is not reachable", fileurl);
                            fclose(file);
                        } else
                            print_log("+ error: %s can not be opened", filename);
                        token = strtok(NULL, " ");
                    }
                }
            }
        } else {
            print_log("* error: %s is not reachable", script.url);
            break;
        }
    }
    lua_close(L);
    pthread_exit(NULL);
    return NULL;
}

void exit_hook(int number) {
    shutdown_flag = true;
    printf("shut down now, exit signal: %d\n", number);
}

static bool load_config(char *config_file) {
    lua_State *L = luaL_newstate();
    if(luaL_dofile(L, config_file))
        return false;
    if(lua_getglobal(L, "script") == LUA_TSTRING) {
        strcpy(config.script, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    if(lua_getglobal(L, "cookies") == LUA_TSTRING) {
        strcpy(config.cookies, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    if(lua_getglobal(L, "proxy") == LUA_TSTRING) {
        strcpy(config.proxy, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    if(lua_getglobal(L, "check_mode") == LUA_TBOOLEAN) {
        config.check_mode = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    if(lua_getglobal(L, "quiet") == LUA_TBOOLEAN) {
        config.quiet = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    if(lua_getglobal(L, "logger") == LUA_TBOOLEAN) {
        config.logger = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    lua_close(L);
    return true;
}

static void usage(const char *name) {
    printf("Usage: %s [-s luascript] [-k cookies] [-p proxy] [-u] [-c] [-q] [-l] [-h] <url> <url> ...\n", name);
    printf("  -s: which lua script to execute\n");
    printf("  -k: cookies used to access url with logging and more features\n");
    printf("  -p: support any proxy, include a HTTP proxy tunnel, example: \"http://127.0.0.1:7777\"\n");
    printf("  -u: get the latest release and more lua scripts\n");
    printf("  -c: check mode for testing lua script, whithout downloading any files\n");
    printf("  -q: quiet mode, do not write output to stdout\n");
    printf("  -l: logger mode, write output to log file\n");
    printf("  -h: show help information\n");
}

int main(int argc, char **argv) {
    if(argc == 1) {
        usage(argv[0]);
        return 1;
    }

    char config_file[MIN_BUFF_SIZE] = "config.lua";
    char logger_file[MIN_BUFF_SIZE] = "stat.log";
#if defined(__linux) || defined(__linux__) || defined(linux)
    char home[MIN_BUFF_SIZE] = {0};
    char dirname[MIN_BUFF_SIZE] = {0};
    char filename[MIN_BUFF_SIZE] = {0};
    strcpy(home, getenv("HOME"));
    strcpy(dirname, basename(argv[0]));
    strcpy(filename, config_file);
    sprintf(config_file, "%s/.%s/%s", home, dirname, filename);
    if(load_config(config_file)) {
        strcpy(filename, logger_file);
        sprintf(logger_file, "%s/.%s/%s", home, dirname, filename);
        if(strlen(config.script) != 0) {
            strcpy(filename, config.script);
            sprintf(config.script, "%s/.%s/%s", home, dirname, filename);
        }
    }
#else
    load_config(config_file);
#endif

    int opt = 0;
    while((opt = getopt(argc, argv, "s:k:p:ucqlh")) != -1) {
        switch(opt) {
            case 's':
                strcpy(config.script, optarg);
                break;
            case 'k':
                strcpy(config.cookies, optarg);
                break;
            case 'p':
                strcpy(config.proxy, optarg);
                break;
            case 'u':
                curl_global_init(CURL_GLOBAL_ALL);
                char buff[MED_BUFF_SIZE] = {0};
                if(curl_buff("https://raw.githubusercontent.com/qianngchn/nextspider/master/scripts/list.lua", buff))
                    printf("%s", buff);
                curl_global_cleanup();
                return 1;
                break;
            case 'c':
                config.check_mode = true;
                break;
            case 'q':
                config.quiet = true;
                break;
            case 'l':
                config.logger = true;
                break;
            case 'h':
                usage(argv[0]);
            default:
                return 1;
        }
    }
    if(optind == argc) {
        printf("need arguments after options\n");
        return 1;
    }

    if(config.logger && logger == NULL)
        logger = fopen(logger_file, "ab");
    print_log("* script: %s, cookies: %s, proxy: %s, check_mode: %d, quiet: %d, logger: %d", strlen(config.script) ? config.script : "none", strlen(config.cookies) ? config.cookies : "none", strlen(config.proxy) ? config.proxy : "none", config.check_mode, config.quiet, config.logger);
    print_log("* thread count: %d", THREAD_COUNT);

    signal(SIGINT, exit_hook);
    signal(SIGTERM, exit_hook);
#if defined(__linux) || defined(__linux__) || defined(linux)
    signal(SIGKILL, exit_hook);
    signal(SIGQUIT, exit_hook);
    signal(SIGHUP, exit_hook);
#endif

    int i = 0, j = 0;
    pthread_t tid[THREAD_COUNT];
    curl_global_init(CURL_GLOBAL_ALL);
    for(i = 0; i < argc - optind && !shutdown_flag; i = i + THREAD_COUNT) {
        for(j = 0; j < THREAD_COUNT; j++) {
            if(pthread_create(tid + j, NULL, handle_loop, argv[optind + i + j]) != 0) {
                print_log("* error: thread for %s can not be created", argv[optind + i + j]);
                return 1;
            }
            print_log("* thread for %s start", argv[optind + i + j]);
            if(argc == optind + i + j + 1) break;
        }
        for(j = 0; j < THREAD_COUNT; j++) {
            if(pthread_join(tid[j], NULL) != 0) {
                print_log("* error: thread for %s can not be joined", argv[optind + i + j]);
                return 1;
            }
            print_log("* thread for %s exit", argv[optind + i + j]);
            if(argc == optind + i + j + 1) break;
        }
    }

    print_log("* bye bye, enjoy your life");
    if(config.logger && logger != NULL)
        fclose(logger);

    curl_global_cleanup();
    return 0;
}
