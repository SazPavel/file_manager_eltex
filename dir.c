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
    WINDOW *win[2] = {NULL, NULL};
    int i = 0, j = 0, k = 0, input_buf = 0, copy_size = 0,
        y = 0, cycle = 1, rows = 0, cols = 0, ip = 0;
    int n[2] = {0, 0}, x[2] = {0, 0}, start[2] = {0, 0};
    struct dirent **namelist[2];
    char wd_win[2][PATH_MAX];
    char wd[PATH_MAX];
    struct copyParams params;
    struct stat buff;
    pthread_t tid;
    
    for(i = 0; i < 2; i++)
    {
        n[i] = scandir(".", &namelist[i], 0, alphasort);
        if(n[i] < 0)
        {
            perror("Scandir error");
            exit(1);
        }
        if(!getcwd(wd_win[i], PATH_MAX))
        {
            perror("Getcwd error");
            exit(1);
        }
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
    win[0] = newwin(rows - 2, cols/2 - 2, 1, 1);

    win[1] = newwin(rows - 2, cols/2 - 2, 1, cols/2 + 1);

    wrefresh(wnd);
    wrefresh(win[0]);
    wrefresh(win[1]);
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
        for(k = 0; k < 2; k++)
        {
            wclear(win[k]);
            for(i = start[k], j = 0; i < n[k]; i++, j++)
            {
                if(y == k)
                {
                    stat(namelist[y][i]->d_name, &buff);
                    if(S_ISDIR(buff.st_mode))
                    {
                        if(i == x[y])
                            wattron(win[k], COLOR_PAIR(3));
                        else
                            wattron(win[k], COLOR_PAIR(2));
                    }else
                        if(i == x[y])
                            wattron(win[k], COLOR_PAIR(1));
                }else
                    wattron(win[k], COLOR_PAIR(5));
                mvwprintw(win[k], j, 0, "%s", namelist[k][i]->d_name);
                wattron(win[k], COLOR_PAIR(4));
            }
            wrefresh(win[k]);
        }
        refresh();

        
#if DEBUG

        move(rows - 2, 0);
        printw("%s  ", wd_win[y]);
        move(rows - 4, 0);
        printw("%s  ", namelist[y][x[y]]->d_name);
        refresh();
#endif

        cbreak();
        input_buf = getch();
        switch(input_buf)
        {
            case K_ENTER:
            case KEY_ENTER:
            {
                stat(namelist[y][x[y]]->d_name, &buff);
                if(S_ISDIR(buff.st_mode) && !access(namelist[y][x[y]]->d_name, R_OK) &&
                    strcmp(namelist[y][x[y]]->d_name, ".") != 0)
                {
                    strcat(wd_win[y], "/");
                    strcat(wd_win[y], namelist[y][x[y]]->d_name);
                    if(chdir(wd_win[y]) < 0)
                    {
                        perror("Chdir error");
                        exit(1);
                    }
                    for(i = 0; i < n[y]; i++)
                        free(namelist[y][i]);
                    free(namelist[y]);
                    memset(&wd_win[y][0], 0, sizeof(wd_win[y]));
                    getcwd(wd_win[y], PATH_MAX);
                    n[y] = scandir(wd_win[y], &namelist[y], 0, alphasort);
                    if(n[y] < 0)
                    {
                        perror("Scandir error");
                        exit(1);
                    }
                    x[y] = 0;
                    start[y] = 0;
                }else{
                    if(S_ISREG(buff.st_mode) && !access(namelist[y][x[y]]->d_name, X_OK))
                    {
                        memset(&wd[0], 0, sizeof(wd));
                        strcat(wd, "./");
                        strcat(wd, namelist[y][x[y]]->d_name);
                        system(wd);
                    }
                }
                break;
            }

            case K_C:
            {
                WINDOW *wnd_copy = NULL;
                char tmp[PATH_MAX];
                int size = 0, tmp_cycle = 1;
                stat(namelist[y][x[y]]->d_name, &buff);
                if(S_ISREG(buff.st_mode) && !access(namelist[y][x[y]]->d_name, R_OK))
                {
                    if(y)
                        wnd_copy = newwin(rows - 2, cols/2 - 1, 1, 1);
                    else
                        wnd_copy = newwin(rows - 2, cols/2 - 2, 1, cols/2 + 1);
                    params.name = namelist[y][x[y]]->d_name;
                    params.dest = wd_win[!y];
                    for(i = 0; i < n[y]; i++)            //need to clear, because a new file is being added
                        free(namelist[y][i]);
                    free(namelist[y]);
                    stat(params.name, &buff);
                    memset(&wd[0], 0, sizeof(wd));
                    getcwd(wd, PATH_MAX);
                    chdir(wd_win[1]);
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
                wrefresh(win[0]);
                wrefresh(win[1]);
                refresh();
                pthread_join(tid, NULL);
                if(y)
                    n[1] = scandir(wd_win[1], &namelist[1], 0, alphasort);
                else
                    n[0] = scandir(wd_win[0], &namelist[0], 0, alphasort);
                pthread_mutex_lock(&lock);
                wait = 1;
                pthread_mutex_unlock(&lock);
                break;
            }

            case K_TAB:
            {
                y ^= 1;
                if(chdir(wd_win[y]) < 0)
                {
                    perror("Chdir error");
                        exit(1);
                }
                break;
            }

            case KEY_UP:
            {
                if(x[y] > 0)
                {
                    x[y] -= 1;
                }
                if(start[y] > x[y])
                    start[y] -= 1;
                break;
            }

            case KEY_DOWN:
            {
                if(x[y] < (n[y] - 1 < rows - 3 ? n[y] - 1 : rows - 3))
                {
                    x[y] += 1;
                }else{
                   if(n[y] - 1 > rows - 3 + start[y])
                   {
                       x[y] += 1;
                       start[y] += 1;
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
    for(i = 0; i < 2; i++)
    {
        for(j = 0; j < n[i]; j++)
            free(namelist[i][j]);
        free(namelist[i]);
    }
    
    if(wnd != NULL)
        delwin(wnd);
    if(win[0] != NULL)
        delwin(win[0]);
    if(win[1] != NULL)
        delwin(win[1]);
    endwin();
    return 0;
}
