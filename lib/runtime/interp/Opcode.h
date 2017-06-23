//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_OPCODE_H
#define SHARP_OPCODE_H

#include "../../../stdimports.h"

/**
 * Bit layout
 *
 * Class D Instruction
    | 123456789012345678901234567890123456789012345678901234 | 12345678 |  max: 7FFFFFFFFFFFFF
    |						argument						  |  opcode  |

    max 36028797018963967 min -36028797018963968

    Class C Instruction
    | 190123456789012345678901234 | 0123456789012345678901234567 | 8 | 12345678 | max: FFFFFFF
    |			 argument		 |			argument		      | +-|  opcode  |

    max 134217727 min -134217727
 */

#define OPCODE_MASK 0xff

#define SET_Ei(i, op) (i=op)

#define SET_Di(i, op, a1) (i=((op) | ((int64_t)a1 << 8)))

#define SET_Ci(i, op, a1, n, a2) (i=((op | ((n & 1) << 8) | ((int64_t)a1 << 9)) | ((int64_t)a2 << 36)))

#define GET_OP(i) (i & OPCODE_MASK)
#define GET_Da(i) (i >> 8)
#define GET_Ca(i) (((i >> 8) & 1) ? (-1*(i >> 9 & 0x7FFFFFF)) : (i >> 9 & 0x7FFFFFF))
#define GET_Cb(i) (i >> 36)

#ifdef DEBUGGING
#include "../internal/sh_asp.h"
int64_t getop(int64_t);
int64_t get_da(int64_t);
int64_t get_ca(int64_t);
int64_t get_cb(int64_t);
int64_t get_reg(int64_t);
data_stack* stack_at(int64_t pos, bool usefp = true);
sh_asp* curr_func();
void print_stack();
#endif

#define DA_MAX 36028797018963967
#define DA_MIN -36028797018963968

#define CA_MAX 134217727
#define CA_MIN -134217727

#define DISPATCH() if(pc<cache_size) { goto *opcode_table[GET_OP(cache[pc])]; } else { throw Exception("invalid branch/dispatch; check your assembly code?"); }

#define _brh pc++; /*for(int i = 0; i < 9895; i++){ i++; }*/ goto interp;
#define _brh_NOINCREMENT goto interp;

#define NOP _brh

#define _int(x) vm->interrupt(x); _brh

#define movi(x) __rxs[cache[pc+1]]=x; pc++; _brh

#define ret ret_frame(return_asp();) _brh

#define hlt state=thread_killed; _brh

#define _newi(x) CHK_NULL(ptr->createnative(__rxs[x]);) _brh

#define _newstr(x) CHK_NULL(ptr->createstr(x);) _brh

#define check_cast(x) CHK_NULL(ptr->checkcast(x);) _brh

#define mov8(r,x) __rxs[r]=(int8_t)__rxs[x]; _brh

#define mov16(r,x) __rxs[r]=(int16_t)__rxs[x]; _brh

#define mov32(r,x) __rxs[r]=(int32_t)__rxs[x]; _brh

#define mov64(r,x) __rxs[r]=(int64_t)__rxs[x]; _brh

#define pushr(r) __stack[(int64_t)++__rxs[sp]].var = __rxs[r]; _brh

#define _add(r,x) __rxs[cache[pc+1]]=__rxs[r]+__rxs[x]; pc++; _brh

#define _sub(r,x) __rxs[cache[pc+1]]=__rxs[r]-__rxs[x]; pc++; _brh

#define _mul(r,x) __rxs[cache[pc+1]]=__rxs[r]*__rxs[x]; pc++; _brh

#define _div(r,x) __rxs[cache[pc+1]]=__rxs[r]/__rxs[x]; pc++; _brh

#define mod(r,x) __rxs[cache[pc+1]]=(int64_t)__rxs[r]%(int64_t)__rxs[x]; pc++; _brh

#define _iadd(r,x) __rxs[r]+=x; _brh

#define _isub(r,x) __rxs[r]-=x; _brh

#define _imul(r,x) __rxs[r]*=x; _brh

#define _idiv(r,x) __rxs[r]/=x; _brh

#define imod(r,x) __rxs[r]=(int64_t)__rxs[r]%(int64_t)x; _brh

#define _addl(r,x) __stack[(int64_t)__rxs[fp]+x].var+=__rxs[r]; _brh

#define _subl(r,x) __stack[(int64_t)__rxs[fp]+x].var-=__rxs[r]; _brh

#define _mull(r,x) __stack[(int64_t)__rxs[fp]+x].var*=__rxs[r]; _brh

#define _divl(r,x) __stack[(int64_t)__rxs[fp]+x].var/=__rxs[r]; _brh

