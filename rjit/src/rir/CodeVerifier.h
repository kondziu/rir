#ifndef CODE_VERIFIER_H
#define CODE_VERIFIER_H

#include "interp.h"

namespace rjit {
namespace rir {

/** Various verifications of the ::Code and ::Function objects.
 */
class CodeVerifier {
  public:
    /** Verifies the stack layout of the Code object and updates its ostack and
     * istack requirements.
     */
    static void calculateAndVerifyStack(::Code* code);

    /** Verifies that the given function object is valid.
     */
    static void vefifyFunctionLayout(SEXP sexp, ::Context* ctx);
};

} // namespace rir
} // namespace rjit

#endif // STACK_VERIFIER_H