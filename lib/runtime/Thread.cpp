//
// Created by BraxtonN on 2/12/2018.
//

#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "Exe.h"
#include "Environment.h"
#include "VirtualMachine.h"
#include "Opcode.h"
#include "register.h"
#include "Manifest.h"

int32_t Thread::tid = 0;
thread_local Thread* thread_self = NULL;
List<Thread*> Thread::threads;
MUTEX Thread::threadsMonitor = NULL;
bool Thread::isAllThreadsSuspended = false;

/*
 * Local registers for the thread to use
 */
thread_local double registers[12];

void Thread::Startup() {
    threads.init();
    threads.setMax(MAX_THREADS);

    Thread* main = (Thread*)malloc(
            sizeof(Thread)*1);
    main->main = &env->methods[manifest.entryMethod];
    main->Create("Main");
}

int32_t Thread::Create(int32_t methodAddress, unsigned long stack_size) {
    if(methodAddress < 0 || methodAddress >= manifest.methods)
        return -1;
    Method* method = &env->methods[methodAddress];
    if(method->paramSize>0)
        return -2;
    if(stack_size < method->paramSize)
        return -3;

    Thread* thread = (Thread*)malloc(
            sizeof(Thread)*1);

    thread->mutex = NULL;
    MUTEX_INIT(&thread->mutex);
    thread->name.init();
    thread->main = method;
    thread->id = Thread::tid++;
    thread->dataStack = NULL;
    thread->callStack.init();
    thread->current = NULL;
    thread->suspendPending = false;
    thread->exceptionThrown = false;
    thread->suspended = false;
    thread->throwable.init();
    thread->exited = false;
    thread->daemon = false;
    thread->terminated = false;
    thread->state = THREAD_CREATED;
    thread->exitVal = 0;
    thread->stack_lmt = stack_size;

    pushThread(thread);

    stringstream ss;
    ss << "Thread@" << thread->id;
    thread->name = ss.str();
    return thread->id;
}

void Thread::Create(string name) {
    this->mutex = NULL;
    MUTEX_INIT(&this->mutex);
    this->name.init();

    this->name = name;
    this->id = Thread::tid++;
    this->dataStack = (StackElement*)__malloc(sizeof(StackElement)*STACK_SIZE);
    this->suspendPending = false;
    this->exceptionThrown = false;
    this->suspended = false;
    this->exited = false;
    this->throwable.init();
    this->daemon = false;
    this->terminated = false;
    this->state = THREAD_CREATED;
    this->exitVal = 0;
    this->stack_lmt = STACK_SIZE;
    this->callStack.init();

    for(unsigned long i = 0; i < STACK_SIZE; i++) {
        this->dataStack[i].object.object = NULL;
        this->dataStack[i].var=0;
    }

    pushThread(this);
}

void Thread::CreateDaemon(string name) {
    this->mutex = NULL;
    MUTEX_INIT(&this->mutex);
    this->name.init();

    this->name = name;
    this->id = Thread::tid++;
    this->dataStack = NULL;
    this->callStack.init();
    this->current = NULL;
    this->suspendPending = false;
    this->exceptionThrown = false;
    this->suspended = false;
    this->exited = false;
    this->daemon = true;
    this->terminated = false;
    this->throwable.init();
    this->state = THREAD_CREATED;
    this->exitVal = 0;
    this->stack_lmt=0;

    pushThread(this);
}

void Thread::pushThread(Thread *thread) {
    MUTEX_LOCK(threadsMonitor)
    threads.push_back(thread);
    MUTEX_UNLOCK(threadsMonitor)
}

void Thread::popThread(Thread *thread) {
    MUTEX_LOCK(threadsMonitor);
    threads.removefirst(thread);
    MUTEX_UNLOCK(threadsMonitor);
}

Thread *Thread::getThread(int32_t id) {
    for(int32_t i = 0 ; i < threads.size(); i++) {
        if(threads.get(i) != NULL && threads.get(i)->id == id)
            return threads.get(i);
    }

    return NULL;
}

