#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <curses.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>

#define DEBUG 0
#define K_TAB 9
#define K_ENTER 10
#define K_C 99

int wait = 1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct copyParams
{
    char *name;
    char *dest;
};

void sig_winch(int signo)
{
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, &size);
    resizeterm(size.ws_row, size.ws_col);
}

void *copy(void *ptr)
{
    //sleep(1);
    struct copyParams *params = ptr; 
    char wd[PATH_MAX];
    strcat(wd, "cp ./");
    strcat(wd, params->name);
    strcat(wd, " ");
    strcat(wd, params->dest);
    strcat(wd, "/");
    system(wd);
    //sleep(1);
    pthread_mutex_lock(&lock);
    wait = 0;
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main()
{
    WINDOW *wnd = NULL;
    WINDOW *left_wnd = NULL;
    WINDOW *right_wnd = NULL;
    int i = 0, j = 0, input_buf = 0, n_l = 0, n_r = 0, x_l = 0, x_r = 0, copy_size = 0,
        y = 0, cycle = 1, rows = 0, cols = 0, start_l = 0, start_r = 0, ip = 0;
    struct dirent **namelist_l;
    char wd_l[PATH_MAX];
    char wd_r[PATH_MAX];
    char wd[PATH_MAX];
    struct copyParams params;
    struct dirent **namelist_r;
    struct stat buff;
    pthread_t tid;
    
    n_l = scandir(".", &namelist_l, 0, alphasort);
    if(n_l < 0)
    {
        perror("Scandir error");
        exit(1);
    }
    n_r = scandir(".", &namelist_r, 0, alphasort);
    if(n_r < 0)
    {
        perror("Scandir error");
        exit(1);
    }
    if(!getcwd(wd_l, PATH_MAX))
    {
        perror("Getcwd error");
        exit(1);
    }
    if(!getcwd(wd_r, PATH_MAX))
    {
        perror("Getcwd error");
        exit(1);
    }
    initscr();
    if(has_colors() == FALSE)
    {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }
    start_color();
    signal(SIGWINCH, sig_winch);
    cbreak();
    curs_set(0);
    refresh();
    getmaxyx(stdscr, rows, cols);
    wnd = newwin(rows, cols, 0, 0);

    box(wnd,'|', '-');
    for(i = 1; i < rows - 1; i++)
    {
        move(i, cols/2);
        printw("|");
    }
    left_wnd = newwin(rows - 2, cols/2 - 2, 1, 1);

    right_wnd = newwin(rows - 2, cols/2 - 2, 1, cols/2 + 1);

    wrefresh(wnd);
    wrefresh(left_wnd);
    wrefresh(right_wnd);
    refresh();
    keypad(stdscr, TRUE);
    noecho();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_WHITE);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    pthread_mutex_init(&lock, NULL);

    while(cycle)
    {
        wclear(left_wnd);
        wclear(right_wnd);
        for(i = start_l, j = 0; i < n_l; i++, j++)
        {
            if(!y)
            {
                stat(namelist_l[i]->d_name, &buff);
                if(S_ISDIR(buff.st_mode))
                {
                    if(i == x_l)
                        wattron(left_wnd, COLOR_PAIR(3));
                    else
                        wattron(left_wnd, COLOR_PAIR(2));
                }else
                    if(i == x_l)
                        wattron(left_wnd, COLOR_PAIR(1));
            }else
                wattron(left_wnd, COLOR_PAIR(5));
            mvwprintw(left_wnd, j, 0, "%s", namelist_l[i]->d_name);
            wattron(left_wnd, COLOR_PAIR(4));
        }
        for(i = start_r, j = 0; i < n_r; i++, j++)
        {
            if(y)
            {
                stat(namelist_r[i]->d_name, &buff);
                if(S_ISDIR(buff.st_mode))
                {
                    if(i == x_r)
                        wattron(right_wnd, COLOR_PAIR(3));
                    else
                        wattron(right_wnd, COLOR_PAIR(2));
                }else
                    if(i == x_r)
                        wattron(right_wnd, COLOR_PAIR(1));
            }else
                wattron(right_wnd, COLOR_PAIR(5));
            mvwprintw(right_wnd, j, 0, "%s", namelist_r[i]->d_name);
            wattron(right_wnd, COLOR_PAIR(4));
        }
        wrefresh(left_wnd);
        wrefresh(right_wnd);
        refresh();

#if DEBUG
        move(rows - 2, 0);
        printw("%s  ", wd);
        move(rows - 4, 0);
        printw("%s  ", params.dest);
        refresh();
#endif

        cbreak();
        input_buf = getch();
        switch(input_buf)
        {
            case K_ENTER:
            case KEY_ENTER:
            {
                if(y)
                {
                    stat(namelist_r[x_r]->d_name, &buff);
                    if(S_ISDIR(buff.st_mode) && !access(namelist_r[x_r]->d_name, R_OK) &&
                        strcmp(namelist_r[x_r]->d_name, ".") != 0)
                    {
                        strcat(wd_r, "/");
                        strcat(wd_r, namelist_r[x_r]->d_name);
                        if(chdir(wd_r) < 0)
                        {
                            perror("Chdir error");
                            exit(1);
                        }
                        for(i = 0; i < n_r; i++)
                            free(namelist_r[i]);
                        free(namelist_r);
                        memset(&wd_r[0], 0, sizeof(wd_r));
                        getcwd(wd_r, PATH_MAX);
                        n_r = scandir(wd_r, &namelist_r, 0, alphasort);
                        if(n_r < 0)
                        {
                            perror("Scandir error");
                            exit(1);
                        }
                        x_r = 0;
                        start_r = 0;
                    }else{
                        if(S_ISREG(buff.st_mode) && !access(namelist_r[x_r]->d_name, X_OK))
                        {
                            memset(&wd[0], 0, sizeof(wd));
                            strcat(wd, "./");
                            strcat(wd, namelist_r[x_r]->d_name);
                            system(wd);
                        }
                    }
                }else{
                    stat(namelist_l[x_l]->d_name, &buff);
                    if(S_ISDIR(buff.st_mode) && !access(namelist_l[x_l]->d_name, R_OK) && 
                        strcmp(namelist_l[x_l]->d_name, ".") != 0)
                    {
                        strcat(wd_l, "/");
                        strcat(wd_l, namelist_l[x_l]->d_name);
                        if(chdir(wd_l) < 0)
                        {
                            perror("Chdir error");
                            exit(1);
                        }
                        for(i = 0; i < n_l; i++)
                            free(namelist_l[i]);
                        free(namelist_l);
                        memset(&wd_l[0], 0, sizeof(wd_l));
                        getcwd(wd_l, PATH_MAX);
                        n_l = scandir(wd_l, &namelist_l, 0, alphasort);
                        if(n_l < 0)
                        {
                            perror("Scandir error");
                            exit(1);
                        }
                        x_l = 0;
                        start_l = 0;
                    }else{
                        if(S_ISREG(buff.st_mode) && !access(namelist_l[x_l]->d_name, X_OK))
                        {
                            memset(&wd[0], 0, sizeof(wd));
                            strcat(wd, "./");
                            strcat(wd, namelist_l[x_l]->d_name);
                            system(wd);
                        }
                    }
                }
                break;
            }

            case K_C:
            {
                WINDOW *wnd_copy = NULL;
                char tmp[PATH_MAX];
                int size = 0, tmp_cycle = 1;
                if(y)
                {
                    stat(namelist_r[x_r]->d_name, &buff);
                    if(S_ISREG(buff.st_mode) && !access(namelist_r[x_r]->d_name, R_OK))
                    {
                        wnd_copy = newwin(rows - 2, cols/2 - 1, 1, 1);
                        params.name = namelist_r[x_r]->d_name;
                        params.dest = wd_l;
                        for(i = 0; i < n_r; i++)            //need to clear, because a new file is being added
                            free(namelist_r[i]);
                        free(namelist_r);
                        stat(params.name, &buff);
                        memset(&wd[0], 0, sizeof(wd));
                        getcwd(wd, PATH_MAX);
                        chdir(wd_r);
                    }
                }else{
                    stat(namelist_l[x_l]->d_name, &buff);
                    if(S_ISREG(buff.st_mode) && !access(namelist_l[x_l]->d_name, R_OK))
                    {
                        wnd_copy = newwin(rows - 2, cols/2 - 2, 1, cols/2 + 1);
                        params.name = namelist_l[x_l]->d_name;
                        params.dest = wd_r;
                        stat(params.name, &buff);
                        for(i = 0; i < n_l; i++)            //need to clear, because a new file is being added
                            free(namelist_l[i]);
                        free(namelist_l);
                        memset(&wd[0], 0, sizeof(wd));
                        getcwd(wd, PATH_MAX);
                        chdir(wd_l);
                    }
                }
                
                copy_size = buff.st_size;
                strcat(tmp, params.dest);
                strcat(tmp, "/");
                strcat(tmp, params.name);
                pthread_create(&tid, NULL, copy, &params);
                while(tmp_cycle)
                {
                    pthread_mutex_lock(&lock);
                    tmp_cycle = wait;
                    pthread_mutex_unlock(&lock);
                    if(stat(tmp, &buff) != 0)
                        size = 0;
                    else
                        size = buff.st_size;
                    wclear(wnd_copy);
                    mvwprintw(wnd_copy, 0, 0,"%s" "%d%s", "copied: ", size * 100 / copy_size, "%");
                    wrefresh(wnd_copy);
                }
                chdir(wd);
                wclear(wnd_copy);
                delwin(wnd_copy);
                wrefresh(left_wnd);
                wrefresh(right_wnd);
                refresh();
                pthread_join(tid, NULL);
                if(y)
                    n_r = scandir(wd_r, &namelist_r, 0, alphasort);
                else
                    n_l = scandir(wd_l, &namelist_l, 0, alphasort);
                pthread_mutex_lock(&lock);
                wait = 1;
                pthread_mutex_unlock(&lock);
                break;
            }

            case K_TAB:
            {
                if(y)
                {
                    y = 0;
                    if(chdir(wd_l) < 0)
                    {
                        perror("Chdir error");
                            exit(1);
                    }
                }else{
                    y = 1;
                    if(chdir(wd_r) < 0)
                    {
                        perror("Chdir error");
                            exit(1);
                    }
                }
                break;
            }

            case KEY_UP:
            {
                if(y)
                {
                    if(x_r > 0)
                    {
                        x_r -= 1;
                    }
                    if(start_r > x_r)
                        start_r -= 1;
                }else{
                    if(x_l > 0)
                    {
                        x_l -= 1;
                    }
                    if(start_l > x_l)
                        start_l -= 1;
                }
                break;
            }

            case KEY_DOWN:
            {
                if(y == 0)
                {
                    if(x_l < (n_l - 1 < rows - 3 ? n_l - 1 : rows - 3))
                    {
                        x_l += 1;
                    }else{
                       if(n_l - 1 > rows - 3 + start_l)
                       {
                           x_l += 1;
                           start_l += 1;
                       }
                    }
                }else{
                    if(x_r < (n_r - 1 < rows - 3 ? n_r - 1 : rows - 3))
                    {
                        x_r += 1;
                    }else{
                       if(n_r - 1 > rows - 3 + start_r)
                       {
                           x_r += 1;
                           start_r += 1;
                       }
                    }
                }
                break;
            }

            case KEY_BACKSPACE:
            {
                cycle = 0;
                break;
            }
        }
    }
    pthread_mutex_destroy(&lock);
    for(i = 0; i < n_r; i++)
        free(namelist_r[i]);
    free(namelist_r);

    for(i = 0; i < n_l; i++)
        free(namelist_l[i]);
    free(namelist_l);
    if(wnd != NULL)
        delwin(wnd);
    if(left_wnd != NULL)
        delwin(left_wnd);
    if(right_wnd != NULL)
        delwin(right_wnd);
    endwin();
    return 0;
}