#define modl(r,x)  __stack[(int64_t)__rxs[fp]+x].modul((int64_t)__rxs[r]); _brh

#define _andl(r,x) __stack[(int64_t)__rxs[fp]+x].andl((int64_t)__rxs[r]); _brh

#define _orl(r,x) __stack[(int64_t)__rxs[fp]+x].orl((int64_t)__rxs[r]); _brh

#define _notl(r,x) __stack[(int64_t)__rxs[fp]+x].notl((int64_t)__rxs[r]); _brh

#define _iaddl(r,x) __stack[(int64_t)__rxs[fp]+x].var+=r; _brh

#define _isubl(r,x) __stack[(int64_t)__rxs[fp]+x].var-=r; _brh

#define _imull(r,x) __stack[(int64_t)__rxs[fp]+x].var*=r; _brh

#define _idivl(r,x) __stack[(int64_t)__rxs[fp]+x].var/=r; _brh

#define imodl(r,x)  __stack[(int64_t)__rxs[fp]+x].modul((int64_t)r); _brh

#define _pop --__rxs[sp]; _brh

#define inc(r) __rxs[r]++; _brh

#define dec(r) __rxs[r]--; _brh

#define smov(r,ofset) __rxs[r]=__stack[(int64_t)(__rxs[0x0000]+ofset)].var; _brh

#define smovr(r,ofset) __stack[(int64_t)__rxs[0x0000]+ofset].var=__rxs[r];  _brh

#define smovobj(ofset) __stack[(int64_t)__rxs[0x0000]+ofset].object.mutate(ptr);  _brh

#define movr(r,x) __rxs[r]=__rxs[x]; _brh

#define movx(r,x) _nativeread(r,x) _brh // TODO: make a movxd to ret value from register

#define lt(r,x) __rxs[0x0002]=__rxs[r]<__rxs[x]; _brh

#define brh pc=__rxs[0x0000]; _brh_NOINCREMENT

#define bre if(__rxs[0x0002]){ pc=__rxs[0x0000]; _brh_NOINCREMENT }else _brh // TODO: remove this legacy instruction

#define ife if(__rxs[0x0002]){ pc=__rxs[0x0000]; _brh_NOINCREMENT }else  _brh

#define ifne if(__rxs[0x0002]==0){ pc=(int64_t)__rxs[0x0000]; _brh_NOINCREMENT }else _brh

#define gt(r,x) __rxs[0x0002]=__rxs[r]>__rxs[x]; _brh

#define gte(r,x) __rxs[0x0002]=__rxs[r]>=__rxs[x]; _brh

#define lte(r,x) __rxs[0x0002]=__rxs[r]<=__rxs[x]; _brh

#define movl(x) ptr=&__stack[(int64_t)__rxs[fp]+x].object; _brh

#define movsl(x) ptr=&__stack[(int64_t)__rxs[sp]+x].object; _brh

#define movbi(x) __rxs[0x0008]=x; pc++; _brh

#define _sizeof(r) CHK_NULL(__rxs[r]=ptr->size;) _brh

#define _put(r) cout << __rxs[r]; _brh

#define putc(r) printf("%c", (char)__rxs[r]); _brh

#define _checklen(r) if(__rxs[r]>=ptr->size) \
    { \
        stringstream ss; \
        ss << "access to object at: " << __rxs[r] << " size is " << ptr->size; \
        throw Exception(&Environment::IndexOutOfBoundsException, ss.str()); \
    }else { _brh }

#define _goto(x) pc = x; _brh_NOINCREMENT

#define _loadx(r) __rxs[r] = pc; _brh

#define pushref() CHK_NULL(ptr->inc_ref(&__stack[(int64_t)++__rxs[sp]].object);) _brh

#define delref(x) CHK_NULL(ptr->del_ref();) _brh

#define _init_frame() init_frame(); _brh

#define call(x) call_asp(x); _brh_NOINCREMENT

#define new_class(x) CHK_NULL(ptr->createclass(x);) _brh

#define movn(x) CHK_NULL(ptr = ptr->size ? &ptr->_Node[x] : NULL;) _brh

#define _sleep(r) __os_sleep((int64_t)__rxs[r]); _brh

#define test(r,x) __rxs[0x0002]=__rxs[r]==__rxs[x]; _brh

#define tne(r,x) __rxs[0x0002]=__rxs[r]!=__rxs[x]; _brh

#define __lock(r) CHK_NULL(ptr->lock((int64_t)__rxs[r]);) _brh

#define __ulock() CHK_NULL(ptr->unlock();) _brh

#define exp(r) __rxs[0x0008] = exponent(__rxs[r]); _brh

#define del() CHK_NULL(ptr->null();) _brh

