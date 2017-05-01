//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_GC_H
#define SHARP_GC_H

#include "../../../stdimports.h"
#include "../internal/Monitor.h"
#include "../oo/Object.h"
#include "../internal/sh_asp.h"


struct _gc_object;

class GC {
public:
    static GC* gc;

    static void GCStartup();
    static void GCShutdown();
    static void _init_GC();
    static void _collect_GC_CONCURRENT();
    static void _collect_GC_EXPLICIT();
    static void _insert(Sh_object*);
    static void _insert_stack(Sh_object*, unsigned long);
    static void _insert_stack(data_stack* st, unsigned long sz);
private:
    Monitor mutex;
    _gc_object* gc_alloc_heap;
    unsigned long allocptr;
    static void _collect();

    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
     _GCThread_start(void *);

    void _GC_run();
};

struct _gc_object{
    double *HEAD;
    Sh_object *_Node, *prev, *nxt;
    Sh_object* _rNode;

    void free(){
        if(HEAD != NULL)
            std::free(HEAD); HEAD = NULL;
    }
};

#define _GC_CAP_THRESHOLD 1.5

#define GC_SLEEP_INTERVAL 10

/**
 * This number must be low considering that the Garbage collector will
 * not be collecting data every second. We want the garbage collector
 * to be asleep as much as possible.
 */
#define GC_SPIN_MULTIPLIER 512

#endif //SHARP_GC_H
