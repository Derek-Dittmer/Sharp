//
// Created by bknun on 4/22/2017.
//

#include "Asm.h"
#include "parser/tokenizer/tokenizer.h"
#include "parser/parseerrors.h"
#include "parser/tokenizer/tokenizer.h"
#include "runtime.h"
#include "../runtime/interp/Opcode.h"
#include "../runtime/interp/register.h"

bool Asm::isend() {
    return npos >= tk->getentities().size() || current().gettokentype() == _EOF;
}

bool Asm::instruction_is(string name) {
    if(current().getid() == IDENTIFIER && current().gettoken() == name) {
        npos++;
        expect_instr = false;
        return true;
    }

    return false;
}

string Asm::expect_identifier() {
    if(current().getid() == IDENTIFIER) {
        string name = current().gettoken();
        npos++;
        return name;
    } else {
        tk->geterrors()->newerror(GENERIC, current(), "expected identifier");
        return "";
    }
}

bool Asm::label_exists(string label) {
    for(unsigned int i = 0; i < label_map->size(); i++) {
        if(label_map->get(i).key == label)
            return true;
    }

     return false;
}

int64_t Asm::current_address() {
    return assembler->__asm64.size();
}

int64_t Asm::get_label(string label) {
    for(unsigned int i = 0; i < label_map->size(); i++) {
        if(label_map->get(i).key == label)
            return label_map->get(i).value;
    }

    return 0;
}

void Asm::expect_register() {
    if(current() == "%")
        npos++;

    if(current() == "adx") {
        i2.high_bytes = adx;
    } else if(current() == "cx") {
        i2.high_bytes = cx;
    } else if(current() == "cmt") {
        i2.high_bytes = cmt;
    } else if(current() == "ebx") {
        i2.high_bytes = ebx;
    } else if(current() == "ecx") {
        i2.high_bytes = ecx;
    } else if(current() == "ecf") {
        i2.high_bytes = ecf;
    } else if(current() == "edf") {
        i2.high_bytes = edf;
    } else if(current() == "ehf") {
        i2.high_bytes = ehf;
    } else if(current() == "bmr") {
        i2.high_bytes = bmr;
    } else if(current() == "egx") {
        i2.high_bytes = egx;
    } else if(current() == "fp") {
        i2.high_bytes = fp;
    } else if(current() == "sp") {
        i2.high_bytes = sp;
    } else {
        // error
        tk->geterrors()->newerror(GENERIC, current(), "symbol `" + current().gettoken() + "` is not a register");
    }

    npos++;
    i2.low_bytes = -1;
}

int Asm::get_offset() {
    if(current() == "+") {
        npos++;
        expect_int();

        return i2.high_bytes;
    } else if(current() == "-") {
        npos++;
        expect_int();

        return -i2.high_bytes;
    } else {
        return 0;
    }
}

bool Asm::hex_int(string s) {
    return s.size() > 2 && s.at(0) == '0' && s.at(1) == 'x';
}

