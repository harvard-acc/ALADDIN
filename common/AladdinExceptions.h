#ifndef _ALADDIN_EXCEPTIONS_H_
#define _ALADDIN_EXCEPTIONS_H_

#include <stdexcept>
#include <string>

#include "typedefs.h"

class ExecNode;

class AladdinException : public std::runtime_error {
 public:
  AladdinException(const std::string& message);
  AladdinException(const ExecNode* node, const std::string& message);
};

class VirtualAddrLookupException : public AladdinException {
 public:
  VirtualAddrLookupException(const std::string& array_name);

 protected:
  static const std::string helpfulSuggestion;
};

class UnknownArrayException : public AladdinException {
 public:
  UnknownArrayException(const std::string& array_name);
  UnknownArrayException(Addr array_addr);

 protected:
  static const std::string helpfulSuggestion;
};

class IllegalHostMemoryAccessException : public AladdinException {
 public:
  IllegalHostMemoryAccessException(const std::string& array_name);
  IllegalHostMemoryAccessException(const ExecNode* node);

 protected:
  static const std::string helpfulSuggestion;
};

class ArrayAccessException : public AladdinException {
 public:
  ArrayAccessException(const std::string& message);
};

class AddressTranslationException : public AladdinException {
 public:
  AddressTranslationException(Addr vaddr, unsigned size);
};

#endif
