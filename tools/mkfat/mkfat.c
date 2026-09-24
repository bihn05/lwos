
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define BPS                 512
#define RSV_S               32
#define N_FATS              2
#define SPC                 1
#define FSINFO              1
#define BACKUP_BOOT_SECTOR  6
#define MIN_FAT32_CLUSTERS  65525

#define ATTR_RO             0x01
#define ATTR_ARCH           0x20
#define ATTR_VOL_ID         0x08

// write LITTLE ENDIAN

static void put_u16(unsigned char *buf, size_t off, uint16_t v) {
    buf[off+0] = (unsigned char)(v & 0xff);
    buf[off+1] = (unsigned char)((v >> 8) & 0xff);
}
static void put_u32(unsigned char *buf, size_t off, uint32_t v) {
    buf[off+0] = (unsigned char)(v & 0xff);
    buf[off+1] = (unsigned char)((v >> 8) & 0xff);
    buf[off+2] = (unsigned char)((v >> 16) & 0xff);
    buf[off+3] = (unsigned char)((v >> 24) & 0xff);
}

// deal error or sth

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}
static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p && n)die("out of mem");
    return p;
}
static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q && n)die("invalid mem");
    return q;
}

// dos date/time

static uint16_t dos_date(time_t t) {
    struct tm lt;
    localtime_r(&t, &lt);
    int year = lt.tm_year + 1900 - 1980;
    if (year < 0)year = 0;
    if (year > 127)year = 127;
    return (uint16_t)((year << 9) | ((lt.tm_mon + 1) << 5) | lt.tm_mday);
}
static uint16_t dos_time(time_t t) {
    struct tm lt;
    localtime_r(&t, &lt);
    return (uint16_t)((lt.tm_hour << 11) | (lt.tm_min << 5) | (lt.tm_sec / 2));
}

// 8.3 filename

static int char_filt(char c) {
    switch (c) {
        case ' ':case '"':case '*':
        case '+':case ',':case ':':
        case ';':case '<':case '=':
        case '>':case '?':case '[':
        case '\\':case ']':case '|':
        return 0;
        default:
        return 1;
    }
}
static int short_name(const char *filename, char n8[8], char e3[3]) {
    size_t len = strlen(filename);
    char upper[1024];
    if (len >= sizeof(upper))len = sizeof(upper) - 1;
    for (size_t i = 0; i < len; i++)
        upper[i] = (char)toupper((unsigned char)filename[i]);
    upper[len] = 0;

    char *dot = strrchr(upper, '.');
    const char *stem_src, *ext_src;
    char empty[1] = "";
    if (dot) {
        *dot=0;
        stem_src = upper;
        ext_src = dot + 1;
    } else {
        stem_src = upper;
        ext_src = empty;
    }

    char stem_kept[1024], ext_kept[1024];
    size_t sk = 0, ek = 0;
    for (const char *p = stem_src; *p && sk < sizeof(stem_kept) - 1; p++)
        if (char_filt(*p)) stem_kept[sk++] = *p;
    stem_kept[sk] = '\0';
    for (const char *p = ext_src; *p && ek < sizeof(ext_kept) - 1; p++)
        if (char_filt(*p)) ext_kept[ek++] = *p;
    ext_kept[ek] = '\0';

    if (sk == 0) return -1;

    memset(n8, ' ', 8);
    memset(e3, ' ', 3);
    memcpy(n8, stem_kept, sk > 8 ? 8 : sk);
    memcpy(e3, ext_kept, ek > 3 ? 3 : ek);
    return 0;
}

// layout

