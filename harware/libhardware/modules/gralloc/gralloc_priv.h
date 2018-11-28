/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/native_handle.h>

#include <linux/fb.h>

/*****************************************************************************/

struct private_module_t;
struct private_handle_t;

struct private_module_t {
    gralloc_module_t base; //继承gralloc_module_t

    private_handle_t* framebuffer;
    uint32_t flags;  //指向图形缓冲区的句柄
    /**
     * 系统帧缓冲区包含有多少个图形缓冲区
     * 一个帧缓冲区包含有多少个图形缓冲区是与它的可视分辨率以及虚拟分辨率的大小有关的。
     * 例如，如果一个帧缓冲区的可视分辨率为800 x 600，而虚拟分辨率为1600 x 600，
     * 那么这个帧缓冲区就可以包含有两个图形缓冲区
     */
    uint32_t numBuffers;
    /**
     * 帧缓冲的使用情况
     * 00分别表示两个图缓冲区都是空闲的，01表示第2个图形缓冲区已经分配出去
    */
    uint32_t bufferMask;
    pthread_mutex_t lock; //保护结构体private_module_t的并行访问
    buffer_handle_t currentBuffer; //当前正在被渲染的图形缓冲区
    int pmem_master; //pmem设备节点的描述符
    void* pmem_master_base; //pmem的起始虚拟地址，平台中内存有ashmen、pmem等多种内存类型

    struct fb_var_screeninfo info; //屏幕设备的可变参数
    struct fb_fix_screeninfo finfo; //屏幕设备的固定参数
    float xdpi;
    float ydpi;
    float fps; //屏幕设备的刷新率
};

/*****************************************************************************/
/**
    应用开发程序自己操作的缓冲区数据结构，可能是在帧缓冲区中分配的，也可能是在内存中分配的
*/

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
struct private_handle_t {
    struct native_handle nativeHandle;
#endif

enum {
        PRIV_FLAGS_FRAMEBUFFER = 0x00000001
    };

    // file-descriptors
    int     fd;
    // ints
    int     magic; //标识一个private_handle_t结构体
    int     flags; //PRIV_FLAGS_FRAMEBUFFER(帧缓冲区分配)或者0
    int     size; //图形缓冲区的大小
    int     offset; //图形缓冲区的偏移地址

    // FIXME: the attributes below should be out-of-line
    uint64_t base __attribute__((aligned(8))); //图形缓冲区的实际地址
    int     pid; //图形缓冲区的创建者的PID

#ifdef __cplusplus
    static inline int sNumInts() {
        return (((sizeof(private_handle_t) - sizeof(native_handle_t))/sizeof(int)) - sNumFds);
    }
    static const int sNumFds = 1;
    static const int sMagic = 0x3141592;

    private_handle_t(int fd, int size, int flags) :
        fd(fd), magic(sMagic), flags(flags), size(size), offset(0),
        base(0), pid(getpid())
    {
        version = sizeof(native_handle);
        numInts = sNumInts();
        numFds = sNumFds;
    }
    ~private_handle_t() {
        magic = 0;
    }

    //native_handle_t指针是否指向了一个private_handle_t结构体,是才可用
    static int validate(const native_handle* h) {
        const private_handle_t* hnd = (const private_handle_t*)h;
        if (!h || h->version != sizeof(native_handle) ||
                h->numInts != sNumInts() || h->numFds != sNumFds ||
                hnd->magic != sMagic)
        {
            ALOGE("invalid gralloc handle (at %p)", h);
            return -EINVAL;
        }
        return 0;
    }
#endif
};

#endif /* GRALLOC_PRIV_H_ */
