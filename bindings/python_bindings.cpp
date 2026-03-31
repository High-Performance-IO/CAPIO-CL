#include <iostream>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <string>

#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/monitor.h"
#include "capiocl/parser.h"
#include "capiocl/serializer.h"

namespace py = pybind11;

PYBIND11_MODULE(_py_capio_cl, m) {
    m.doc() =
        "CAPIO-CL: Cross Application Programmable I/O - Coordination Language python bindings.";

    py::register_exception<capiocl::parser::ParserException>(m, "ParserException");
    py::register_exception<capiocl::serializer::SerializerException>(m, "SerializerException");
    py::register_exception<capiocl::monitor::MonitorException>(m, "MonitorException");

    m.attr("CAPIO_CL_DEFAULT_WF_NAME") = py::str(capiocl::CAPIO_CL_DEFAULT_WF_NAME);
    m.attr("DEFAULT_MCAST_GROUP") =
        py::make_tuple(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                       std::stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v));

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
        .def("newFile", &capiocl::engine::Engine::newFile, py::arg("filename"))
        .def("print", &capiocl::engine::Engine::print)
        .def("contains", &capiocl::engine::Engine::contains, py::arg("path"))
        .def("size", &capiocl::engine::Engine::size)
        .def("add",
             py::overload_cast<std::filesystem::path &, std::vector<std::string> &,
                               std::vector<std::string> &, const std::string &, const std::string &,
                               bool, bool, std::vector<std::filesystem::path> &>(
                 &capiocl::engine::Engine::add),
             py::arg("path"), py::arg("producers"), py::arg("consumers"), py::arg("commit_rule"),
             py::arg("fire_rule"), py::arg("permanent"), py::arg("exclude"),
             py::arg("dependencies"))
        .def("addProducer", &capiocl::engine::Engine::addProducer, py::arg("path"),
             py::arg("producer"))
        .def("addConsumer", &capiocl::engine::Engine::addConsumer, py::arg("path"),
             py::arg("consumer"))
        .def("addFileDependency", &capiocl::engine::Engine::addFileDependency, py::arg("path"),
             py::arg("file_dependency"))
        .def("remove", &capiocl::engine::Engine::remove, py::arg("path"))
        .def("setCommitRule", &capiocl::engine::Engine::setCommitRule, py::arg("path"),
             py::arg("commit_rule"))
        .def("setFireRule", &capiocl::engine::Engine::setFireRule, py::arg("path"),
             py::arg("fire_rule"))
        .def("setPermanent", &capiocl::engine::Engine::setPermanent, py::arg("path"),
             py::arg("permanent"))
        .def("setExclude", &capiocl::engine::Engine::setExclude, py::arg("path"),
             py::arg("exclude"))
        .def("setDirectory", &capiocl::engine::Engine::setDirectory, py::arg("path"))
        .def("setFile", &capiocl::engine::Engine::setFile, py::arg("path"))
        .def("setCommittedCloseNumber", &capiocl::engine::Engine::setCommitedCloseNumber,
             py::arg("path"), py::arg("num"))
        .def("setDirectoryFileCount", &capiocl::engine::Engine::setDirectoryFileCount,
             py::arg("path"), py::arg("num"))
        .def("setFileDeps", &capiocl::engine::Engine::setFileDeps, py::arg("path"),
             py::arg("dependencies"))
        .def("setStoreFileInMemory", &capiocl::engine::Engine::setStoreFileInMemory,
             py::arg("path"))
        .def("setStoreFileInFileSystem", &capiocl::engine::Engine::setStoreFileInFileSystem,
             py::arg("path"))
        .def("getDirectoryFileCount", &capiocl::engine::Engine::getDirectoryFileCount,
             py::arg("path"))
        .def("getCommitRule", &capiocl::engine::Engine::getCommitRule, py::arg("path"))
        .def("getFireRule", &capiocl::engine::Engine::getFireRule, py::arg("path"))
        .def("getProducers", &capiocl::engine::Engine::getProducers, py::arg("path"))
        .def("getConsumers", &capiocl::engine::Engine::getConsumers, py::arg("path"))
        .def("getCommitCloseCount", &capiocl::engine::Engine::getCommitCloseCount, py::arg("path"))
        .def("getCommitOnFileDependencies", &capiocl::engine::Engine::getCommitOnFileDependencies,
             py::arg("path"))
        .def("getFileToStoreInMemory", &capiocl::engine::Engine::getFileToStoreInMemory)
        .def("getHomeNode", &capiocl::engine::Engine::getHomeNode, py::arg("path"))
        .def("isProducer", &capiocl::engine::Engine::isProducer, py::arg("path"),
             py::arg("app_name"))
        .def("isConsumer", &capiocl::engine::Engine::isConsumer, py::arg("path"),
             py::arg("app_name"))
        .def("isFirable", &capiocl::engine::Engine::isFirable, py::arg("path"))
        .def("isFile", &capiocl::engine::Engine::isFile, py::arg("path"))
        .def("isExcluded", &capiocl::engine::Engine::isExcluded, py::arg("path"))
        .def("isDirectory", &capiocl::engine::Engine::isDirectory, py::arg("path"))
        .def("isStoredInMemory", &capiocl::engine::Engine::isStoredInMemory, py::arg("path"))
        .def("isPermanent", &capiocl::engine::Engine::isPermanent, py::arg("path"))
        .def("setAllStoreInMemory", &capiocl::engine::Engine::setAllStoreInMemory)
        .def("getWorkflowName", &capiocl::engine::Engine::getWorkflowName)
        .def("setWorkflowName", &capiocl::engine::Engine::setWorkflowName, py::arg("name"))
        .def("setCommitted", &capiocl::engine::Engine::setCommitted, py::arg("path"))
        .def("isCommitted", &capiocl::engine::Engine::isCommitted, py::arg("path"))
        .def("setHomeNode", &capiocl::engine::Engine::setHomeNode, py::arg("path"))
        .def("getPaths", &capiocl::engine::Engine::getPaths)
        .def("startApiServer", &capiocl::engine::Engine::startApiServer)
        .def("__str__", &capiocl::engine::Engine::print)
        .def("__repr__",
             [](const capiocl::engine::Engine &e) {
                 return "<Engine repr at " + std::to_string(reinterpret_cast<uintptr_t>(&e)) + ">";
             })
        .def(py::self == py::self);

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

    py::class_<capiocl::engine::CapioCLEntry>(m, "CapioCLEntry")
        .def(py::init<>())
        .def_readwrite("producers", &capiocl::engine::CapioCLEntry::producers)
        .def_readwrite("consumers", &capiocl::engine::CapioCLEntry::consumers)
        .def_readwrite("file_dependencies", &capiocl::engine::CapioCLEntry::file_dependencies)
        .def_readwrite("commit_rule", &capiocl::engine::CapioCLEntry::commit_rule)
        .def_readwrite("fire_rule", &capiocl::engine::CapioCLEntry::fire_rule)
        .def_readwrite("directory_children_count",
                       &capiocl::engine::CapioCLEntry::directory_children_count)
        .def_readwrite("commit_on_close_count",
                       &capiocl::engine::CapioCLEntry::commit_on_close_count)
        .def_readwrite("enable_directory_count_update",
                       &capiocl::engine::CapioCLEntry::enable_directory_count_update)
        .def_readwrite("store_in_memory", &capiocl::engine::CapioCLEntry::store_in_memory)
        .def_readwrite("permanent", &capiocl::engine::CapioCLEntry::permanent)
        .def_readwrite("excluded", &capiocl::engine::CapioCLEntry::excluded)
        .def_readwrite("is_file", &capiocl::engine::CapioCLEntry::is_file)
        .def_static("from_json", &capiocl::engine::CapioCLEntry::fromJson, py::arg("in"))
        .def("to_json", &capiocl::engine::CapioCLEntry::toJson);
}