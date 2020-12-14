/*!
 *  Copyright (c) 2019 by Contributors
 * \file shared_mem.cc
 * \brief Shared memory management.
 */
#ifndef _WIN32
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <dmlc/logging.h>
#include <dgl/runtime/shared_mem.h>

#include "resource_manager.h"

namespace dgl {
namespace runtime {

#ifndef _WIN32
/*
 * Shared memory is a resource that cannot be cleaned up if the process doesn't
 * exit normally. We'll manage the resource with ResourceManager.
 */
class SharedMemoryResource: public Resource {
  std::string name;

 public:
  explicit SharedMemoryResource(const std::string &name) {
    this->name = name;
  }

  void Destroy() {
    LOG(INFO) << "remove " << name << " for shared memory";
    shm_unlink(name.c_str());
  }
};
#endif  // _WIN32

SharedMemory::SharedMemory(const std::string &name) {
#ifndef _WIN32
  this->name = name;
  this->own = false;
  this->fd = -1;
  this->ptr = nullptr;
  this->size = 0;
#else
  LOG(FATAL) << "Shared memory is not supported on Windows.";
#endif  // _WIN32
}

SharedMemory::~SharedMemory() {
#ifndef _WIN32
  munmap(ptr, size);
  close(fd);
  if (own) {
    LOG(INFO) << "remove " << name << " for shared memory";
    shm_unlink(name.c_str());
    // The resource has been deleted. We don't need to keep track of it any more.
    DeleteResource(name);
  }
#else
  LOG(FATAL) << "Shared memory is not supported on Windows.";
#endif  // _WIN32
}

void *SharedMemory::CreateNew(size_t size) {
#ifndef _WIN32
  this->own = true;

  int flag = O_RDWR|O_CREAT;
  fd = shm_open(name.c_str(), flag, S_IRUSR | S_IWUSR);
  CHECK_NE(fd, -1) << "fail to open " << name << ": " << strerror(errno);
  // Shared memory cannot be deleted if the process exits abnormally.
  AddResource(name, std::shared_ptr<Resource>(new SharedMemoryResource(name)));
  auto res = ftruncate(fd, size);
  CHECK_NE(res, -1)
      << "Failed to truncate the file. " << strerror(errno);
  ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  CHECK_NE(ptr, MAP_FAILED)
      << "Failed to map shared memory. mmap failed with error " << strerror(errno);
  return ptr;
#else
  LOG(FATAL) << "Shared memory is not supported on Windows.";
#endif  // _WIN32
}

void *SharedMemory::Open(size_t size) {
#ifndef _WIN32
  int flag = O_RDWR;
  fd = shm_open(name.c_str(), flag, S_IRUSR | S_IWUSR);
  CHECK_NE(fd, -1) << "fail to open " << name << ": " << strerror(errno);
  ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  CHECK_NE(ptr, MAP_FAILED)
      << "Failed to map shared memory. mmap failed with error " << strerror(errno);
  return ptr;
#else
  LOG(FATAL) << "Shared memory is not supported on Windows.";
#endif  // _WIN32
}

bool SharedMemory::Exist(const std::string &name) {
#ifndef _WIN32
  int fd = shm_open(name.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
  if (fd >= 0) {
    close(fd);
    return true;
  } else {
    return false;
  }
#else
  LOG(FATAL) << "Shared memory is not supported on Windows.";
#endif  // _WIN32
}

}  // namespace runtime
}  // namespace dgl
