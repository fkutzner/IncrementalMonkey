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
 * \brief Functions for creating child processes and obtaining feedback
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>

namespace incmonk {
class ChildExecutionFailure {
};

/**
 * \brief Synchronously executes the given function in a child process.
 * 
 * \param fn    A function that will be invoked in the child process.
 * \param childExitVal  The exit() value returned from the child process
 *   on successful termination.
 * \param timeout An optional timeout. If the child process did not complete
 *   within this time limit, it is killed and nothing is returned.
 * 
 * \returns The return value of `fn`'s execution in the child process. 
 * 
 * This function can is used to execute the system under test without
 * risking crashes the fuzzer process.
 * 
 * \throws ChildExecutionFailure when the fork()ed child process
 * exits during the execution of `fn`. If an exception is thrown from
 * `fn`, this exception is also raised.
 * 
 * \throws std::runtime_error when the child process setup failed.
 */
auto syncExecInFork(std::function<uint64_t()> const& fn,
                    int childExitVal,
                    std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    -> std::optional<uint64_t>;

}
