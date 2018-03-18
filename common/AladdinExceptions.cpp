#include "AladdinExceptions.h"
#include "ExecNode.h"

AladdinException::AladdinException(const std::string& message)
    : std::runtime_error(message) {}

AladdinException::AladdinException(const ExecNode* node,
                                   const std::string& message)
    : std::runtime_error(std::string("At node ") +
                         std::to_string(node->get_node_id()) + ": " + message) {
}

const std::string VirtualAddrLookupException::helpfulSuggestion =
    "Please ensure that you have called mapArrayToAccelerator() with "
    "the correct array name parameter.";

VirtualAddrLookupException::VirtualAddrLookupException(
    const std::string& array_name)
    : AladdinException(
          std::string("Could not find a virtual address mapping for array \"") +
          array_name + "\". " + helpfulSuggestion) {}

const std::string UnknownArrayException::helpfulSuggestion =
    "Please ensure that you have declared this array in your Aladdin "
    "configuration file with the correct partition type, size, and "
    "partition factor, and that you have not renamed pointers in your code ("
    "except through function calls).";

UnknownArrayException::UnknownArrayException(const std::string& array_name)
    : AladdinException(std::string("Unknown array \"") + array_name + "\". " +
                       helpfulSuggestion) {}

UnknownArrayException::UnknownArrayException(Addr array_addr)
    : AladdinException(std::string("Unknown array at address ") +
                       std::to_string(array_addr) + ". " + helpfulSuggestion) {}

const std::string IllegalHostMemoryAccessException::helpfulSuggestion =
    "If this is not supposed to be a host memory access, please ensure that "
    "the array being accessed has been declared in your Aladdin configuration "
    "file.";

IllegalHostMemoryAccessException::IllegalHostMemoryAccessException(
    const std::string& array_name)
    : AladdinException(std::string("Accessing host memory array \"") +
                       array_name +
                       "\" directly from the accelerator is not allowed! " +
                       helpfulSuggestion) {}

IllegalHostMemoryAccessException::IllegalHostMemoryAccessException(
    const ExecNode* node)
    : AladdinException(node,
                       std::string("Accessing host memory array \"") +
                           node->get_array_label() +
                           "\" directly from the accelerator is not allowed! " +
                           helpfulSuggestion) {}

ArrayAccessException::ArrayAccessException(const std::string& message)
    : AladdinException(message) {}

AddressTranslationException::AddressTranslationException(Addr vaddr,
                                                         unsigned size)
    : AladdinException(std::string(
          "Unable to translate simulation virtual address range: " +
          std::to_string(vaddr) + ", size " + std::to_string(size))) {}