void Thread::suspendSelf() {
    thread_self->suspended = true;
    thread_self->suspendPending = false;

    /*
	 * We call wait upon suspend. This function will
	 * sleep the thread most of the time. unsuspendThread() or
	 * resumeAllThreads() should be called to revive thread.
	 */
    thread_self->wait();
}

void Thread::wait() {
    const long sMaxRetries = 128 * 1024;

    long spinCount = 0;
    long retryCount = 0;

    this->state = THREAD_SUSPENDED;

    while (this->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;
#ifdef WIN32_
            Sleep(5);
#endif
#ifdef POSIX_
            usleep(5*POSIX_USEC_INTERVAL);
#endif
        } else if(this->state == THREAD_KILLED) {
            this->suspended = false;
            return;
        }
    }

    this->state = THREAD_RUNNING;
}

int Thread::start(int32_t id) {
    Thread *thread = getThread(id);

    if (thread == NULL)
        return 1;

    if(thread->state == THREAD_RUNNING)
        return 2;

    if(thread->terminated)
        return 4;

    thread->exited = false;
    thread->state = THREAD_CREATED;
#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            &vm->InterpreterThreadStart,       // thread function caller
            thread,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(thread->thread == NULL) return 3; // thread was not started
    else
        return waitForThread(thread);
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, env->InterpreterThreadStart, (void*) thread))
        return 3; // thread was not started
    else {
        return waitForThread(thread);
    }
#endif

}

int Thread::destroy(int64_t id) {
    if(id == thread_self->id)
        return 1; // cannot destroy thread_self

    Thread* thread = getThread(id);
    if(thread == NULL || thread->daemon)
        return 1;

    if (thread->state == THREAD_KILLED || thread->terminated)
    {
        if (thread->id == main_threadid)
        {
            return 3; // should never happen
        }
        else
        {
            popThread(thread);
            thread->term(); // terminate thread
            std::free (thread);
            return 0;
        }
    } else {
        return 2;
    }
}

int Thread::interrupt(int32_t id) {
    if(id == thread_self->id)
        return 1; // cannot interrupt thread_self

    Thread* thread = getThread(id);
    if(thread == NULL || thread->daemon)
        return 1;
    if(thread->terminated)
        return 2;

    return interrupt(thread);
}

void Thread::waitForThreadSuspend(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    suspendThread(thread);
    while (thread->state == THREAD_RUNNING && !thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
            {
                return; // give up
            } else if(thread->state >= THREAD_SUSPENDED)
                return;
        }
    }
}

void Thread::waitForThreadExit(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    while (!thread->exited)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
            {
                return; // give up
            } else if(thread->exited)
                return;
        }
    }
}

int Thread::waitForThread(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    while (thread->state != THREAD_RUNNING)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
            {
                return -255; // give up
            } else if(thread->state != THREAD_RUNNING)
                return 0;
        }
    }
    return 0;
}

void Thread::suspendAllThreads() {
    Thread* thread;
    isAllThreadsSuspended = true;

    for(unsigned int i= 0; i < threads.size(); i++) {
        thread=threads.get(i);

        if(thread!=NULL &&
           (thread->id != thread_self->id)){
            suspendThread(thread);
            waitForThreadSuspend(thread);
        }
    }
}

void Thread::resumeAllThreads() {
    Thread* thread;
    isAllThreadsSuspended = false;

    for(unsigned int i= 0; i < threads.size(); i++) {
        thread=threads.get(i);

        if(thread!=NULL &&
           (thread->id != thread_self->id)){
            unsuspendThread(thread);
        }
    }
}

int Thread::unsuspendThread(Thread *thread) {
    thread->suspended = false;
    return 0;
}

void Thread::suspendThread(Thread *thread) {
    if(thread->id == thread_self->id)
        suspendSelf();
    else {
        thread->suspendPending = true;
    }
}

void Thread::term() {
    this->state = THREAD_KILLED;
    this->terminated = true;
    MUTEX_UNLOCK(this->mutex);
    if(dataStack != NULL) {
        for(unsigned long i = 0; i < this->stack_lmt; i++) {
            GarbageCollector::self->freeObject(&dataStack[i].object);
        }
        std::free(dataStack); dataStack = NULL;
    }
    this->name.free();
}

