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


#include <libincmonk/Fork.h>

#include <array>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

namespace incmonk {
namespace {

class Pipe {
public:
  Pipe()
  {
    if (pipe(m_pipeFd.data()) < 0) {
      perror("pipe");
      throw std::runtime_error{"Child process creation failed"};
    }
  }

  ~Pipe()
  {
    close(m_pipeFd[0]);
    close(m_pipeFd[1]);
  }

  int getReadFd() const noexcept { return m_pipeFd[0]; }
  int getWriteFd() const noexcept { return m_pipeFd[1]; }

  bool hasData() const
  {
    pollfd fd{getReadFd(), /* events */ POLLIN, /* revents */ 0};
    int const timeoutMillis = 10;
    int numReadyFDs = poll(&fd, 1, timeoutMillis);
    if (numReadyFDs < 0) {
      std::cout << "poll failure" << std::endl;
      throw std::runtime_error{"Child process communication failed"};
    }
    return numReadyFDs == 1;
  }

private:
  std::array<int, 2> m_pipeFd = {0, 0};
};

}

auto syncExecInFork(std::function<uint64_t()> const& fn, int childExitVal) -> uint64_t
{
  Pipe comm;
  pid_t childPid = fork();

  if (childPid == 0) {
    uint64_t result = 0;
    try {
      result = fn();
    }
    catch (...) {
      exit(EXIT_FAILURE);
    }
    write(comm.getWriteFd(), &result, sizeof(uint64_t));
    exit(childExitVal);
  }
  else if (childPid > 0) {
    // Wait for the child process to finish
    while (true) {
      int statloc = 0;
      pid_t waitResult = waitpid(childPid, &statloc, 0);
      assert(errno != EINVAL);

      if (waitResult != -1) {
        if (WIFEXITED(statloc) == 0) {
          throw ChildExecutionFailure{};
        }
        break;
      }

      if (errno == ECHILD) {
        // the child exited before the waitpid call
        break;
      }
      assert(errno == EINTR);
      // waitpid has been interrupted by a signal, wait some more
    }

    // The child is dead by now. Fetch the result from the pipe:
    if (!comm.hasData()) {
      // The child process didn't get around to write to the pipe
      throw ChildExecutionFailure{};
    }

    uint64_t result = 0;
    std::size_t numBytesRead = read(comm.getReadFd(), &result, sizeof(uint64_t));
    if (numBytesRead != sizeof(uint64_t)) {
      throw ChildExecutionFailure{};
    }
    return result;
  }
  else {
    throw std::runtime_error{"Child process creation failed"};
  }
}
}