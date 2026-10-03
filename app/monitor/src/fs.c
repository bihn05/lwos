#include "fs.h"
#include "mem.h"
#include "string.h"

static BYTE buf[512];
static PBLKDEV disk;
static DWORD fat_lba, data_lba, root_cluster, cluster_count;
static BYTE sectors_per_cluster;
static int mounted;

static void print_field(PCSTR s, int width) {
    for (int i = 0; i < width; i++)
        lw_putc(s[i]);
}
static void print_field_pad(PCSTR s, int width, int l) {
    int i;
    for (i = 0; i < width; i++)
        lw_putc(s[i]);
    for (; i < l; i++)
        lw_putc(' ');
}
/* 磁盘字段按字节读取, 不依赖指针对齐或 C 结构布局。 */
static WORD read16(PCBYTE p) {
    return (WORD)p[0] | ((WORD)p[1] << 8);
}

static DWORD read32(PCBYTE p) {
    return (DWORD)read16(p) | ((DWORD)read16(p + 2) << 16);
}

static int read_sector(QWORD lba) {
    if (!disk || !disk->read || lba >= disk->sectors)
        return FS_ERR_IO;
    return disk->read(disk, lba, 1, buf) ? FS_ERR_IO : 0;
}

static int valid_cluster(DWORD c) {
    return c >= 2 && c - 2 < cluster_count;
}

static QWORD cluster_to_lba(DWORD c) {
    return (QWORD)data_lba + (QWORD)(c - 2) * sectors_per_cluster;
}

/* 1: 有下一簇, 0: 链结束, <0: 错误。 */
static int next_cluster(DWORD current, PDWORD next) {
    if (!valid_cluster(current))
        return FS_ERR_FORMAT;
    int err = read_sector((QWORD)fat_lba + current / 128);
    if (err)
        return err;
    DWORD c = read32(buf + (current % 128) * 4) & 0x0fffffffu;
    if (c >= 0x0ffffff8u)
        return 0;
    if (!valid_cluster(c) || c >= 0x0ffffff0u)
        return FS_ERR_FORMAT;
    *next = c;
    return 1;
}