int Thread::join(int32_t id) {
    if (id == thread_self->id)
        return 1;

    Thread* thread = getThread(id);
    if (thread == NULL || thread->daemon)
        return 1;
    if(thread->terminated)
        return 2;

    return threadjoin(thread);
}

int Thread::threadjoin(Thread *thread) {
    if (thread->state == THREAD_RUNNING)
    {
#ifdef WIN32_
        WaitForSingleObject(thread->thread, INFINITE);
        return 0;
#endif
#ifdef POSIX_
        if(pthread_join(thread->thread, NULL))
            return 3;
#endif
    }

    return 2;
}

void Thread::killAll() {
    Thread* thread;
    suspendAllThreads();

    for(unsigned int i = 0; i < threads.size(); i++) {
        thread = threads.get(i);

        if(thread != NULL && thread->id != thread_self->id) {
            if(thread->state == THREAD_RUNNING){
                interrupt(thread);
            }

            thread->term();
            waitForThreadExit(thread);
        } else if(thread != NULL){
            thread->term();
        }
    }
}

int Thread::interrupt(Thread *thread) {
    if (thread->state == THREAD_RUNNING)
    {
        if (thread->id == main_threadid)
        {
            /*
            * Shutdown all running threads
            * and de-allocate all allocated
            * memory. If we do not call join()
            * to wait for all other threads
            * regardless of what they are doing, we
            * stop them.
            */
            env->shutdown();
        }
        else
        {
            thread->state = THREAD_KILLED; // terminate thread
            return 0;
        }
    }

    return 2;
}

void Thread::shutdown() {
    if(!threads.empty()) {
        Thread::killAll();

        for(unsigned int i = 0; i < threads.size(); i++) {
            if(threads.get(i) != NULL) {
                std::free(threads.get(i));
            }
        }

        threads.free();
    }
}

void Thread::exit() {

    if(this->exceptionThrown) {
        this->exitVal = -800;
        cout << "Unhandled exception on thread " << name.str() << " (most recent call last):\n";
                cout << throwable.stackTrace.str();
        cout << endl << throwable.throwable->name.str() << " "
           << throwable.message.str() << "\n";
    } else {
        if(!daemon)
            this->exitVal = (int)dataStack[0].var;
        else
            this->exitVal = 0;
    }

    this->exited = true;
    this->state = THREAD_KILLED;
}

int Thread::startDaemon(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
(*threadFunc)(void *), Thread *thread) {
    if (thread == NULL || !thread->daemon)
        return 1;

    if(thread->state == THREAD_RUNNING)
        return 2;

    thread->exited = false;
#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            (LPTHREAD_START_ROUTINE)threadFunc,       // thread function caller
            thread,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(thread->thread == NULL) return 3; // thread was not started
    else
        return waitForThread(thread);
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, threadFunc, (void*) thread)!=0)
        return 3; // thread was not started
    else
        return waitForThread(thread);
#endif
}

#ifdef DEBUGGING
void printRegs() {
    cout << endl;
    cout << "Registers: \n";
    cout << "adx = " << registers[adx] << endl;
    cout << "cx = " << registers[cx] << endl;
    cout << "cmt = " << registers[cmt] << endl;
    cout << "ebx = " << registers[ebx] << endl;
    cout << "ecx = " << registers[ecx] << endl;
    cout << "ecf = " << registers[ecf] << endl;
    cout << "edf = " << registers[edf] << endl;
    cout << "ehf = " << registers[ehf] << endl;
    cout << "bmr = " << registers[bmr] << endl;
    cout << "egx = " << registers[egx] << endl;
    cout << "sp = " << registers[sp] << endl;
    cout << "fp = " << registers[fp] << endl;
    cout << endl;
}
#endif