#define movg(x) ptr = &env->global_heap[x]; _brh

#define movnd(x) CHK_NULL(ptr = &ptr->_Node[(int64_t)__rxs[x]];) _brh

#define sdelref() __stack[(int64_t)__rxs[sp]--].object.del_ref(); _brh

#define new_obj_arry(x) CHK_NULL(ptr->createobjectarry(__rxs[x]);) _brh

#define _not(r,x) __rxs[r]=!__rxs[x]; _brh

#define skp(x) pc += x; _brh

#define loadf(r, x) __rxs[r]=pc+x; _brh

#define popr(r) __rxs[r]=__stack[(int64_t)__rxs[sp]--].var; _brh

#define shl(r,x) __rxs[cache[pc+1]]=(int64_t)__rxs[r]<<(int64_t)__rxs[x]; pc++; _brh

#define shr(r,x) __rxs[cache[pc+1]]=(int64_t)__rxs[r]>>(int64_t)__rxs[x]; pc++; _brh

#define popref() CHK_NULL(ptr->mutate(&__stack[(int64_t)__rxs[sp]--].object);) _brh

#define mutl(x) CHK_NULL(__stack[(int64_t)__rxs[fp]+x].object.inc_ref(ptr);) _brh

#define skpe(x) if(__rxs[0x0002]){ pc+=x; _brh_NOINCREMENT }else  _brh

#define skpne(x) if(__rxs[0x0002]==0){ pc+=x; _brh_NOINCREMENT }else _brh

#define _and(r,x) __rxs[0x0002]=__rxs[r]&&__rxs[x];  _brh

#define uand(r,x) __rxs[0x0002]=(int64_t)__rxs[r]&(int64_t)__rxs[x];  _brh

#define _or(r,x) __rxs[0x0002]=(int64_t)__rxs[r]|(int64_t)__rxs[x];  _brh

#define _unot(r,x) __rxs[0x0002]=(int64_t)__rxs[r]^(int64_t)__rxs[x];  _brh

#define _throw() throw Exception("", false);  _brh

#define _chknull() CHK_NULL(__rxs[0x0002]=ptr->isnull();) _brh

#define returnref() CHK_NULL(ptr->inc_ref(&__stack[(int64_t)__rxs[fp]-5].object);) _brh

#define sdel() CHK_NULL(__stack[(int64_t)__rxs[sp]--].object.null();) _brh

#define _init_opcode_table \
    static void* opcode_table[] = { \
        &&_NOP,	\
        &&_INT,	\
        &&MOVI,	\
        &&RET,	\
        &&HLT,	\
        &&NEWi,	\
        &&CHECK_CAST,	\
		&&MOV8,	\
		&&MOV16,    \
		&&MOV32,	\
		&&MOV64,	\
		&&PUSHR,	\
		&&ADD,	\
		&&SUB,	\
		&&MUL,	\
		&&DIV,	\
		&&MOD,	\
		&&POP,	\
		&&INC,	\
		&&DEC,	\
		&&MOVR,	\
		&&MOVX,	\
		&&LT,	\
		&&BRH,	\
		&&BRE,	\
		&&IFE,	\
		&&IFNE,	\
		&&GT,	\
		&&GTE,	\
		&&LTE,	\
		&&MOVL,	\
        &&_NOP,	\
        &&_NOP,	\
		&&RMOV,	\
		&&MOV,	\
		&&MOVD,	\
		&&MOVBI,	\
		&&_SIZEOF,	\
		&&PUT,	\
		&&PUTC,	\
		&&CHECKLEN, \
        &&_NOP,	\
        &&_NOP,	\
        &&_NOP,	\
        &&_NOP,	\
		&&MOVN, \
        &&GOTO,	\
        &&MOVG,	\
        &&_NOP,	\
        &&LOADX, \
        &&NEWstr, \
        &&PUSHREF, \
        &&DELREF,   \
        &&INIT_FRAME, \
        &&CALL,         \
        &&NEW_CLASS,      \
        &&SMOV,            \
        &&SMOVR,            \
        &&SMOVOBJ,           \
        &&IADD,               \
        &&ISUB,                \
        &&IMUL,                 \
        &&IDIV,                  \
        &&IMOD,                   \
        &&_SLEEP,                   \
        &&TEST,                      \
        &&LOCK,                       \
        &&ULOCK,                       \
        &&EXP,                          \
        &&ADDL,                          \
        &&SUBL,                           \
        &&MULL,                            \
        &&DIVL,                             \
        &&MODL,                              \
        &&MOVSL,                              \
        &&DEL,                                 \
        &&MOVND,                                \
        &&SDELREF,                                \
        &&NEW_OBJ_ARRY,                           \
        &&NOT,                                      \
        &&SKP,                                       \
        &&LOADF,                                      \
        &&IADDL,                                       \
        &&ISUBL,                                        \
        &&IMULL,                                         \
        &&IDIVL,                                          \
        &&IMODL,                                           \
        &&POPR,                                             \
        &&SHL,                                               \
        &&SHR,                                                \
        &&TNE,                                                 \
        &&POPREF,                                               \
        &&MUTL,                                                  \
        &&SKPE,                                                   \
        &&SKPNE,                                                   \
        &&AND,                                                      \
        &&uAND,                                                      \
        &&OR,                                                         \
        &&uNOT,                                                        \
        &&ANDL,                                                        \
        &&ORL,                                                        \
        &&NOTL,                                                        \
        &&_THROW,                                                        \
        &&CHKNULL,                                                        \
        &&RETURNREF,                                                        \
        &&SDEL,                                                        \
    };

