#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct param_t param_t;

struct param_t {
    const char *file;
    pid_t       pid;
    bool        verbose;
    bool        b;
    bool        k;
    bool        m;
    bool        g;
};

static void     help(void);
static bool     do_param_parsing(int argc, char **argv, param_t *param);
static void     do_page_cache(param_t *param);
static void     do_mmap_info(param_t *param);
static void     print_file_incore(param_t *param);
static size_t   cache_of_file(const char *file);

size_t          rss_total = 0;
size_t          mmap_total = 0;
size_t          annon_total = 0;
size_t          page_cache_total = 0;

int
main(int argc, char **args) {
    param_t     param;
    memset(&param, 0, sizeof(param));

    if (!do_param_parsing(argc, args, &param)) {
        exit(1);
    }

    if (param.file != NULL) {
      print_file_incore(&param);
      return 0;
    }

    if (kill(param.pid, 0)) {
        fprintf(stderr, "%m\n");
        exit(1);
    }

    do_page_cache(&param);
    do_mmap_info(&param);

    annon_total = rss_total - mmap_total;

    if (param.k) {
        rss_total >>= 10;
        mmap_total >>= 10;
        annon_total >>= 10;
        page_cache_total >>= 10;
    }
    if (param.m) {
        rss_total >>= 20;
        mmap_total >>= 20;
        annon_total >>= 20;
        page_cache_total >>= 20;
    }
    if (param.g) {
        rss_total >>= 30;
        mmap_total >>= 30;
        annon_total >>= 30;
        page_cache_total >>= 30;
    }

    if (param.verbose) {
        fprintf(stdout,
                "%-20s"
                "%-20s"
                "%-20s"
                "%-20s\n",
                "Rss", "Mmap", "Annon", "PageCache"); 
    }
    fprintf(stdout,
            "%-20lu"
            "%-20lu"
            "%-20lu"
            "%-20lu\n",
            rss_total, mmap_total, annon_total, page_cache_total);

    return 0;
}

static void
do_page_cache(param_t *param)
{
    DIR            *dir = NULL;
    struct dirent  *fentry = NULL;
    char            path[64];

    snprintf(path, sizeof(path), "/proc/%d/fd", param->pid);
    if ((dir = opendir(path)) == NULL) {
        perror(path);
        return ;
    }

    if (param->verbose) {
        fprintf(stdout, "Page Cache:\n");
    }
    while ((fentry = readdir(dir)) != NULL) {
        if (!strcmp(fentry->d_name, ".")
                || !strcmp(fentry->d_name, "..")) {
            continue;
        }
        char symlink[256];
        char opened_file[256];
        snprintf(symlink, sizeof(symlink), "%s/%s", path, fentry->d_name);
        ssize_t n = 0;
        if ((n = readlink(symlink, opened_file, sizeof(opened_file))) == -1) {
            perror(symlink);
            continue;
        }
        if (opened_file[0] != '/') {
            continue;
        }
        opened_file[n] = '\0';
        if (!strcmp(opened_file, "/dev/null")) {
            continue;
        }
        size_t cached_size = cache_of_file(opened_file);
        page_cache_total += cached_size;
        if (param->verbose) {
            cached_size = !param->k ? cached_size : (cached_size>>10);
            cached_size = !param->m ? cached_size : (cached_size>>20);
            cached_size = !param->g ? cached_size : (cached_size>>30);
            fprintf(stdout, "%s: %lu\n", opened_file, cached_size);
        }
    }
}
static void
do_mmap_info(param_t *param)
{
    FILE           *file = NULL;
    char            path[64];
    char            mmap_file[256];
    char            line[256];

    snprintf(path, sizeof(path), "/proc/%d/smaps", param->pid);
    if ((file = fopen(path, "r")) == NULL) {
        perror("fopen");
        return ;
    }

    if (param->verbose) {
        fprintf(stdout, "mmapped rss:\n");
    }
    bool care = false;
    while (fgets(line, sizeof line, file)) {
        size_t cached_size = 0;
        if (sscanf(line, "%*[^/]%s", mmap_file) == 1) {
            care = true;
        } else if (care && !strncmp("Rss", line, 3)) {
            sscanf(line, "%*[^0-9]%lu%*[^0-9]", &cached_size);
            cached_size <<= 10;
            if (cached_size) {
                mmap_total += cached_size;
                if (param->verbose) {
                    cached_size = !param->k ? cached_size : (cached_size>>10);
                    cached_size = !param->m ? cached_size : (cached_size>>20);
                    cached_size = !param->g ? cached_size : (cached_size>>30);
                    fprintf(stdout, "%s: %lu\n", mmap_file, cached_size);
                }
            }
            care = false;
        }
        if (!strncmp("Rss", line, 3)) {
            sscanf(line, "%*[^0-9]%lu%*[^0-9]", &cached_size);
            cached_size <<= 10;
            rss_total += cached_size;
        }
    }
}

