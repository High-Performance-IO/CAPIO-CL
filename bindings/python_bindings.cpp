#include "capiocl.hpp"
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

namespace py = pybind11;

PYBIND11_MODULE(_py_capio_cl, m) {
    m.doc() =
        "CAPIO-CL: Cross Application Programmable I/O - Coordination Language python bindings.";

    py::register_exception<capiocl::ParserException>(m, "ParserException");

    py::module_ fire_rules       = m.def_submodule("fire_rules", "CAPIO-CL fire rules");
    fire_rules.attr("UPDATE")    = py::str(capiocl::fire_rules::UPDATE);
    fire_rules.attr("NO_UPDATE") = py::str(capiocl::fire_rules::NO_UPDATE);

    py::module_ commit_rules            = m.def_submodule("commit_rules", "CAPIO-CL commit rules");
    commit_rules.attr("ON_CLOSE")       = py::str(capiocl::commit_rules::ON_CLOSE);
    commit_rules.attr("ON_FILE")        = py::str(capiocl::commit_rules::ON_FILE);
    commit_rules.attr("ON_N_FILES")     = py::str(capiocl::commit_rules::ON_N_FILES);
    commit_rules.attr("ON_TERMINATION") = py::str(capiocl::commit_rules::ON_TERMINATION);

    py::module_ VERSION = m.def_submodule("VERSION", "CAPIO-CL version");
    VERSION.attr("V1")  = py::str(capiocl::CAPIO_CL_VERSION::V1);

    py::class_<capiocl::Engine>(
        m, "Engine", "The main CAPIO-CL engine for managing data communication and I/O operations.")
        .def(py::init<>())
        .def("newFile", &capiocl::Engine::newFile)
        .def("print", &capiocl::Engine::print)
        .def("contains", &capiocl::Engine::contains)
        .def("size", &capiocl::Engine::size)
        .def("add", &capiocl::Engine::add)
        .def("addProducer", &capiocl::Engine::addProducer)
        .def("addConsumer", &capiocl::Engine::addConsumer)
        .def("addFileDependency", &capiocl::Engine::addFileDependency)
        .def("remove", &capiocl::Engine::remove)
        .def("setCommitRule", &capiocl::Engine::setCommitRule)
        .def("setFireRule", &capiocl::Engine::setFireRule)
        .def("setPermanent", &capiocl::Engine::setPermanent)
        .def("setExclude", &capiocl::Engine::setExclude)
        .def("setDirectory", &capiocl::Engine::setDirectory)
        .def("setFile", &capiocl::Engine::setFile)
        .def("setCommittedCloseNumber", &capiocl::Engine::setCommitedCloseNumber)
        .def("setDirectoryFileCount", &capiocl::Engine::setDirectoryFileCount)
        .def("setFileDeps", &capiocl::Engine::setFileDeps)
        .def("setStoreFileInMemory", &capiocl::Engine::setStoreFileInMemory)
        .def("setStoreFileInFileSystem", &capiocl::Engine::setStoreFileInFileSystem)
        .def("getDirectoryFileCount", &capiocl::Engine::getDirectoryFileCount)
        .def("getCommitRule", &capiocl::Engine::getCommitRule)
        .def("getFireRule", &capiocl::Engine::getFireRule)
        .def("getProducers", &capiocl::Engine::getProducers)
        .def("getConsumers", &capiocl::Engine::getConsumers)
        .def("getCommitCloseCount", &capiocl::Engine::getCommitCloseCount)
        .def("getCommitOnFileDependencies", &capiocl::Engine::getCommitOnFileDependencies)
        .def("getFileToStoreInMemory", &capiocl::Engine::getFileToStoreInMemory)
        .def("getHomeNode", &capiocl::Engine::getHomeNode)
        .def("isProducer", &capiocl::Engine::isProducer)
        .def("isConsumer", &capiocl::Engine::isConsumer)
        .def("isFirable", &capiocl::Engine::isFirable)
        .def("isFile", &capiocl::Engine::isFile)
        .def("isExcluded", &capiocl::Engine::isExcluded)
        .def("isDirectory", &capiocl::Engine::isDirectory)
        .def("isStoredInMemory", &capiocl::Engine::isStoredInMemory)
        .def("isPermanent", &capiocl::Engine::isPermanent)
        .def("setAllStoreInMemory", &capiocl::Engine::setAllStoreInMemory)
        .def("getWorkflowName", &capiocl::Engine::getWorkflowName)
        .def("setWorkflowName", &capiocl::Engine::setWorkflowName)
        .def("__str__", &capiocl::Engine::print)
        .def("__repr__",
             [](const capiocl::Engine &e) {
                 return "<Engine repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def(pybind11::self == pybind11::self);

    py::class_<capiocl::Parser>(m, "Parser", "The CAPIO-CL Parser component.")
        .def_static("parse", &capiocl::Parser::parse, py::arg("source"),
                    py::arg("resolve_prefix") = "", py::arg("store_only_in_memory") = false)
        .def("__str__",
             [](const capiocl::Parser &e) {
                 return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def("__repr__", [](const capiocl::Parser &e) {
            return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
        });

    py::class_<capiocl::Serializer>(m, "Serializer", "The CAPIO-CL Serializer component.")
        .def(py::init<>())
        .def_static("dump", &capiocl::Serializer::dump, py::arg("engine"), py::arg("workflow_name"),
                    py::arg("filename"), py::arg("version") = capiocl::CAPIO_CL_VERSION::V1)
        .def("__str__",
             [](const capiocl::Serializer &e) {
                 return "<Serializer repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) +
                        ">";
             })
        .def("__repr__", [](const capiocl::Serializer &e) {
            return "<Serializer repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
        });
}