int fat_read_fte(DWORD dir_cluster, int index, PSFT_T p) {
    if (!mounted)
        return FS_ERR_NOT_MOUNTED;
    if (index < 0 || !p)
        return FS_ERR_FORMAT;
    DWORD sector_index = (DWORD)index / 16;
    DWORD hops = sector_index / sectors_per_cluster;
    if (hops >= cluster_count)
        return FS_ERR_FORMAT; /* 限制损坏或循环的簇链。 */
    DWORD c = dir_cluster; // input directorie cluster
    for (DWORD i = 0; i < hops; i++) {
        int status = next_cluster(c, &c);
        if (status <= 0)
            return status;
    }
    int err = read_sector(cluster_to_lba(c) + sector_index % sectors_per_cluster);
    if (err)
        return err;
    PCBYTE e = buf + ((DWORD)index % 16) * 32;
    memzero(p, sizeof(*p));
    if (e[0] == 0)
        return FS_ENTRY_END;
    if (e[0] == 0xe5 || (e[11] & 0x0f) == 0x0f || (e[11] & 0x08))
        return FS_ENTRY_SKIP; /* 删除项、长文件名项、卷标。 */
    memcpy(p->name, (PVOID)e, 11);
    if ((BYTE)p->name[0] == 0x05)
        p->name[0] = (CHAR)0xe5;
    p->attr = e[11];
    p->cluster = (((DWORD)read16(e + 0x14) << 16) |
                  read16(e + 0x1a)) & 0x0fffffffu;
    p->length = read32(e + 0x1c);
    return FS_ENTRY_VALID;
}
static DWORD next_cluster_q(DWORD cluster) { // quiet version
    DWORD sector;
    DWORD offset;
    DWORD res;

    sector = fat_lba + cluster / 128;
    offset = (cluster % 128) * 4;
    if (!read_sector(sector))return 0xFFFFFFFF;
    res = read32(buf+offset);

    return res&0x0FFFFFFF;
}
// 找项目，不用看是目录项还是文件项目 它们都叫目录项得了
int dir_find(DWORD dir_cluster, PCSTR name, PSFT_T out, int is_dir) {
    DWORD cluster = dir_cluster;

    for (;;) {
        int i;

        for (i = 0; i < 16; i++) {
            int status = fat_read_fte(cluster, i, out);

            if (status == 0)return 0;   // end of list
            if (status == 2)continue;   // skipped
            if (status != 1)return status;  // errno

            if (strncmp(out->name, name, 11) != 0)
                continue;

            if (!!(out->attr & 0x10) != !!is_dir)
                continue;

            return 1;
        }

        cluster = next_cluster_q(cluster);

        if (cluster >= 0x0FFFFFF8)
            return 0;
    }
}
static int make_name83(PCSTR src, PSTR out) {
    int nl, el = 0;
    int i;

    if (!src || !out)
        return 0;

    nl = strcspn(src, ".");

    if (nl > 8)
        return -1; // filename too long

    if (src[nl] == '.') {
        el = strlen(src + nl + 1);

        if (el > 3)
            return -2; // extend name too long
    }

    for (i = 0; i < 11; i++)
        out[i] = ' ';

    for (i = 0; i < nl; i++)
        out[i] = (char)to_upper((BYTE)src[i]);

    for (i = 0; i < el; i++)
        out[8 + i] = (char)to_upper((BYTE)src[nl + 1 + i]);

    out[11] = '\0';

    return 1;
}
static DWORD cur_cluster;
static SFT_T cur_res;
/*
看第一个字是不是/来判断是否从根目录出发
然后解析第一个词语:
词语可能长的样子：xxxx.xxx 最终要转换成XXXX....XXX
*/
int resolve_path(PCSTR path, PDWORD cluster) { // 这里cluster并非传入，而是返回这个项目的起始cluster
    if (!mounted || !path || !cluster) {
        return FS_PATH_ERROR;
    }
    PCSTR p=path;
    CHAR nm[14];
    CHAR nm83[12];
    int status=0;
    skip_ws(&p);
    if (p[0]=='/') {
        lw_puts("ROOT\n\r");
        cur_cluster=root_cluster;
        p++;
    }
    L1:
    memzero(nm, 14);
    memzero(nm83, 12);
    int l=strcspn(p, "/");
    memcpy(nm, p, l);
    (void)make_name83(nm, nm83);
    if (p[l]=='/') {
        print_field_pad(nm, l, 12);
        lw_putc('|');
        print_field(nm83, 11);
        lw_puts("|TARGET=DIR\n\r");
        status=dir_find(cur_cluster,nm83,&cur_res,1);
        if (status==1) {
            cur_cluster=cur_res.cluster;
            lw_puts("FOUND AT ");
            lw_put_dword(cur_cluster);
            lw_puts("\n\r");
            p+=l+1;
            goto L1;
        }
        if (status==0) {
            lw_puts("NOT FOUND\n\r");
            return -1;
        }
    } else {
        print_field_pad(nm, l, 12);
        lw_putc('|');
        print_field(nm83, 11);
        lw_puts("|TARGET=FILE\n\r");
        status=dir_find(cur_cluster,nm83,&cur_res,0);
        if (status==1) {
            *cluster=cur_res.cluster;
            lw_puts("FOUND AT ");
            lw_put_dword(*cluster);
            lw_puts("\n\r");
            return 0;
        }
        if (status==0) {
            lw_puts("NOT FOUND\n\r");
            return -1;
        }
        return -2;
    }
    lw_put_dword(status);
    return -3;
}
void fs_scan(void) {
    if (!mounted) {
        lw_puts("FILESYSTEM NOT INITIALIZED; USE F FIRST\n\r");
        return;
    }
    SFT_T e;
    DWORD cur_cluster=root_cluster;
    /* 限制遍历量, 避免循环 FAT 链使扫描永远不结束。 */
    QWORD max_entries = (QWORD)cluster_count * sectors_per_cluster * 16;
    if (max_entries > 0x7fffffffu)
        max_entries = 0x7fffffffu;
    for (int idx = 0; (QWORD)idx < max_entries; idx++) {
        int status = fat_read_fte(cur_cluster, idx, &e);
        if (status == FS_ENTRY_END)
            return;
        if (status < 0) {
            lw_puts("DIRECTORY READ FAILED\n\r");
            return;
        }
        if (status == FS_ENTRY_SKIP)
            continue;
        print_field(e.name, 8);
        lw_putc(' ');
        if (e.attr & 0x10)
            lw_puts("<DIR>");
        else
            print_field(e.name + 8, 3);
        lw_putc(' ');
        lw_put_dword(e.length);
        lw_puts(" BYTES AT ");
        lw_put_dword(e.cluster);
        lw_puts("\n\r");
    }
    lw_puts("DIRECTORY CHAIN LIMIT REACHED\n\r");
}

