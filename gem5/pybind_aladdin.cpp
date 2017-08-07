/* Add additional PyBind11 bindings for Aladdin.
 *
 * HybridDatapath inherits multiply from Gem5Datapath (which is a MemObject)
 * and ScratchpadDatapath. The latter is purely for Aladdin and has no
 * connection to gem5, but PyBind11 needs to know about it in order to create
 * Python bindings for a multiply-inherited object.
 */

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "sim/init.hh"

#include "aladdin/common/ScratchpadDatapath.h"

namespace py = pybind11;

static void module_init(py::module &m_internal) {
  py::module m = m_internal.def_submodule("param_Aladdin");
  py::class_<BaseDatapath, std::unique_ptr<BaseDatapath, py::nodelete>>(
      m, "BaseDatapath");
  py::class_<ScratchpadDatapath, BaseDatapath,
             std::unique_ptr<ScratchpadDatapath, py::nodelete>>(
      m, "ScratchpadDatapath");
}

static EmbeddedPyBind embed_obj("ScratchpadDatapath", module_init, "");
