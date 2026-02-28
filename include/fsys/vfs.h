// kernel/fsys/vfs.h

#ifndef _VFS_H_
#define _VFS_H_

#include <stdint.h>
#include <printk.h>

#include <driver/block.h>

#define VFS_FLAG_FILE 0x01
#define VFS_FLAG_DIR  0x02

struct vfs_node;

// VFS 节点操作集：由具体的文件系统（如 LWFS）负责实现这些函数指针
typedef struct {
    int (*read)(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    int (*write)(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    struct vfs_node* (*finddir)(struct vfs_node* dir, const char* name);
    int (*create)(struct vfs_node* dir, const char* name, uint32_t flags);
} vfs_ops_t;

// VFS 节点结构：内核眼中看到的文件/目录实体
typedef struct vfs_node {
    char name[32];          // 节点名称
    uint32_t size;          // 节点大小（字节）
    uint32_t flags;         // 属性：文件 or 目录
    uint32_t internal;      // 内部数据，VFS 不关心具体内容，由具体文件系统使用

    void* fs_instance;    // 指向所属文件系统实例的指针，方便访问底层数据
    void* fs_private_data;  // 私有数据指针。VFS不关心里面是什么，LWFS可以用它来存"起始簇号(start_cluster)"
    
    vfs_ops_t* ops;         // 指向该节点支持的操作
} vfs_node_t;

#endif