void Asm::expect_int() {
    bool hash = false;
    if(current() == "#" && current().getid() != CHAR_LITERAL) {
        hash = true;
        npos++;
    }

    if(current().getid() == INTEGER_LITERAL || current().getid() == HEX_LITERAL) {
        double x;
        string int_string = runtime::invalidate_underscores(current().gettoken());

        if(runtime::all_integers(int_string) || hex_int(int_string)) {
            x = std::strtol (int_string.c_str(), NULL, 0);
            if(x > DA_MAX || x < DA_MIN) {
                stringstream ss;
                ss << "integral number too large: " + int_string;
                tk->geterrors()->newerror(GENERIC, current(), ss.str());
            }
            i2.high_bytes = (int64_t )x;
            i2.low_bytes = -1;
        }else {
            x = std::strtod (int_string.c_str(), NULL);
            if((int64_t )x > DA_MAX || (int64_t )x < DA_MIN) {
                stringstream ss;
                ss << "integral number too large: " + int_string;
                tk->geterrors()->newerror(GENERIC, current(), ss.str());
            }

            i2.high_bytes = (int64_t )x;

            if(current().gettoken().find('.') != string::npos || current().gettoken().find('e') != string::npos
               || current().gettoken().find('E') != string::npos)
                i2.low_bytes = abs(runtime::get_low_bytes(x));
            else
                i2.low_bytes = -1;
        }
    } else if(current().getid() == CHAR_LITERAL) {
        if(hash)
            tk->geterrors()->newerror(GENERIC, current(), "invalid format, symbol '#' requires integer literal following it");

        if(current().gettoken().size() > 1) {
            switch(current().gettoken().at(1)) {
                case 'n':
                    i2.high_bytes = '\n';
                    break;
                case 't':
                    i2.high_bytes = '\t';
                    break;
                case 'b':
                    i2.high_bytes = '\b';
                    break;
                case 'v':
                    i2.high_bytes = '\v';
                    break;
                case 'r':
                    i2.high_bytes = '\r';
                    break;
                case 'f':
                    i2.high_bytes = '\f';
                    break;
                case '\\':
                    i2.high_bytes = '\\';
                    break;
                default:
                    i2.high_bytes = current().gettoken().at(1);
                    break;
            }
            i2.low_bytes = -1;
        } else {
            i2.high_bytes = current().gettoken().at(0);
            i2.low_bytes = -1;
        }
    } else if(current().gettoken() == "true" || current().gettoken() == "false") {
        if(hash)
            tk->geterrors()->newerror(GENERIC, current(), "invalid format, symbol '#' requires integer literal following it");

        if(current().gettoken() == "true") {
            i2.high_bytes = 1;
            i2.low_bytes = -1;
        } else {
            i2.high_bytes = 0;
            i2.low_bytes = -1;
        }
    } else {
        // error
        i2.high_bytes = 0;
        i2.low_bytes = -1;
        tk->geterrors()->newerror(GENERIC, current(), "expected integer literal");
    }

    npos++;
}

void Asm::expect(string token) {
    if(current() == token) {
        npos++;
    } else {
        tk->geterrors()->newerror(GENERIC, current(), "expected `" + token + "`");
    }
}

List<string> Asm::parse_modulename() {
    List<string> name;
    name.add(current().gettoken());

    npos++;
    while(current().gettokentype() == DOT) {
        name.add(current().gettoken());
        npos++;

        name.add(expect_identifier());
    }

    return name;
}

extern _operator string_toop(string op);

Method* Asm::getScopedMethod(ClassObject* klass, string method, int64_t _offset, long line, long col) {

    Method* func;

    if((func = klass->getFunction(method, _offset))) {
        if(klass->getMacros(method, _offset)) {
            tk->geterrors()->newerror(GENERIC, line, col, "call to function method `" + method + "` is ambiguous");
        }
        return func;
    } else if((func = klass->getMacros(method, _offset))) {
        return func;
    } else if((func = klass->getOverload(string_toop(method), _offset))) {
        return func;
    } else if(method == klass->getName()) {
        if(_offset < klass->constructorCount()) {
            return klass->getConstructor(_offset);
        }
    }

    return NULL;
}

void Asm::removeDots(List<string>& lst) {
    List<string> slst;
    for(unsigned int i = 0; i < lst.size(); i++) {
        if(lst.at(i) == ".") {
            continue;
        } else {
            slst.push_back(lst.get(i));
        }
    }

    lst.addAll(slst);
}

void Asm::removeDots(List<nString>& lst) {
    readjust:
    for(unsigned int i = 0; i < lst.size(); i++) {
        if(lst.at(i) == ".") {
            lst.get(i).free();
            lst.remove(i);
            goto readjust;
        }
    }
}

