//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"
#include "../alloc/GC.h"

Sh_object::Sh_object() {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->HEAD = NULL;
    nxt = NULL, prev=NULL;
    _Node = NULL;
    refs.init();
}

Sh_object::Sh_object(int64_t type) {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->HEAD = NULL;
    nxt = NULL, prev=NULL;
    _Node = NULL;
    refs.init();
}

void Sh_object::free() {
    mark = gc_orange;
    if(HEAD != NULL)
        std::free(HEAD); HEAD = NULL;

    for(unsigned long i=0; i < refs.size(); i++) {
        Sh_InvRef(refs.at(i));
    }

    size=0;
    refs.free();
}

void Sh_object::copy_object(Sh_object *pObject) {
    // TODO: implement
}

void Sh_object::createnative(int64_t size) {
    if(mark != gc_green) {
        HEAD= (double*)memalloc(sizeof(double)*size);
        this->size=size;
        prev = NULL, nxt=NULL;
        _Node=NULL, _rNode=NULL;
        mark = gc_green;

        for(int64_t i=0; i<size; i++){
            _nativewrite(i,0)
        }
    }
}

void Sh_object::_Sh_IncRef(Sh_object *ptr) {
    this->refs.push_back(ptr);
    ptr->HEAD=HEAD;
    ptr->nxt==nxt;
    ptr->prev=prev;
    ptr->size=size;
    ptr->_Node=_Node;
    ptr->_rNode=this;
}

void Sh_object::createstr(int64_t ref) {
    if(mark == gc_green) {
        GC::_insert(this);
    }

    nString str = env->getstring(ref);
    if(str.len == 0)
        HEAD = NULL;
    else
        HEAD= (double*)memalloc(sizeof(double)*str.len);
    this->size=str.len;
    prev = NULL, nxt=NULL;
    _Node=NULL, _rNode=NULL;
    mark = gc_green;

    for(int64_t i=0; i<size; i++){
        _nativewrite(i,str.chars[i])
    }
}