static void compute_layout(uint32_t part_sectors,
                           uint32_t *out_fat_size,
                           uint32_t *out_count_of_clusters) {
    uint32_t fat_size = 1;
    for (;;) {
        int64_t data_sectors = (int64_t)part_sectors - RSV_S - (int64_t)N_FATS * fat_size;
        if (data_sectors <= 0)
            die("too small partition ");
        
        uint32_t count_of_clusters = (uint32_t)(data_sectors / SPC);
        uint32_t entries_per_fat_sector = BPS / 4;
        uint32_t needed = ((count_of_clusters + 2) + entries_per_fat_sector - 1)
                            / entries_per_fat_sector;

        if (needed <= fat_size) {
            *out_fat_size = fat_size;
            *out_count_of_clusters = count_of_clusters;
            return;
        }

        uint32_t next = fat_size + 1;
        if (needed > next)next = needed;
        fat_size = next;
    }
}

static uint32_t cluster_to_sector(uint32_t cluster, uint32_t data_start_sector) {
    return data_start_sector + (cluster - 2) * SPC;
}

static void build_boot_sector(unsigned char b[BPS],
                              uint32_t part_sectors,
                              uint32_t fat_size,
                              uint32_t part_lba,
                              uint32_t volume_id) {
    memset(b, 0, BPS);
    b[0]=0xeb,b[1]=0x58,b[2]=0x90;
    memcpy(b+3,"LWCNC1.0",8);
    put_u16(b, 0x0b, BPS);
    b[0x0d] = SPC;
    put_u16(b, 0x0e, RSV_S);
    b[0x10] = N_FATS;
    put_u16(b, 0x11, 0);
    put_u16(b, 0x13, 0);
    b[0x15] = 0xf8;
    put_u16(b, 0x16, 0);
    put_u16(b, 0x18, 63);
    put_u16(b, 0x1a, 16);
    put_u32(b, 0x1c, part_lba);
    put_u32(b, 0x20, part_sectors);
    put_u32(b, 0x24, fat_size);
    put_u16(b, 0x28, 0);
    put_u16(b, 0x2a, 0);
    put_u32(b, 0x2c, 2);
    put_u16(b, 0x30, FSINFO);
    put_u16(b, 0x32, BACKUP_BOOT_SECTOR);
    /* 0x34-40 keep resv, 0*/
    b[0x40] = 0x80;
    b[0x41] = 0x0;
    b[0x42] = 0x29;
    put_u32(b, 0x43, volume_id);
    memcpy(b+0x47, "LWCNC FAT32", 11);
    memcpy(b+0x52, "FAT32   ", 8);
    b[0x1fe] = 0x55;
    b[0x1ff] = 0xaa;
}

static void build_fsinfo(unsigned char b[BPS], uint32_t free_clusters) {
    memset(b, 0, BPS);
    put_u32(b, 0x000, 0x41615252);
    put_u32(b, 0x1e4, 0x61417272);
    put_u32(b, 0x1e8, free_clusters);
    put_u32(b, 0x1ec, 0xffffffffu);
    put_u32(b, 0x1fc, 0xaa550000u);
}

/*
        FAT32's FTE struct
     +0h:    8b  filename
     +8h:    3b  extended name
     +bh:  BYTE  attribute
     +ch:  BYTE  NTres
     +dh:  BYTE  create millisec
     +eh:  WORD  create time
    +10h:  WORD  create date
    +12h:  WORD  last access date
    +14h:  WORD  start cluster (masked 0xffff0000)
    +16h:  WORD  last modify time
    +18h:  WORD  last modify date
    +1ah:  WORD  start cluster (masked 0x0000ffff)
    +1ch: DWORD  file length
*/

static void dir_entry(unsigned char e[32],
                    const char n8[8], 
                    const char e3[3],
                    unsigned char attr,
                    uint32_t cluster,
                    uint32_t size,
                    time_t mtime) {
    memset(e, 0, 32);
    memcpy(e+0, n8, 8);
    memcpy(e+8, e3, 3);
    e[0xb] = attr;
    e[0xd] = 0;
    put_u16(e, 0xe, dos_time(mtime));
    put_u16(e, 0x10, dos_date(mtime));
    put_u16(e, 0x12, dos_date(mtime));
    put_u16(e, 0x14, (uint16_t)((cluster >> 16) & 0xffff));
    put_u16(e, 0x16, dos_time(mtime));
    put_u16(e, 0x18, dos_date(mtime));
    put_u16(e, 0x1a, (uint16_t)(cluster & 0xffff));
    put_u32(e, 0x1c, size);
}

