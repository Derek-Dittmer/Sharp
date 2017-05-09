//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_EXCEPTION_H
#define SHARP_EXCEPTION_H

#include "../../../stdimports.h"
#include "string.h"
#include <stdexcept>

class ClassObject;
class Method;

class Throwable {
public:
    Throwable()
            :
            throwable(NULL),
            message(),
            native(false),
            stackTrace()
    {
    }

    void init() {
        throwable =NULL;
        message.init();
        this->stackTrace.init();
        native = false;
    }

    Throwable(ClassObject* throwable, string message, bool native)
            :
            throwable(throwable),
            native(native)
    {
        this->message.init();
        this->stackTrace.init();
        this->message=message;
    }

    void operator=(Throwable& t) {
        throwable=t.throwable;
        message = t.message;
        stackTrace = t.stackTrace;
        native = t.native;
    }

    void drop();

    bool native;
    ClassObject* throwable;
    nString message, stackTrace;
};

class Exception : public std::runtime_error {
public:
    Exception(const char *msg, bool native = true);
    Exception(const std::string &__arg, bool native = true);
    Exception(ClassObject* throwable, const std::string &__arg, bool native = true);

    ~Exception();

    Throwable& getThrowable() { return throwable; }

    Throwable throwable;

private:
    void setupFrame();
};

struct ExceptionTable{
    int64_t start_pc, end_pc;
    int64_t local;
    nString klass;
};

#define EXCEPTION_PRINT_MAX 20

#endif //SHARP_EXCEPTION_H