int fs_init(void) {
    mounted = 0;
    lw_puts("FILESYSTEM DETECT\n\r");
    lw_disk_probe();
    disk = lw_blk_get(0);
    if (!disk || !disk->read || disk->sector_size != sizeof(buf))
        return FS_ERR_IO;
    int err = read_sector(0);
    if (err)
        return err;
    if (read16(buf + 0x1fe) != 0xaa55)
        return FS_ERR_FORMAT;

    /* 选 FAT32 分区, 无需依赖 MBR active 标志。 */
    DWORD part_lba = 0, part_sectors = 0;
    for (int i = 0; i < 4; i++) {
        PCBYTE p = buf + 0x1be + i * 16;
        if (p[4] == 0x0b || p[4] == 0x0c) {
            part_lba = read32(p + 8);
            part_sectors = read32(p + 12);
            break;
        }
    }
    if (!part_lba || !part_sectors || part_lba >= disk->sectors ||
        part_sectors > disk->sectors - part_lba)
        return FS_ERR_FORMAT;
    err = read_sector(part_lba);
    if (err)
        return err;
    if (read16(buf + 0x1fe) != 0xaa55 || read16(buf + 0x0b) != 512)
        return FS_ERR_FORMAT;

    BYTE spc = buf[0x0d], fats = buf[0x10];
    WORD reserved = read16(buf + 0x0e);
    DWORD fat_size = read32(buf + 0x24);
    DWORD total = read32(buf + 0x20);
    DWORD root = read32(buf + 0x2c) & 0x0fffffffu;
    WORD flags = read16(buf + 0x28);
    BYTE active_fat = (flags & 0x80) ? (flags & 0x0f) : 0;
    QWORD overhead = (QWORD)reserved + (QWORD)fats * fat_size;
    if (!spc || spc > 128 || (spc & (spc - 1)) || !fats || !reserved ||
        !fat_size || !total || total > part_sectors || overhead >= total ||
        active_fat >= fats || read16(buf + 0x11) || read16(buf + 0x16) ||
        read16(buf + 0x2a) || (QWORD)part_lba + total > 0x100000000ull)
        return FS_ERR_FORMAT;
    DWORD clusters = (total - (DWORD)overhead) / spc;
    if (clusters < 65525 || clusters > 0x0fffffeeu || root < 2 ||
        root - 2 >= clusters || (QWORD)fat_size * 128 < (QWORD)clusters + 2)
        return FS_ERR_FORMAT;
    sectors_per_cluster = spc;
    cluster_count = clusters;
    root_cluster = root;
    fat_lba = part_lba + reserved + active_fat * fat_size;
    data_lba = part_lba + (DWORD)overhead;
    mounted = 1;
    lw_puts("FAT32 READY\n\r");
    cur_cluster=root_cluster;
    return 0;
}
