#include "Compiler.h"

#include "BC.h"
#include "CodeStream.h"

#include "../RIntlns.h"
#include "../RList.h"
#include "../Sexp.h"
#include "../Symbols.h"

#include "OldInterpreter.h"
#include "Optimizer.h"
#include "Pool.h"

#include "CodeVerifier.h"

namespace rjit {
namespace rir {

namespace {

fun_idx_t compilePromise(FunctionHandle& f, SEXP exp);
void compileExpr(FunctionHandle& f, CodeStream& cs, SEXP exp);

// function application
void compileCall(FunctionHandle& parent, CodeStream& cs, SEXP ast, SEXP fun,
                 SEXP args) {
    // application has the form:
    // LHS ( ARGS )

    // LHS can either be an identifier or an expression
    Match(fun) {
        Case(SYMSXP) { cs << BC::ldfun(fun); }
        Else({
            compileExpr(parent, cs, fun);
            cs << BC::isfun();
        });
    }

    // Process arguments:
    // Arguments can be optionally named
    std::vector<fun_idx_t> callArgs;
    std::vector<SEXP> names;

    for (auto arg = RList(args).begin(); arg != RList::end(); ++arg) {
        // (1) Arguments are wrapped as Promises:
        //     create a new Code object for the promise
        size_t prom = compilePromise(parent, *arg);
        callArgs.push_back(prom);

        // (2) remember if the argument had a name associated
        if (*arg == R_DotsSymbol){
            names.push_back(R_DotsSymbol);
        } else {
            names.push_back(arg.hasTag() ? arg.tag() : R_NilValue);
        }
    }
    assert(callArgs.size() < MAX_NUM_ARGS);

    cs << BC::call(callArgs, names);

    cs.addAst(ast);
}

// Lookup
void compileGetvar(CodeStream& cs, SEXP name) {
    if (DDVAL(name)) {
        cs << BC::ldddvar(name);
    } else if (name == R_MissingArg) {
        cs << BC::push(R_MissingArg);
    } else {
        cs << BC::ldvar(name);
    }
}

// Constant
void compileConst(CodeStream& cs, SEXP constant) {
    SET_NAMED(constant, 2);
    cs << BC::push(constant);
}

void compileExpr(FunctionHandle& function, CodeStream& cs, SEXP exp) {
    // Dispatch on the current type of AST node
    Match(exp) {
        // Function application
        Case(LANGSXP, fun, args) { compileCall(function, cs, exp, fun, args); }
        // Variable lookup
        Case(SYMSXP) { compileGetvar(cs, exp); }
        // This happens in complex assignments, see eval.c::applydefine
        // (hint: rhsprom)
        Case(PROMSXP, value, expr, env) {
            assert(env == R_NilValue);
            assert(value != R_UnboundValue);
            compileConst(cs, value);
        }
        Case(BCODESXP) {
            assert(false);
        }
        // TODO : some code (eg. serialize.c:2154) puts closures into asts...
        //        not sure how we want to handle it...
        // Case(CLOSXP) {
        //     assert(false);
        // }

        // Constant
        Else(compileConst(cs, exp));
    }
}

std::vector<fun_idx_t> compileFormals(FunctionHandle& fun, SEXP formals) {
    std::vector<fun_idx_t> res;

    for (auto arg = RList(formals).begin(); arg != RList::end(); ++arg) {
        if (*arg != R_MissingArg)
            res.push_back(compilePromise(fun, *arg));
        else
            res.push_back(MISSING_ARG_OFFSET);
    }

    return res;
}

fun_idx_t compilePromise(FunctionHandle& function, SEXP exp) {
    CodeStream cs(function, exp);
    compileExpr(function, cs, exp);
    cs << BC::ret();
    return cs.finalize();
}
}

Compiler::CompilerRes Compiler::finalize() {
    // Rprintf("****************************************************\n");
    // Rprintf("Compiling function\n");
    FunctionHandle function = FunctionHandle::create();

    auto formProm = compileFormals(function, formals);

    CodeStream cs(function, exp);
    compileExpr(function, cs, exp);
    cs << BC::ret();
    cs.finalize();

    CodeVerifier::vefifyFunctionLayout(function.store, globalContext());
    FunctionHandle opt = Optimizer::optimize(function);
    CodeVerifier::vefifyFunctionLayout(opt.store, globalContext());

    Protect p;
    SEXP formout = R_NilValue;
    SEXP f = formout;
    SEXP formin = formals;
    for (auto prom : formProm) {
        SEXP arg = (prom == MISSING_ARG_OFFSET) ? 
            R_MissingArg : (SEXP)opt.codeAtOffset(prom);
        SEXP next = CONS_NR(arg, R_NilValue);
        SET_TAG(next, TAG(formin));
        formin = CDR(formin);
        if (formout == R_NilValue) {
            formout = f = next;
            p(formout);
        } else {
            SETCDR(f, next);
            f = next;
        }
    }

    // TODO compiling the formals is broken, since the optimizer drops the
    // formals code from the function object since they are not referenced!
    // 
    return {opt.store, formals /* formout */ };
}
}
}