double exponent(int64_t n){
    if (n < 100000){
        // 5 or less
        if (n < 100){
            // 1 or 2
            if (n < 10)
                return n*0.1;
            else
                return n*0.01;
        }else{
            // 3 or 4 or 5
            if (n < 1000)
                return n*0.001;
            else{
                // 4 or 5
                if (n < 10000)
                    return n*0.0001;
                else
                    return n*0.00001;
            }
        }
    } else {
        // 6 or more
        if (n < 10000000) {
            // 6 or 7
            if (n < 1000000)
                return n*0.000001;
            else
                return n*0.0000001;
        } else if(n < 1000000000) {
            // 8 to 10
            if (n < 100000000)
                return n*0.00000001;
            else {
                // 9 or 10
                if (n < 1000000000)
                    return n*0.000000001;
                else
                    return n*0.0000000001;
            }
        } else if(n < 1000000000000000) {
            // 11 to 15
            if (n < 100000000000)
                return n*0.00000000001;
            else {
                // 12 to 15
                if (n < 1000000000000)
                    return n*0.000000000001;
                else if (n < 10000000000000)
                    return n*0.0000000000001;
                else if (n < 100000000000000)
                    return n*0.00000000000001;
                else
                    return n*0.000000000000001;
            }
        }
        else {
            return n*0.0000000000000001;
        }
    }
}

/**
 * This tells the virtual machine to process code
 * when executing a finally block or the regular processing
 * of a function.
 */
short int startAddress = 0;

/*
 * We need this to keep track of which finally block we are executing
 */
FinallyTable finallyTable;
long count = 0;