typedef struct {
    char n8[8];
    char e3[3];
    unsigned char *data;
    long size;
    time_t mtime;
    int readonly;
} FileEntry;

typedef struct {
    char *name;
    char *path;
    int readonly;
} SourceSpec;

static int cmp_str(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

int main(int argn, char **argv) {
    const char *image = NULL;
    const char *fsroot = NULL;
    long total_sectors = -1;
    long part_lba_arg = -1;
    SourceSpec *extras = NULL;
    size_t n_extras = 0, cap_extras = 0;

    for (int i = 1; i < argn; i++) {
        if (!strcmp(argv[i], "--image") && i + 1 < argn) {
            image = argv[++i];
        } else if (!strcmp(argv[i], "--total-sectors") && i + 1 < argn) {
            total_sectors = strtol(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--part-lba") && i + 1 < argn) {
            part_lba_arg = strtol(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--fsroot") && i + 1 < argn) {
            fsroot = argv[++i];
        } else if (!strcmp(argv[i], "--extra-file") && i + 1 < argn) {
            char *spec = argv[++i];
            char *eq = strchr(spec, '=');
            if (!eq) die("--extra-file wants NAME=PATH, got '%s'", spec);
            if (n_extras == cap_extras) {
                cap_extras = cap_extras ? cap_extras * 2 : 8;
                extras = xrealloc(extras, cap_extras * sizeof(*extras));
            }
            size_t namelen = (size_t)(eq - spec);
            extras[n_extras].name = xmalloc(namelen + 1);
            memcpy(extras[n_extras].name, spec, namelen);
            extras[n_extras].name[namelen] = '\0';
            extras[n_extras].path = eq + 1;
            extras[n_extras].readonly = 0;
            n_extras++;
        } else {
            die("unrecognized argument '%s'", argv[i]);
        }
    }
    if (!image) die("--image is required");
    if (!fsroot) die("--fsroot is required");
    if (total_sectors < 0) die("--total-sectors is required");
    if (part_lba_arg < 0) die("--part-lba is required");

    uint32_t part_lba = (uint32_t)part_lba_arg;
    if (total_sectors < part_lba_arg) die("--total-sectors must be >= --part-lba");
    uint32_t part_sectors = (uint32_t)(total_sectors - part_lba_arg);

    uint32_t fat_size, count_of_clusters;
    compute_layout(part_sectors, &fat_size, &count_of_clusters);
    if (count_of_clusters < MIN_FAT32_CLUSTERS)
        die("only %u clusters at this size, need >= %d for a real FAT32 volume -- "
            "raise --total-sectors", count_of_clusters, MIN_FAT32_CLUSTERS);

    uint32_t data_start_sector = RSV_S + N_FATS * fat_size; /* partition-relative */

    /* ---- gather fsroot/ files (regular files only, sorted by name) ---- */
    DIR *d = opendir(fsroot);
    if (!d) die("cannot open --fsroot '%s': %s", fsroot, strerror(errno));
    char **names = NULL;
    size_t n_names = 0, cap_names = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", fsroot, de->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (n_names == cap_names) {
            cap_names = cap_names ? cap_names * 2 : 16;
            names = xrealloc(names, cap_names * sizeof(*names));
        }
        names[n_names++] = strdup(de->d_name);
    }
    closedir(d);
    qsort(names, n_names, sizeof(*names), cmp_str);

    size_t n_sources = n_names + n_extras;
    SourceSpec *sources = xmalloc(n_sources * sizeof(*sources));
    for (size_t i = 0; i < n_names; i++) {
        sources[i].name = names[i];
        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", fsroot, names[i]);
        sources[i].path = strdup(full);
        sources[i].readonly = 1;
    }
    for (size_t i = 0; i < n_extras; i++) {
        sources[n_names + i] = extras[i];
    }

    /* ---- build file entries: 8.3 name, collision check, read data ---- */
    FileEntry *files = xmalloc(n_sources * sizeof(*files));
    char (*seen_n8)[8] = xmalloc(n_sources * sizeof(*seen_n8));
    char (*seen_e3)[3] = xmalloc(n_sources * sizeof(*seen_e3));
    size_t n_files = 0;

    for (size_t i = 0; i < n_sources; i++) {
        char n8[8], e3[3];
        if (short_name(sources[i].name, n8, e3) != 0)
            die("'%s' has no usable 8.3 name", sources[i].name);
        for (size_t j = 0; j < n_files; j++) {
            if (!memcmp(seen_n8[j], n8, 8) && !memcmp(seen_e3[j], e3, 3))
                die("'%s' collides with another file's 8.3 name -- rename one",
                    sources[i].name);
        }
        memcpy(seen_n8[n_files], n8, 8);
        memcpy(seen_e3[n_files], e3, 3);

        FILE *fh = fopen(sources[i].path, "rb");
        if (!fh) die("cannot open '%s': %s", sources[i].path, strerror(errno));
        fseek(fh, 0, SEEK_END);
        long sz = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        unsigned char *data = xmalloc((size_t)sz);
        if (sz > 0 && fread(data, 1, (size_t)sz, fh) != (size_t)sz)
            die("short read on '%s'", sources[i].path);
        fclose(fh);

        struct stat st;
        if (stat(sources[i].path, &st) != 0)
            die("cannot stat '%s': %s", sources[i].path, strerror(errno));

        memcpy(files[n_files].n8, n8, 8);
        memcpy(files[n_files].e3, e3, 3);
        files[n_files].data = data;
        files[n_files].size = sz;
        files[n_files].mtime = st.st_mtime;
        files[n_files].readonly = sources[i].readonly;
        n_files++;
    }

    /* ---- allocate clusters: root dir first, then each file in turn ---- */
    long cluster_bytes = (long)BPS * SPC;
    long entries_per_cluster = cluster_bytes / 32;
    long root_entries_needed = 1 + (long)n_files; /* + volume label entry */
    uint32_t root_clusters = (uint32_t)((root_entries_needed + entries_per_cluster - 1)
                                         / entries_per_cluster);
    if (root_clusters < 1) root_clusters = 1;

    uint32_t *fat = xmalloc((size_t)(count_of_clusters + 2) * sizeof(uint32_t));
    memset(fat, 0, (size_t)(count_of_clusters + 2) * sizeof(uint32_t));
    fat[0] = 0x0FFFFFF8u;
    fat[1] = 0x0FFFFFFFu;

    uint32_t next_cluster = 2 + root_clusters; /* root itself is pinned at cluster 2 */

    /* alloc_chain: if start_pinned, use `pinned_start` (root case); else use
       and advance next_cluster. Returns the first cluster of the chain. */
    uint32_t root_first_cluster;
    {
        uint32_t c = 2;
        for (uint32_t i = 0; i < root_clusters; i++) {
            uint32_t nxt = (i < root_clusters - 1) ? c + 1 : 0x0FFFFFFFu;
            fat[c] = nxt;
            c++;
        }
        root_first_cluster = 2;
    }
    if (root_first_cluster != 2) die("internal error: root not at cluster 2");

    uint32_t *file_first = xmalloc(n_files * sizeof(uint32_t));
    uint32_t *file_nclusters = xmalloc(n_files * sizeof(uint32_t));

    for (size_t i = 0; i < n_files; i++) {
        long n_clusters = (files[i].size + cluster_bytes - 1) / cluster_bytes;
        if (n_clusters < 1) n_clusters = 1;
        uint32_t first = next_cluster;
        uint32_t c = first;
        for (long k = 0; k < n_clusters; k++) {
            uint32_t nxt = (k < n_clusters - 1) ? c + 1 : 0x0FFFFFFFu;
            fat[c] = nxt;
            c++;
        }
        next_cluster += (uint32_t)n_clusters;
        file_first[i] = first;
        file_nclusters[i] = (uint32_t)n_clusters;
    }

    uint32_t used_clusters = next_cluster - 2;
    if (used_clusters > count_of_clusters)
        die("fsroot/ needs %u clusters but the volume only has %u -- "
            "raise --total-sectors or trim fsroot/", used_clusters, count_of_clusters);

    /* ---- build root directory entries ---- */
    unsigned char *root_bytes = xmalloc((n_files + 1) * 32);
    dir_entry(root_bytes, "LWCNC FA", "T32", ATTR_VOL_ID, 0, 0, time(NULL));
    for (size_t i = 0; i < n_files; i++) {
        unsigned char attr = ATTR_ARCH | (files[i].readonly ? ATTR_RO : 0);
        dir_entry(root_bytes + (i + 1) * 32, files[i].n8, files[i].e3, attr,
                   file_first[i], (uint32_t)files[i].size, files[i].mtime);
    }

    /* ---- assemble the partition image in memory ---- */
    size_t part_bytes_len = (size_t)part_sectors * BPS;
    unsigned char *part = xmalloc(part_bytes_len);
    memset(part, 0, part_bytes_len);

    uint32_t volume_id = (uint32_t)time(NULL);
    unsigned char boot[BPS], fsinfo[BPS];
    build_boot_sector(boot, part_sectors, fat_size, part_lba, volume_id);
    memcpy(part + 0 * BPS, boot, BPS);
    build_fsinfo(fsinfo, count_of_clusters - used_clusters);
    memcpy(part + FSINFO * BPS, fsinfo, BPS);
    memcpy(part + BACKUP_BOOT_SECTOR * BPS, boot, BPS);

    unsigned char *fat_bytes = xmalloc((size_t)fat_size * BPS);
    memset(fat_bytes, 0, (size_t)fat_size * BPS);
    for (uint32_t i = 0; i < count_of_clusters + 2; i++)
        put_u32(fat_bytes, (size_t)i * 4, fat[i]);
    for (int copy = 0; copy < N_FATS; copy++) {
        size_t off = (size_t)(RSV_S + copy * fat_size) * BPS;
        memcpy(part + off, fat_bytes, (size_t)fat_size * BPS);
    }

    size_t root_off = (size_t)cluster_to_sector(2, data_start_sector) * BPS;
    memcpy(part + root_off, root_bytes, (n_files + 1) * 32);

    for (size_t i = 0; i < n_files; i++) {
        size_t off = (size_t)cluster_to_sector(file_first[i], data_start_sector) * BPS;
        memcpy(part + off, files[i].data, (size_t)files[i].size);
    }

    /* ---- write into the disk image at part_lba ---- */
    FILE *img = fopen(image, "r+b");
    if (!img) die("cannot open --image '%s': %s", image, strerror(errno));
    if (fseek(img, (long)part_lba * BPS, SEEK_SET) != 0)
        die("seek failed on '%s': %s", image, strerror(errno));
    if (fwrite(part, 1, part_bytes_len, img) != part_bytes_len)
        die("short write on '%s'", image);
    fclose(img);

    printf("mkfat: %zu file(s), %u clusters (%u MiB data area), FAT size %u sectors x%d\n",
           n_files, count_of_clusters,
           (unsigned)((uint64_t)count_of_clusters * cluster_bytes / (1024 * 1024)),
           fat_size, N_FATS);
    return 0;
}