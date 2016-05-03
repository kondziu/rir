#ifndef RJIT_RIR_BC_INC
#define RJIT_RIR_BC_INC

#include <cstdint>
#include <cstddef>

#include "RDefs.h"

namespace rjit {
namespace rir {

// ============================================================
// ==== Bytecode Layout:
//
// [ uint8_t bytecode | optional immediate argument ]
//
enum class BC_t : uint8_t {
    invalid,

    // Push a constant to the stack
    // I: constant (via Pool)
    // S: +1
    push,

    // Function lookup
    // I: symbol (via Pool)
    // S: +1
    getfun,

    // Variable lookup
    // I: symbol (via Pool)
    // S: +1
    getvar,

    // Call function
    // I: N - number of arguments
    // S: -N
    call,

    // Call function with named arguments
    // I: list of names (via Pool)
    // S: - #names
    call_name,

    // Create a promise
    // I: promise index
    // S: +1
    mkprom,

    // Create a closure
    // I: closure index
    // S: +1
    mkclosure,

    ret,
    force,
    drop,

    num_of
};

// ============================================================
// ==== immediate argument types
//
typedef uint32_t pool_idx_t;
typedef uint16_t fun_idx_t;
typedef uint16_t num_args_t;

union immediate_t {
    pool_idx_t pool;
    fun_idx_t fun;
    num_args_t numArgs;
};

static constexpr size_t MAX_NUM_ARGS = 1L << (8 * sizeof(num_args_t));
static constexpr size_t MAX_FUN_IDX = 1L << (8 * sizeof(fun_idx_t));
static constexpr size_t MAX_POOL_IDX = 1L << (8 * sizeof(pool_idx_t));

// ============================================================
// ==== Creation and decoding of Bytecodes
//
// * Static factory functions to create an instance of a bytecode
//   which can be pushed onto a CodeStream
// * read and advance to read the next bytecode from an array
//
class CodeStream;
class BC {
    BC(BC_t bc) : bc(bc), immediate({0}) {}
    BC(BC_t bc, immediate_t immediate) : bc(bc), immediate(immediate) {}

  public:
    const BC_t bc;
    const immediate_t immediate;

    inline size_t size() const;
    void write(CodeStream& cs) const;

    // Getters for the immediate arguments
    SEXP immediateConst();
    inline fun_idx_t immediateFunIdx() { return immediate.fun; }
    inline fun_idx_t immediateNumArgs() { return immediate.numArgs; }

    // Decode BC from bytecode stream
    inline const static BC read(BC_t* pc);
    inline const static BC advance(BC_t** pc);

    // Create a new BC instance
    inline const static BC call(num_args_t numArgs);
    inline const static BC call_name(SEXP names);
    inline const static BC push(SEXP constant);
    inline const static BC getfun(SEXP sym);
    inline const static BC getvar(SEXP sym);
    inline const static BC mkprom(fun_idx_t prom);
    inline const static BC ret();
    inline const static BC drop();
    inline const static BC force();
};

} // rir
} // rjit

#endif