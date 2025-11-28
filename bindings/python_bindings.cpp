#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/monitor.h"
#include "capiocl/parser.h"
#include "capiocl/printer.h"
#include "capiocl/serializer.h"

namespace py = pybind11;

PYBIND11_MODULE(_py_capio_cl, m) {
    m.doc() =
        "CAPIO-CL: Cross Application Programmable I/O - Coordination Language python bindings.";

    py::register_exception<capiocl::parser::ParserException>(m, "ParserException");
    py::register_exception<capiocl::serializer::SerializerException>(m, "SerializerException");
    py::register_exception<capiocl::monitor::MonitorException>(m, "MonitorException");

    py::module_ fire_rules       = m.def_submodule("fire_rules", "CAPIO-CL fire rules");
    fire_rules.attr("UPDATE")    = py::str(capiocl::fireRules::UPDATE);
    fire_rules.attr("NO_UPDATE") = py::str(capiocl::fireRules::NO_UPDATE);

    py::module_ commit_rules            = m.def_submodule("commit_rules", "CAPIO-CL commit rules");
    commit_rules.attr("ON_CLOSE")       = py::str(capiocl::commitRules::ON_CLOSE);
    commit_rules.attr("ON_FILE")        = py::str(capiocl::commitRules::ON_FILE);
    commit_rules.attr("ON_N_FILES")     = py::str(capiocl::commitRules::ON_N_FILES);
    commit_rules.attr("ON_TERMINATION") = py::str(capiocl::commitRules::ON_TERMINATION);

    py::module_ VERSION = m.def_submodule("VERSION", "CAPIO-CL version");
    VERSION.attr("V1")  = py::str(capiocl::CAPIO_CL_VERSION::V1);

    py::class_<capiocl::engine::Engine>(
        m, "Engine", "The main CAPIO-CL engine for managing data communication and I/O operations.")
        .def(py::init<>())
        .def("newFile", &capiocl::engine::Engine::newFile)
        .def("print", &capiocl::engine::Engine::print)
        .def("contains", &capiocl::engine::Engine::contains)
        .def("size", &capiocl::engine::Engine::size)
        .def("add", &capiocl::engine::Engine::add)
        .def("addProducer", &capiocl::engine::Engine::addProducer)
        .def("addConsumer", &capiocl::engine::Engine::addConsumer)
        .def("addFileDependency", &capiocl::engine::Engine::addFileDependency)
        .def("remove", &capiocl::engine::Engine::remove)
        .def("setCommitRule", &capiocl::engine::Engine::setCommitRule)
        .def("setFireRule", &capiocl::engine::Engine::setFireRule)
        .def("setPermanent", &capiocl::engine::Engine::setPermanent)
        .def("setExclude", &capiocl::engine::Engine::setExclude)
        .def("setDirectory", &capiocl::engine::Engine::setDirectory)
        .def("setFile", &capiocl::engine::Engine::setFile)
        .def("setCommittedCloseNumber", &capiocl::engine::Engine::setCommitedCloseNumber)
        .def("setDirectoryFileCount", &capiocl::engine::Engine::setDirectoryFileCount)
        .def("setFileDeps", &capiocl::engine::Engine::setFileDeps)
        .def("setStoreFileInMemory", &capiocl::engine::Engine::setStoreFileInMemory)
        .def("setStoreFileInFileSystem", &capiocl::engine::Engine::setStoreFileInFileSystem)
        .def("getDirectoryFileCount", &capiocl::engine::Engine::getDirectoryFileCount)
        .def("getCommitRule", &capiocl::engine::Engine::getCommitRule)
        .def("getFireRule", &capiocl::engine::Engine::getFireRule)
        .def("getProducers", &capiocl::engine::Engine::getProducers)
        .def("getConsumers", &capiocl::engine::Engine::getConsumers)
        .def("getCommitCloseCount", &capiocl::engine::Engine::getCommitCloseCount)
        .def("getCommitOnFileDependencies", &capiocl::engine::Engine::getCommitOnFileDependencies)
        .def("getFileToStoreInMemory", &capiocl::engine::Engine::getFileToStoreInMemory)
        .def("getHomeNode", &capiocl::engine::Engine::getHomeNode)
        .def("isProducer", &capiocl::engine::Engine::isProducer)
        .def("isConsumer", &capiocl::engine::Engine::isConsumer)
        .def("isFirable", &capiocl::engine::Engine::isFirable)
        .def("isFile", &capiocl::engine::Engine::isFile)
        .def("isExcluded", &capiocl::engine::Engine::isExcluded)
        .def("isDirectory", &capiocl::engine::Engine::isDirectory)
        .def("isStoredInMemory", &capiocl::engine::Engine::isStoredInMemory)
        .def("isPermanent", &capiocl::engine::Engine::isPermanent)
        .def("setAllStoreInMemory", &capiocl::engine::Engine::setAllStoreInMemory)
        .def("getWorkflowName", &capiocl::engine::Engine::getWorkflowName)
        .def("setWorkflowName", &capiocl::engine::Engine::setWorkflowName)
        .def("setCommitted", &capiocl::engine::Engine::setCommitted)
        .def("isCommitted", &capiocl::engine::Engine::isCommitted)
        .def("setHomeNode", &capiocl::engine::Engine::setHomeNode)
        .def("getPaths", &capiocl::engine::Engine::getPaths)
        .def("__str__", &capiocl::engine::Engine::print)
        .def("__repr__",
             [](const capiocl::engine::Engine &e) {
                 return "<Engine repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def(pybind11::self == pybind11::self);

    py::class_<capiocl::parser::Parser>(m, "Parser", "The CAPIO-CL Parser component.")
        .def_static("parse", &capiocl::parser::Parser::parse, py::arg("source"),
                    py::arg("resolve_prefix") = "", py::arg("store_only_in_memory") = false)
        .def("__str__",
             [](const capiocl::parser::Parser &e) {
                 return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def("__repr__", [](const capiocl::parser::Parser &e) {
            return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
        });

    m.def("serialize", &capiocl::serializer::Serializer::dump, py::arg("engine"),
          py::arg("filename"), py::arg("version") = capiocl::CAPIO_CL_VERSION::V1);
}