void Asm::expect_function() {
    if(!(current().getid() == IDENTIFIER && !parser::iskeyword(current().gettoken()))) {
        tk->geterrors()->newerror(GENERIC, current(), "expected identifier");
        return;
    }

    List<string> module = parse_modulename();
    List<string> function;

    if(current().gettokentype() == HASH) {
        npos++;

        function.add(expect_identifier());

        while(current().gettokentype() == DOT ) {
            npos++;
            function.add(expect_identifier());
        }
    }

    int64_t offset = 0;
    if(current() == "+") {
        npos++;
        expect_int();

        offset = i2.high_bytes;
    }

    string module_name = "";
    if(function.size() > 0) {
        removeDots(function);

        for(unsigned int i = 0; i < module.size(); i++) {
            module_name += module.at(i);
        }

        module.addAll(function);
    } else {
        removeDots(module);
    }

    Method* method;
    if(module_name == "" && module.size() == 1) {
        if((method = instance->getmacros(module_name, module.at(0), offset)) != NULL){
            i2.high_bytes = method->vaddr;
        } else {

            string mname = module.at(0);
            if(instance->current_scope()->klass != NULL) {
                method = getScopedMethod(instance->current_scope()->klass, mname, offset, current().getline(), current().getcolumn());

                if(method != NULL) {
                    i2.high_bytes = method->vaddr;
                } else {
                    tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
                    return;
                }
            } else {
                tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
                return;
            }
        }
    } else {
        ClassObject* klass = instance->getClass(module_name, module.get(0));

        if(klass != NULL) {
            for(unsigned int i = 1; i < module.size() - 1; i++) {
                if((klass = klass->getChildClass(module.get(i))) == NULL) {
                    tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + module.get(i) + "`");
                    return;
                }
            }

            string mname = module.at(module.size()-1);
            method = getScopedMethod(klass, mname, offset, current().getline(), current().getcolumn());

            if(method != NULL) {
                i2.high_bytes = method->vaddr;
            } else {
                tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
                return;
            }
        } else {
            tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + module.get(0) + "`");
            return;
        }
    }
}

void Asm::expect_class() {
    if(!(current().getid() == IDENTIFIER && !parser::iskeyword(current().gettoken()))) {
        tk->geterrors()->newerror(GENERIC, current(), "expected identifier");
        return;
    }

    List<string> module = parse_modulename();
    List<string> klassHeiarchy;

    if(current().gettokentype() == HASH) {
        npos++;

        klassHeiarchy.add(expect_identifier());

        while(current().gettokentype() == DOT ) {
            npos++;
            klassHeiarchy.add(expect_identifier());
        }
    }

    string module_name = "";
    if(klassHeiarchy.size() > 0) {
        removeDots(klassHeiarchy);

        for(unsigned int i = 0; i < module.size(); i++) {
            module_name += module.at(i);
        }

        module.addAll(klassHeiarchy);
    } else {
        removeDots(module);
    }

    ClassObject* klass;
    if(module_name == "" && module.size() == 1) {
        if((klass = instance->getClass(module_name, module.get(0))) != NULL){
            i2.high_bytes = klass->vaddr;
        } else {

            string kname = module.at(0);
            if(instance->current_scope()->klass != NULL) {
                klass = instance->current_scope()->klass->getChildClass(kname);

                if(klass != NULL) {
                    i2.high_bytes = klass->vaddr;
                } else {
                    tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + kname + "`");
                    return;
                }
            } else {
                tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + kname + "`");
                return;
            }
        }
    } else {
        ClassObject* klass = instance->getClass(module_name, module.at(0));

        if(klass != NULL) {
            for(unsigned int i = 1; i < module.size(); i++) {
                if((klass = klass->getChildClass(module.at(i))) == NULL) {
                    tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + module.at(i) + "`");
                    return;
                }
            }

            if(klass != NULL) {
                i2.high_bytes = klass->vaddr;
            } else {
                tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + module.at(0) + "`");
                return;
            }
        } else {
            tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + module.at(0) + "`");
            return;
        }
    }
}

