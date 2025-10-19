// include/design_patterns/memento/snapio.hpp
#pragma once
#include <memory>


// The adapter to be able to support differnt container
struct SnapIO {
  virtual void write(const void* p, size_t n) = 0;
  virtual void read(void* p, size_t n) = 0;
  virtual size_t tell() const = 0;
  virtual void seek(size_t pos) = 0;
  virtual size_t size() const = 0;
  virtual std::unique_ptr<SnapIO> clone() const = 0;
  virtual ~SnapIO() = default;
};