static void
print_file_incore(param_t *param)
{
    size_t size = cache_of_file(param->file);
    size = param->k ? (size>>10) : size;
    size = param->m ? (size>>20) : size;
    size = param->g ? (size>>30) : size;
    fprintf(stdout, "%lu\n", size);
}

static size_t
cache_of_file(const char *file)
{
    int             fd;
    struct stat     st;
    size_t          fsize;          //~ file size
    size_t          psize;          //~ page size
    size_t          tailsize;       //~ fsize % psize
    size_t          npages;         //~ page count of file
    size_t          pincore;        //~ page count in cache
    size_t          bincore;        //~ bytes in cache
    bool            aligned;        //~ whether fsize is power of psize
    char            *base;          //~ base of mmap
    unsigned char   *hints;         //~ hints used by `mincore'

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        perror(file);
        return 0;
    }
    if (fstat(fd, &st) == -1) {
        perror(file);
        return 0;
    }
    fsize = st.st_size;
    psize = getpagesize();
    npages = (fsize + psize - 1) / psize;
    aligned = !(fsize / psize);

    if (fsize == 0) {
        close(fd);
        return 0;
    }
    base = (char*)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (base == MAP_FAILED) {
        perror(file);
        return 0;
    }

    hints = (unsigned char *) malloc(npages);
    if (mincore(base, fsize, hints) == -1) {
        perror("mincore");
        return 0;
    }

    tailsize = fsize % psize;
    bool tailincore = (hints[npages-1] & 0x1);
    size_t i = 0;
    pincore = 0;
    for ( ; i < npages; ++i) {
        if (hints[i] & 0x1) {
            ++pincore;
        }
    }
    bincore = pincore * psize;
    bincore -= (tailincore && !aligned) ? psize - tailsize : 0;

    munmap(base, fsize);
    free(hints);
    return bincore;
}

bool
do_param_parsing(int argc, char **argv, param_t *param)
{
    int             opt;
    const char      *optstr = "hvbkmgp:f:";
    struct option   long_opts[] = {
        {"help", 0, NULL, 'h'},
        {"verbose", 0, NULL, 'v'},
        {"pid", 1, NULL, 'p'},
        {"file", 1, NULL, 'f'},
        {"byte", 0, NULL, 'b'},
        {"kilo", 0, NULL, 'k'},
        {"mega", 0, NULL, 'm'},
        {"giga", 0, NULL, 'g'},
        {NULL, 0, NULL, 0}
    };

    bool cache = false;
    bool process = false;

    param->pid = -1;
    param->verbose = false;
    param->b = true;
    param->k = false;
    param->m = false;
    param->g = false;
    while ((opt = getopt_long(argc, argv, optstr, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'p':
                param->pid = atoi(optarg);
                process = true;
                break;
            case 'f':
                param->file = optarg;
                cache = true;
                break;
            case 'v':
                param->verbose = true;
                break;
            case 'b':
                param->b = true;
                break;
            case 'k':
                param->k = true;
                break;
            case 'm':
                param->m = true;
                break;
            case 'g':
                param->g = true;
                break;
            case '?':
            case 'h':
            default :
                help();
                exit(1);
                break;
        }
    }
    if (param->b + param->k + param->m + param->g > 2) {
        fprintf(stderr, "-b, -k, -m, -g are mutual exlusive\n");
        return false;
    }
    if (cache + process == 2) {
      fprintf(stderr, "use <-p> or <-f>, not both\n");
      return false;
    }
    if (cache + process == 0) {
      if (param->pid < 0) {
        help();
        return false;
      }
      if (param->file < 0) {
        help();
        return false;
      }
    }
    return true;
}

void
help()
{
    fprintf(stderr,
            "mtair [options] <-p pid>, or <-f file>\n"
            "-h, --help\n\t"
            "print this message.\n"
            "-v, --verbose\n\t"
            "verbose mode\n"
            "-b, --byte\n\t"
            "set metric to byte\n"
            "-k, --kilo\n\t"
            "set metric to kilobyte\n"
            "-m, --mega\n\t"
            "set metric to megabyte\n"
            "-g, --giga\n\t"
            "set metric to gigabyte\n"
            "-f, --file\n\t"
            "file whose cache being queried\n"
            "-p, --pid\n\t"
            "process id\n");
}
