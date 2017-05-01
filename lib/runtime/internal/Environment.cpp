//
// Created by BraxtonN on 2/17/2017.
//

#include "Environment.h"
#include "Exe.h"
#include "../oo/Exception.h"
#include "../oo/Object.h"
#include "../alloc/GC.h"

ClassObject Environment::Throwable;
ClassObject Environment::StackOverflowErr;
ClassObject Environment::RuntimeException;
ClassObject Environment::ThreadStackException;
ClassObject Environment::IndexOutOfBoundsException;
ClassObject Environment::NullptrException;

ClassObject *Environment::findClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name.str() == name)
            return &env->classes[i];
    }

    throw Exception("class not found `" + name + "'");
}

ClassObject *Environment::tryFindClass(string name) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].name.str() == name)
            return &env->classes[i];
    }

    return NULL;
}

ClassObject *Environment::findClass(int64_t id) {
    for (uint64_t i = 0; i < manifest.classes; i++) {
        if (env->classes[i].id == id)
            return &env->classes[i];
    }

    stringstream msg;
    msg << "class not found @id:" << id;
    throw Exception(msg.str());
}

void Environment::shutdown() {
    for(unsigned int i = 0; i < manifest.addresses; i++) {
        this->__address_spaces[i].free();
    }
    std::free (this->__address_spaces);
    for(int64_t i = 0; i < manifest.strings; i++)
        this->strings->value.free();
    std::free (this->strings);

    for(int64_t i = 0; i < manifest.classes; i++)
        this->classes->free();
    std::free (this->classes);

    // TODO: free objects and aux classes
    for(int64_t i = 0; i < manifest.classes; i++)
        this->global_heap[i].free();
    std::free (this->global_heap);

    this->IndexOutOfBoundsException.free();
    this->NullptrException.free();
    this->RuntimeException.free();
    this->StackOverflowErr.free();
    this->ThreadStackException.free();
    this->Throwable.free();

    manifest.application.free();
    manifest.executable.free();
    manifest.version.free();
}

void Environment::newClass(int64_t object, int64_t klass) {
    if(global_heap[object].mark == gc_green)
        global_heap[object].free();

    global_heap[object].mark = gc_green;
}

void Environment::newClass(Sh_object* object, int64_t klass) {
    if(object->mark == gc_green)
        object->free();

    object->mark = gc_green;
}

void Environment::init(Sh_object* objects, int64_t size) {
    if(objects != NULL)
    {
        Sh_object* ptr=&objects[0];
        for(int64_t i = 0; i < size; i++) {
            ptr->HEAD=NULL;
            ptr->_Node = NULL, ptr->prev=NULL,ptr->nxt=NULL;
            ptr->mark = gc_orange;
            ptr->size = 0;
            ptr->monitor = Monitor();
            ptr->refs.init();

            ptr++;
        }
    }

}

void Environment::init(_gc_object* objects, int64_t size) {
    if(objects != NULL)
    {
        _gc_object* ptr=&objects[0];
        for(int64_t i = 0; i < size; i++) {
            ptr->HEAD=NULL;
            ptr->_Node = NULL, ptr->prev=NULL,ptr->nxt=NULL;

            ptr++;
        }
    }

}

void Environment::newNative(Sh_object *object, int8_t type) {
    if(object->mark == gc_green)
        object->free();
}

void Environment::newArray(Sh_object *object, int64_t len) {
    if(object->mark == gc_green)
        object->free();

}

void Environment::newRefrence(Sh_object *object) {
    if(object->mark == gc_green)
        object->free();

}

void Environment::free(Sh_object *objects, int64_t len) {
    if(len > 0 && objects != NULL) {
        for(int64_t i = 0; i < len; i++) {
            // TodO: implement
            objects[i].free();
        }
        std::free(objects);
    }
}

void Environment::gcinsert_stack(Sh_object *objects, int64_t len) {
    if(len > 0 && objects != NULL) {
        GC::_insert_stack(objects, len);
    }
}

void Environment::freesticky(_gc_object *objects, int64_t len) {
    if(len > 0 && objects != NULL) {
        for(int64_t i = 0; i < len; i++) {
            // TodO: implement
            objects[i].free();
        }
    }
}

void Environment::init(data_stack *st, int64_t stack_size) {
    if(stack_size > 0 && st != NULL) {
        for(int64_t i = 0; i < stack_size; i++) {
            st[i].object.HEAD=NULL;
            st[i].object._Node = NULL, st[i].object.prev=NULL,st[i].object.nxt=NULL;
            st[i].object.mark = gc_orange;
            st[i].object.size = 0;
            st[i].object.monitor = Monitor();
            st[i].object.refs.init();
        }
    }
}

nString Environment::getstring(int64_t ref) {
    if(ref < 0 || ref >= manifest.strings)
        return nString("");
    else if(ref == strings[ref].id)
        return strings[ref].value;
    else {
        for(unsigned int i = 0; i < manifest.strings; i++) {
            if(strings[i].id == ref)
                return strings[i].value;
        }
    }

    return nString("");
}