void Asm::parse(m64Assembler &assembler, runtime *instance, string& code, ast* pAst) {
    if(code == "") return;

    this->assembler = &assembler;
    this->instance = instance;
    this->code = code;
    tk = new tokenizer(code, "stdin");
    label_map = &instance->current_scope()->label_map;
    RuntimeNote note = RuntimeNote(instance->_current->sourcefile, instance->_current->geterrors()->getline(pAst->line),
                                   pAst->line, pAst->col);
    keypair<std::string, int64_t> label;

    // TODO: add smov & smovr instructions
    if(tk->geterrors()->_errs())
    {
        cout << note.getNote("Assembler messages:");
        tk->geterrors()->print_errors();

        errors+= tk->geterrors()->error_count();
        uo_errors+= tk->geterrors()->uoerror_count();
    } else {
        /* parse assembly */
        int2_t itmp;

        while(!isend())
        {
            if(instruction_is("nop")) {
                assembler.push_i64(SET_Ei(i64, op_NOP));
            } else if(instruction_is("int")) {
                expect_int();
                assembler.push_i64(SET_Di(i64, op_INT, i2.high_bytes));
            } else if(instruction_is("movi")) {
                if(current() == "$") {
                    int64_t _offset=0, _register=0, address=-1;
                    string name;
                    expect("$");

                    if(current() == "$") {
                        npos++;
                        address = assembler.__asm64.size() == 0 ? 0 : assembler.__asm64.size()-1;
                    } else {
                        name = expect_identifier();
                    }


                    if(current() == "+") {
                        npos++;
                        expect_int();
                        _offset = i2.high_bytes;
                    }

                    expect(",");
                    expect_register();
                    _register=i2.high_bytes;

                    if(address!=-1) {
                        assembler.push_i64(SET_Di(i64, op_MOVI, (address+_offset)), _register);
                    } else {
                        instance->current_scope()->addStore(name, _register, _offset,
                                                            assembler, pAst->line, pAst->col);
                    }
                } else {
                    expect_int();
                    itmp = i2;
                    expect(",");
                    expect_register();
                    assembler.push_i64(SET_Di(i64, op_MOVI, itmp.high_bytes), i2.high_bytes);
                }
            } else if(instruction_is("ret")) {
                assembler.push_i64(SET_Ei(i64, op_RET));
            } else if(instruction_is("hlt")) {
                assembler.push_i64(SET_Ei(i64, op_HLT));
            } else if(instruction_is("new_i")) {
                expect_register();
                assembler.push_i64(SET_Di(i64, op_NEWi, i2.high_bytes));
            } else if(instruction_is("check_cast")) {
                assembler.push_i64(SET_Ei(i64, op_CHECK_CAST));
            } else if(instruction_is("mov8")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV8, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov16")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV16, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov32")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV32, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov64")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV64, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("pushr")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_PUSHR, i2.high_bytes));
            } else if(instruction_is("add")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_ADD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("sub")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SUB, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("mul")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MUL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("div")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_DIV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("mod")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("pop")) {
                assembler.push_i64(SET_Ei(i64, op_POP));
            } else if(instruction_is("inc")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_INC, i2.high_bytes));
            } else if(instruction_is("dec")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_DEC, i2.high_bytes));
            } else if(instruction_is("movr")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVR, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movx")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVX, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("lt")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_LT, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("brh")) {
                assembler.push_i64(SET_Ei(i64, op_BRH));
            } else if(instruction_is("bre")) {
                assembler.push_i64(SET_Ei(i64, op_BRE));
            } else if(instruction_is("ife")) {
                assembler.push_i64(SET_Ei(i64, op_IFE));
            } else if(instruction_is("ifne")) {
                assembler.push_i64(SET_Ei(i64, op_IFNE));
            } else if(instruction_is("gt")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_GT, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("ge")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_GTE, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("le")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_LTE, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if((i2.high_bytes = instance->current_scope()->getLocalFieldIndex(local)) == -1)  {
                        tk->geterrors()->newerror(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }

                    if(!instance->current_scope()->function->isStatic()) {
                        i2.high_bytes++; // increment by 1 for self
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_MOVL, i2.high_bytes));
            } else if(instruction_is("rmov")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_RMOV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_MOV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movd")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_MOVD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movbi")) {
                expect_int();

                if(i2.low_bytes != -1) {
                    assembler.push_i64(SET_Di(i64, op_MOVBI, i2.high_bytes), i2.low_bytes);
                } else {
                    itmp = i2;
                    expect(",");
                    expect_int();

                    assembler.push_i64(SET_Di(i64, op_MOVBI, itmp.high_bytes), i2.high_bytes);
                }

            } else if(instruction_is("_sizeof")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_SIZEOF, i2.high_bytes));
            } else if(instruction_is("put")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_PUT, i2.high_bytes));
            } else if(instruction_is("_putc")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_PUTC, i2.high_bytes));
            } else if(instruction_is("chklen")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_CHECKLEN, i2.high_bytes));
            } else if(current() == ".") {
                npos++;

                string name = expect_identifier();
                if(!label_exists(name)) {
                    label.set(name, current_address());
                    label_map->add(label);
                } else {
                    npos--;
                    tk->geterrors()->newerror(GENERIC, current(), "redefinition of label `" + name + "`");
                    npos++;
                }

                expect_instr = true;
                expect(":");
            } else if(instruction_is("goto")) {
                expect("$");
                string name = expect_identifier();
                int64_t _offset=0;

                if(current() == "+") {
                    npos++;

                    int64_t adx = get_label(name);
                    expect_int();

                    _offset = i2.high_bytes;
                    i2.high_bytes=adx;
                } else {
                    i2.high_bytes=get_label(name);
                }

                if(label_exists(name)) {
                    assembler.push_i64(SET_Di(i64, op_GOTO, (i2.high_bytes+_offset)));
                } else {
                    instance->current_scope()->addBranch(name, _offset, assembler, pAst->line, pAst->col);
                }
            } else if(instruction_is("pushref")) {
                assembler.push_i64(SET_Ei(i64, op_PUSHREF));
            } else if(instruction_is("del_ref")) {
                assembler.push_i64(SET_Ei(i64, op_DELREF));
            } else if(instruction_is("iframe")) {
                assembler.push_i64(SET_Ei(i64, op_INIT_FRAME));
            } else if(instruction_is("call")) {
                expect("<");
                expect_function();
                expect(">");
                assembler.push_i64(SET_Di(i64, op_CALL, i2.high_bytes));
            } else if(instruction_is("new_class")) {
                expect("<");
                expect_class();
                expect(">");
                assembler.push_i64(SET_Di(i64, op_NEW_CLASS, i2.high_bytes));
            } else if(instruction_is("movn")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_MOVN, i2.high_bytes));
            }  else if(instruction_is("smov")) {
                i2.low_bytes = 0;
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SMOV, abs(i2.high_bytes), (i2.high_bytes<0), get_offset()));
            } else if(instruction_is("smovr")) {
                i2.low_bytes = 0;
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SMOVR, abs(i2.high_bytes), (i2.high_bytes<0), get_offset()));
            } else if(instruction_is("smovobj")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_SMOVOBJ, i2.high_bytes));
            }else if(instruction_is("iadd")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IADD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("isub")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_ISUB, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imul")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IMUL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("idiv")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IDIV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imod")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IMOD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("slp")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_SLEEP, i2.high_bytes));
            } else if(instruction_is("test")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_TEST, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("_lck")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_LOCK, i2.high_bytes));
            } else if(instruction_is("ulck")) {
                assembler.push_i64(SET_Ei(i64, op_ULOCK));
            } else if(instruction_is("exp")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_EXP, i2.high_bytes));
            } else {
                npos++;
                tk->geterrors()->newerror(GENERIC, current(), "expected instruction");
            }
        }

        if(expect_instr) {
            tk->geterrors()->newerror(GENERIC, current(), "expected instruction");
        }

        if(tk->geterrors()->_errs())
        {

            cout << note.getNote("Assembler messages:");
            tk->geterrors()->print_errors();

            errors+= tk->geterrors()->error_count();
            uo_errors+= tk->geterrors()->uoerror_count();
        }
    }

    tk->free();
    this->code = "";
}

void Asm::check_CB() {
    if(i2.high_bytes > CA_MAX || i2.high_bytes < CA_MIN) {
                    stringstream ss;
                    ss << "integral number too large: " << i2.high_bytes;
                    tk->geterrors()->newerror(GENERIC, current(), ss.str());
                }
}

token_entity Asm::current() {
    return tk->getentities().get(npos);
}

string Asm::registrerToString(int64_t r) {
    switch(r) {
        case adx:
            return "adx";
        case cx:
            return "cx";
        case cmt:
            return "cmt";
        case ebx:
            return "ebx";
        case ecx:
            return "ecx";
        case ecf:
            return "ecf";
        case edf:
            return "edf";
        case ehf:
            return "ehf";
        case bmr:
            return "bmr";
        case egx:
            return "egx";
        case sp:
            return "sp";
        case fp:
            return "fp";
        default:
            return "?";
    }
}