/*
 * All opcodes removed must be filled with _NOP instruction to prevent
 * invalid instruction dispatching
 */
enum OPCODE {
    op_NOP=0x0,
    op_INT=0x1,
    op_MOVI=0x2,
    op_RET=0x3,
    op_HLT=0x4,
    op_NEWi=0x5,
    op_CHECK_CAST=0x6,
    op_MOV8=0x7,
    op_MOV16=0x8,
    op_MOV32=0x9,
    op_MOV64=0xa,
    op_PUSHR=0xb,
    op_ADD=0xc,
    op_SUB=0xd,
    op_MUL=0xe,
    op_DIV=0xf,
    op_MOD=0x10,
    op_POP=0x11,
    op_INC=0x12,
    op_DEC=0x13,
    op_MOVR=0x14,
    op_MOVX=0x15,
    op_LT=0x16,
    op_BRH=0x17,
    op_BRE=0x18,
    op_IFE=0x19,
    op_IFNE=0x1a,
    op_GT=0x1b,
    op_GTE=0x1c,
    op_LTE=0x1d,
    op_MOVL=0x1e,
    op_RMOV=0x21,
    op_MOV=0x22,
    op_MOVD=0x23,
    op_MOVBI=0x24,
    op_SIZEOF=0x25,
    op_PUT=0x26,
    op_PUTC=0x27,
    op_CHECKLEN=0x28,

    op_MOVU8=0x29,
    op_MOVU16=0x2a,
    op_MOVU32=0x2b,
    op_MOVU64=0x2c,

    op_MOVN=0x2d, /* move ptr = &ptr->_Node[r]; */
	op_GOTO=0x2e,
    op_MOVG=0x2f, /* move ptr = &env->objects[r]; */
    op_LOADX=0x31,
    op_NEWSTR=0x32,
    op_PUSHREF=0x33,
    op_DELREF=0x34,
    op_INIT_FRAME=0x35,
    op_CALL=0x36,
    op_NEW_CLASS=0x37,
	op_SMOV=0x38,
	op_SMOVR=0x39,
    op_SMOVOBJ=0x3a,
    op_IADD=0x3b,
    op_ISUB=0x3c,
    op_IMUL=0x3d,
    op_IDIV=0x3e,
    op_IMOD=0x3f,
	op_SLEEP=0x40,
    op_TEST=0x41,
    op_LOCK=0x42,
    op_ULOCK=0x43,
    op_EXP=0x44,
    op_ADDL=0x45,
    op_SUBL=0x46,
    op_MULL=0x47,
    op_DIVL=0x48,
    op_MODL=0x49,
    op_MOVSL=0x4a,
    op_DEL=0x4b,
    op_MOVND=0x4c,
    op_SDELREF=0x4d,
    op_NEW_OBJ_ARRY=0x4e,
    op_NOT=0x4f,
    op_SKP=0x50,
    op_LOADF=0x51,
    op_IADDL=0x52,
    op_ISUBL=0x53,
    op_IMULL=0x54,
    op_IDIVL=0x55,
    op_IMODL=0x56,
    op_POPR=0x57,
    op_SHL=0x58,
    op_SHR=0x59,
    op_TNE=0x5a,
    op_POPREF=0x5b,
    op_MUTL=0x5c,
    op_SKPE=0x5d,
    op_SKPNE=0x5e,
    op_AND=0x5f,
    op_uAND=0x60,
    op_OR=0x61,
    op_uNOT=0x62,
    op_ANDL=0x63,
    op_ORL=0x64,
    op_NOTL=0x65,
    op_THROW=0x66,
    op_CHKNULL=0x67,
    op_RETURNREF=0x68,
    op_SDEL=0x69,

    MAX_OPCODE=0x59
};

#endif //SHARP_OPCODE_H
