/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

/**
 * \file
 * 
 * \brief Functions for printing FuzzTrace objects as C++ or ICNF
 */

#pragma once

#include <libincmonk/FuzzTrace.h>

#include <ostream>
#include <string>

namespace incmonk {
/**
 * \brief Translates the given trace to a corresponding C++11 function body
 *   
 * No `ipasir_init` or `ipasir_destroy` calls are generated.
 * 
 * Usage example: generate regression tests for error traces.
 * 
 * \param first       The trace [first, last) to be translated
 * \param last
 * \param solverName  The variable name of the IPASIR solver pointer (ie. the `void*` argument)
 * 
 * \returns           A C++11 function body containing IPASIR calls corresponding to the trace
 */
auto toCxxFunctionBody(FuzzTrace::const_iterator first,
                       FuzzTrace::const_iterator last,
                       std::string const& solverName) -> std::string;


/**
 * \brief Translates the given trace to a corresponding ICNF instance
 * 
 * \param first       The trace [first, last) to be translated
 * \param last
 * 
 * \returns           An ICNF instance corresponding to the trace
 */
void toICNF(FuzzTrace::const_iterator first,
            FuzzTrace::const_iterator last,
            std::ostream& targetStream);
}