void Thread::exec() {

    uint64_t tmp;
    int64_t val;
    SharpObject* o;
    Object* o2;
    void* opcodeStart = (startAddress == 0) ?  (&&interp) : (&&finally) ;
    Method* finnallyMethod;

    _initOpcodeTable
    try {
        for (;;) {
            /* We dont want to always run finally code when we start a thread */
            if(startAddress == 0) goto interp;
            finnallyMethod = current;

            finally:
            if((pc <finallyTable.start_pc || pc > finallyTable.end_pc)
               && current==finnallyMethod)
                return;

            interp:
            count++;
            //cout << count << endl;
            if (suspendPending)
                suspendSelf();
            if (state == THREAD_KILLED)
                return;

            DISPATCH();
            _NOP:
                _brh
            _INT:
                vm->sysInterrupt(GET_Da(cache[pc]));
                _brh
            _MOVI:
                registers[cache[pc+1]]=GET_Da(cache[pc]); pc++;
                _brh
            RET:
                returnFrame(vm->returnMethod();)
                _brh
            HLT:
                state=THREAD_KILLED;
                _brh
            NEWARRAY:
                dataStack[(long long)++registers[sp]].object =
                        GarbageCollector::self->newObject(registers[GET_Da(cache[pc])]);
                _brh
            CAST:
                o2->castObject(registers[GET_Da(cache[pc])]);
                _brh
            MOV8:
                registers[GET_Ca(cache[pc])]=(int8_t)registers[GET_Cb(cache[pc])];
                _brh
            MOV16:
                registers[GET_Ca(cache[pc])]=(int16_t)registers[GET_Cb(cache[pc])];
                _brh
            MOV32:
                registers[GET_Ca(cache[pc])]=(int32_t)registers[GET_Cb(cache[pc])];
                _brh
            MOV64:
                registers[GET_Ca(cache[pc])]=(int64_t)registers[GET_Cb(cache[pc])];
                _brh
            MOVU8:
                registers[GET_Ca(cache[pc])]=(uint8_t)registers[GET_Cb(cache[pc])];
                _brh
            MOVU16:
                registers[GET_Ca(cache[pc])]=(uint16_t)registers[GET_Cb(cache[pc])];
                _brh
            MOVU32:
                registers[GET_Ca(cache[pc])]=(uint32_t)registers[GET_Cb(cache[pc])];
                _brh
            MOVU64:
                registers[GET_Ca(cache[pc])]=(uint64_t)registers[GET_Cb(cache[pc])];
                _brh
            RSTORE:
                dataStack[(int64_t)++registers[sp]].var = registers[GET_Da(cache[pc])];
                _brh
            ADD:
                registers[cache[pc+1]]=registers[GET_Ca(cache[pc])]+registers[GET_Cb(cache[pc])]; pc++;
                _brh
            SUB:
                registers[cache[pc+1]]=registers[GET_Ca(cache[pc])]-registers[GET_Cb(cache[pc])]; pc++;
                _brh
            MUL:
                registers[cache[pc+1]]=registers[GET_Ca(cache[pc])]*registers[GET_Cb(cache[pc])]; pc++;
                _brh
            DIV:
                registers[cache[pc+1]]=registers[GET_Ca(cache[pc])]/registers[GET_Cb(cache[pc])]; pc++;
                _brh
            MOD:
                registers[cache[pc+1]]=(int64_t)registers[GET_Ca(cache[pc])]%(int64_t)registers[GET_Cb(cache[pc])]; pc++;
                _brh
            IADD:
                registers[GET_Ca(cache[pc])]+=GET_Cb(cache[pc]);
                _brh
            ISUB:
                registers[GET_Ca(cache[pc])]-=GET_Cb(cache[pc]);
                _brh
            IMUL:
                registers[GET_Ca(cache[pc])]*=GET_Cb(cache[pc]);
                _brh
            IDIV:
                registers[GET_Ca(cache[pc])]/=GET_Cb(cache[pc]);
                _brh
            IMOD:
                registers[GET_Ca(cache[pc])]=(int64_t)registers[GET_Ca(cache[pc])]%(int64_t)GET_Cb(cache[pc]);
                _brh
            POP:
                --registers[sp];
                _brh
            INC:
                registers[GET_Da(cache[pc])]++;
                _brh
            DEC:
                registers[GET_Da(cache[pc])]--;
                _brh
            MOVR:
                registers[GET_Ca(cache[pc])]=registers[GET_Cb(cache[pc])];
                _brh
            IALOAD:
                o = (dataStack+(uint64_t)registers[sp])->object.object;
                if(o != NULL) {
                    registers[GET_Ca(cache[pc])] = o->HEAD[(uint64_t)registers[GET_Cb(cache[pc])]];
                } else throw Exception(Environment::NullptrException, "");
                _brh
            BRH:
                pc=registers[adx];
                _brh_NOINCREMENT
            IFE:
                if(registers[cmt]) {
                    pc=registers[adx]; _brh_NOINCREMENT
                } else  _brh
            IFNE:
                if(registers[cmt]==0) {
                    pc=registers[adx]; _brh_NOINCREMENT
                } else  _brh
            LT:
                registers[cmt]=registers[GET_Ca(cache[pc])]<registers[GET_Cb(cache[pc])];
                _brh
            GT:
                registers[cmt]=registers[GET_Ca(cache[pc])]>registers[GET_Cb(cache[pc])];
                _brh
            LTE:
                registers[cmt]=registers[GET_Ca(cache[pc])]<=registers[GET_Cb(cache[pc])];
                _brh
            GTE:
                registers[cmt]=registers[GET_Ca(cache[pc])]>=registers[GET_Cb(cache[pc])];
                _brh
            MOVL:
                o2 = &dataStack[(int64_t)registers[fp]+GET_Da(cache[pc])].object;
                _brh
            MOVSL:
                o2 = &dataStack[(int64_t)registers[sp]+GET_Da(cache[pc])].object;
                _brh
            MOVBI:
                registers[bmr]=GET_Da(cache[pc]) + exponent(cache[pc + 1]); pc++;
                _brh
            SIZEOF:
                CHECK_NULLOBJ(registers[GET_Da(cache[pc])]=o2->object->size;)
                _brh
            PUT:
                cout << registers[GET_Da(cache[pc])];
                _brh
            PUTC:
                printf("%c", (char)registers[GET_Da(cache[pc])]);
                _brh
            CHECKLEN:
            CHECK_NULLOBJ(
                    if(registers[GET_Da(cache[pc])]<o2->object->size) { _brh }
                    else {
                        stringstream ss;
                        ss << "Access to Object at: " << registers[GET_Da(cache[pc])] << " size is " << o2->object->size;
                        throw Exception(Environment::IndexOutOfBoundsException, ss.str());
                    }
                )
            GOTO:
                pc = GET_Da(cache[pc]);
                _brh_NOINCREMENT
            LOADPC:
                registers[GET_Da(cache[pc])] = pc;
                _brh
            PUSHOBJ:
                dataStack[(int64_t)++registers[sp]].object=o2;
                _brh
            DEL:
                GarbageCollector::self->freeObject(o2);
                _brh
            CALL:
                vm->executeMethod(GET_Da(cache[pc]));
                _brh_NOINCREMENT
            NEWCLASS:
                CHECK_NULL(dataStack[(int64_t)++registers[sp]].object = GarbageCollector::self->newObject(env->findClassBySerial(GET_Da(cache[pc])));)
                _brh
            MOVN: // TODO: check to see if we really need this instruction
                CHECK_NULLOBJ(o2 = &o2->object->node[GET_Da(cache[pc])];)
                _brh
            SLEEP:
                __os_sleep((int64_t)registers[GET_Da(cache[pc])]);
                _brh
            TEST:
                registers[cmt]=registers[GET_Ca(cache[pc])]==registers[GET_Cb(cache[pc])];
                _brh
            TNE:
                registers[cmt]=registers[GET_Ca(cache[pc])]!=registers[GET_Cb(cache[pc])];
                _brh
            LOCK:
                CHECK_NULLOBJ(MUTEX_LOCK(o2->object->mutex);)
                _brh
            ULOCK:
                CHECK_NULLOBJ(MUTEX_UNLOCK(o2->object->mutex);)
                _brh
            EXP:
                registers[bmr] = exponent(registers[GET_Da(cache[pc])]);
                _brh
            MOVG:
                o2 = env->globalHeap+GET_Da(cache[pc]);
                _brh
            MOVND:
                CHECK_NULLOBJ(o2 = &o2->object->node[(int64_t)registers[GET_Da(cache[pc])]];)
                _brh
            NEWOBJARRAY:
                CHECK_NULLOBJ(dataStack[(int64_t)++registers[sp]].object = GarbageCollector::self->newObjectArray(registers[GET_Da(cache[pc])]);)
                _brh
            NOT:
                registers[GET_Ca(cache[pc])]=!registers[GET_Cb(cache[pc])];
                _brh
            SKIP:
                pc += GET_Da(cache[pc]);
                _brh
            LOADVAL:
                registers[GET_Da(cache[pc])]=dataStack[(int64_t)registers[sp]--].var;
                _brh
            SHL:
                registers[cache[pc+1]]=(int64_t)registers[GET_Ca(cache[pc])]<<(int64_t)registers[GET_Cb(cache[pc])]; pc++;
                _brh
            SHR:
                registers[cache[pc+1]]=(int64_t)registers[GET_Ca(cache[pc])]>>(int64_t)registers[GET_Cb(cache[pc])]; pc++;
                _brh
            SKPE:
                if(registers[cmt]) {
                    pc+=GET_Da(cache[pc]); _brh_NOINCREMENT
                } else _brh
            SKNE:
                if(registers[cmt]==0) {
                    pc+=GET_Da(cache[pc]); _brh_NOINCREMENT
                } else _brh
            AND:
                registers[cmt]=registers[GET_Ca(cache[pc])]&&registers[GET_Cb(cache[pc])];
                _brh
            UAND:
                registers[cmt]=(int64_t)registers[GET_Ca(cache[pc])]&(int64_t)registers[GET_Cb(cache[pc])];
                _brh
            OR:
                registers[cmt]=(int64_t)registers[GET_Ca(cache[pc])]|(int64_t)registers[GET_Cb(cache[pc])];
                _brh
            UNOT:
                registers[cmt]=(int64_t)registers[GET_Ca(cache[pc])]^(int64_t)registers[GET_Cb(cache[pc])];
                _brh
            THROW:
                throw Exception("", false);
                _brh
            CHECKNULL:
                CHECK_NULL(registers[cmt]=o2->object==NULL;)
                _brh
            RETURNOBJ:
                dataStack[(int64_t)registers[fp]].object=o2;
                _brh
            NEWCLASSARRAY:
                CHECK_NULL(
                        dataStack[(int64_t)++registers[sp]].object = GarbageCollector::self->newObjectArray(registers[GET_Ca(cache[pc])],
                                                                           env->findClassBySerial(GET_Cb(cache[pc])));
                )
                _brh
            NEWSTRING:
                GarbageCollector::self->createStringArray(o2, env->getStringById(GET_Da(cache[pc])));
                _brh
            ADDL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var+=registers[GET_Ca(cache[pc])];
                _brh
            SUBL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var-=registers[GET_Ca(cache[pc])];
                _brh
            MULL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var*=registers[GET_Ca(cache[pc])];
                _brh
            DIVL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var/=registers[GET_Ca(cache[pc])];
                _brh
            MODL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].modul(registers[GET_Ca(cache[pc])]);
                _brh
            IADDL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var+=GET_Ca(cache[pc]);
                _brh
            ISUBL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var-=GET_Ca(cache[pc]);
                _brh
            IMULL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var*=GET_Ca(cache[pc]);
                _brh
            IDIVL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var/=GET_Ca(cache[pc]);
                _brh
            IMODL:
                val = dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var;
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var=val%GET_Ca(cache[pc]);
                _brh
            LOADL:
                registers[GET_Ca(cache[pc])]=dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var;
                _brh
            IALOAD_2:
                CHECK_INULLOBJ(
                        registers[GET_Ca(cache[pc])] = o2->object->HEAD[(uint64_t)registers[GET_Cb(cache[pc])]];
                )
                _brh
            POPOBJ:
                o2 = &dataStack[(int64_t)registers[sp]--].object;
                _brh
            SMOVR:
                dataStack[(int64_t)registers[sp]+GET_Cb(cache[pc])].var=registers[GET_Ca(cache[pc])];
                _brh
            SMOVR_2:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].var=registers[GET_Ca(cache[pc])];
                _brh
            ANDL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].andl(registers[GET_Ca(cache[pc])]);
                _brh
            ORL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].orl(registers[GET_Ca(cache[pc])]);
                _brh
            NOTL:
                dataStack[(int64_t)registers[fp]+GET_Cb(cache[pc])].notl(registers[GET_Ca(cache[pc])]);
                _brh
            RMOV:
                CHECK_NULLOBJ(
                    o2->object->HEAD[(int64_t)registers[GET_Ca(cache[pc])]]=registers[GET_Cb(cache[pc])];
                )
                _brh
            SMOV:
                registers[GET_Ca(cache[pc])]=dataStack[(int64_t)registers[sp]+GET_Cb(cache[pc])].var;
                _brh
            LOADPC_2:
                registers[GET_Ca(cache[pc])]=pc+GET_Cb(cache[pc]);
                _brh
            RETURNVAL:
                dataStack[(int64_t)registers[fp]].var=registers[GET_Da(cache[pc])];
                _brh
            ISTORE:
                dataStack[(int64_t)++registers[sp]].var = GET_Da(cache[pc]);
                _brh
            ISTOREL:
                dataStack[(int64_t)registers[fp]+GET_Da(cache[pc])].var=cache[pc+1]; pc++;
                _brh

        }
    } catch (bad_alloc &e) {
        cout << "std::bad_alloc\n";
        // TODO: throw out of memory error
    } catch (Exception &e) {
        throwable = e.getThrowable();
        exceptionThrown = true;

        if(state == THREAD_KILLED) {
            vm->fillStackTrace(throwable.stackTrace);
            return;
        }
        vm->Throw(&dataStack[(int64_t)registers[sp]].object);

        DISPATCH();
    }
}

void Thread::dbg() {
    int i = 0;
}

void __os_sleep(int64_t INTERVAL) {
#ifdef WIN32_
    Sleep(INTERVAL);
#endif
#ifdef POSIX_
    usleep(INTERVAL);
#endif
}
