#ifndef RIR_FUNCTION_H
#define RIR_FUNCTION_H

#include "BC.h"
#include <vector>

namespace rjit {
namespace rir {

class Function {
  public:
    std::vector<Code*> code;

    Function() {}

    void addCode(fun_idx_t pos, Code* c) {
        assert(pos < code.size());
        code[pos] = c;
    }

    fun_idx_t next() {
        code.push_back(nullptr);
        assert(code.size() < MAX_FUN_IDX);
        return code.size() - 1;
    }
};
}
}

#endif