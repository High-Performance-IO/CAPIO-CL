#include "capiocl.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

namespace py = pybind11;

PYBIND11_MODULE(py_capio_cl, m) {
    m.doc() =
        "CAPIO-CL: Cross Application Programmable I/O - Coordination Language python bindings.";
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
        .def("__str__", &capiocl::Engine::print)
        .def("__repr__", [](const capiocl::Engine &e) {
            return "<Engine repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
        });

    m.attr("MODE_UPDATE")              = py::str(capiocl::MODE_UPDATE);
    m.attr("MODE_NO_UPDATE")           = py::str(capiocl::MODE_NO_UPDATE);
    m.attr("COMMITTED_ON_CLOSE")       = py::str(capiocl::COMMITTED_ON_CLOSE);
    m.attr("COMMITTED_ON_FILE")        = py::str(capiocl::COMMITTED_ON_FILE);
    m.attr("COMMITTED_N_FILES")        = py::str(capiocl::COMMITTED_N_FILES);
    m.attr("COMMITTED_ON_TERMINATION") = py::str(capiocl::COMMITTED_ON_TERMINATION);

    py::class_<capiocl::Parser>(m, "Parser", "The CAPIO-CL Parser component.")
        .def("parse", &capiocl::Parser::parse)
        .def("__str__",
             [](const capiocl::Parser &e) {
                 return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def("__repr__", [](const capiocl::Engine &e) {
            return "<Parser repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
        });
}