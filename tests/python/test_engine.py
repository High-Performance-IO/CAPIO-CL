import os
import importlib.util
from pathlib import PosixPath

if not os.path.exists(os.getenv("CAPIO_CL_PY_BINDING_PATH")):
     raise FileNotFoundError(os.getenv("CAPIO_CL_PY_BINDING_PATH"))

spec = importlib.util.spec_from_file_location("py_capio_cl", os.getenv("CAPIO_CL_PY_BINDING_PATH"))
py_capio_cl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(py_capio_cl)


def test_instantiation():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0


def test_add_file_default():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0

     engine.newFile("test.dat")
     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") == py_capio_cl.COMMITTED_ON_TERMINATION
     assert engine.getFireRule("test.dat") == py_capio_cl.MODE_UPDATE
     assert engine.getConsumers("test.dat") == []
     assert engine.getProducers("test.dat") == []
     assert not engine.isPermanent("test.dat")
     assert not engine.isExcluded("test.dat")
     assert engine.isFile("test.dat")
     assert not engine.isDirectory("test.dat")
     assert engine.getDirectoryFileCount("test.dat") == 0
     assert not engine.isStoredInMemory("test.dat")


def test_add_file_default_glob():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0

     engine.newFile("test.*")
     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") == py_capio_cl.COMMITTED_ON_TERMINATION
     assert engine.getFireRule("test.dat") == py_capio_cl.MODE_UPDATE
     assert engine.getConsumers("test.dat") == []
     assert engine.getProducers("test.dat") == []
     assert not engine.isPermanent("test.dat")
     assert not engine.isExcluded("test.dat")
     assert engine.isFile("test.dat")
     assert not engine.isDirectory("test.dat")
     assert engine.getDirectoryFileCount("test.dat") == 0
     assert not engine.isStoredInMemory("test.dat")


def test_add_file_default_glob_question():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0

     engine.newFile("test.?")
     assert engine.size() == 1
     assert engine.getCommitRule("test.1") == py_capio_cl.COMMITTED_ON_TERMINATION
     assert engine.getFireRule("test.1") == py_capio_cl.MODE_UPDATE
     assert engine.getConsumers("test.1") == []
     assert engine.getProducers("test.1") == []
     assert not engine.isPermanent("test.1")
     assert not engine.isExcluded("test.1")
     assert engine.isFile("test.1")
     assert not engine.isDirectory("test.1")
     assert engine.getDirectoryFileCount("test.1") == 0
     assert not engine.isStoredInMemory("test.1")


def test_add_file_manually():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     path = "test.dat"
     producers, consumers, file_dependencies = [], [], []

     engine.add(path, producers, consumers, py_capio_cl.COMMITTED_ON_TERMINATION,
                py_capio_cl.MODE_UPDATE, False, False, file_dependencies)

     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") == py_capio_cl.COMMITTED_ON_TERMINATION
     assert engine.getFireRule("test.dat") == py_capio_cl.MODE_UPDATE
     assert engine.getConsumers("test.dat") == []
     assert engine.getProducers("test.dat") == []
     assert not engine.isPermanent("test.dat")
     assert not engine.isExcluded("test.dat")
     assert engine.isFile("test.dat")
     assert not engine.isDirectory("test.dat")
     assert engine.getDirectoryFileCount("test.dat") == 0
     assert not engine.isStoredInMemory("test.dat")


def test_add_file_manually_glob():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     path = "test.*"
     producers, consumers, file_dependencies = [], [], []

     engine.add(path, producers, consumers, py_capio_cl.COMMITTED_ON_TERMINATION,
                py_capio_cl.MODE_UPDATE, False, False, file_dependencies)

     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") == py_capio_cl.COMMITTED_ON_TERMINATION
     assert engine.getFireRule("test.dat") == py_capio_cl.MODE_UPDATE
     assert engine.getConsumers("test.dat") == []
     assert engine.getProducers("test.dat") == []
     assert not engine.isPermanent("test.dat")
     assert not engine.isExcluded("test.dat")
     assert engine.isFile("test.dat")
     assert not engine.isDirectory("test.dat")
     assert engine.getDirectoryFileCount("test.dat") == 0
     assert not engine.isStoredInMemory("test.dat")


def test_add_file_manually_question():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     path = "test.?"
     producers, consumers, file_dependencies = [], [], []

     engine.add(path, producers, consumers, py_capio_cl.COMMITTED_ON_CLOSE,
                py_capio_cl.MODE_NO_UPDATE, False, False, file_dependencies)
     engine.setDirectory("test.?")
     engine.setDirectoryFileCount("test.?", 10)

     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") != py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.dat") != py_capio_cl.MODE_NO_UPDATE
     assert engine.getCommitRule("test.1") == py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.1") == py_capio_cl.MODE_NO_UPDATE
     assert engine.getCommitRule("test.2") == py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.2") == py_capio_cl.MODE_NO_UPDATE
     assert engine.isDirectory("test.1")
     assert engine.getDirectoryFileCount("test.?") == 10
     assert engine.getDirectoryFileCount("test.3") == 10
     assert engine.getConsumers("test.4") == []
     assert engine.getProducers("test.5") == []
     assert not engine.isPermanent("test.6")
     assert not engine.isExcluded("test.7")
     assert not engine.isFile("test.8")
     assert engine.isDirectory("test.9")
     assert engine.getDirectoryFileCount("test.a") == 10
     assert not engine.isStoredInMemory("test.b")


def test_add_file_manually_glob_explicit():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     path = "test.[abc][abc][abc]"
     producers, consumers, file_dependencies = [], [], []

     engine.add(path, producers, consumers, py_capio_cl.COMMITTED_ON_CLOSE,
                py_capio_cl.MODE_NO_UPDATE, False, False, file_dependencies)
     engine.setDirectory("test.[abc][abc][abc]")
     engine.setDirectoryFileCount("test.[abc][abc][abc]", 10)

     assert engine.size() == 1
     assert engine.getCommitRule("test.dat") != py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.dat") != py_capio_cl.MODE_NO_UPDATE
     assert engine.getCommitRule("test.abc") == py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.aaa") == py_capio_cl.MODE_NO_UPDATE
     assert engine.getCommitRule("test.cab") == py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.bac") == py_capio_cl.MODE_NO_UPDATE
     assert engine.getCommitRule("test.ccc") == py_capio_cl.COMMITTED_ON_CLOSE
     assert engine.getFireRule("test.aaa") == py_capio_cl.MODE_NO_UPDATE
     assert engine.isDirectory("test.bbb")
     assert engine.getDirectoryFileCount("test.3") != 10


def test_producers_consumers_file_dependencies():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     producers = ["A", "B"]
     consumers = ["C", "D"]
     file_dependencies = ["E", "F"]

     engine.newFile("test.dat")

     engine.addProducer("test.dat", producers[0])
     assert len(engine.getProducers("test.dat")) == 1
     assert engine.isProducer("test.dat", producers[0])

     engine.addProducer("test.dat", producers[1])
     assert len(engine.getProducers("test.dat")) == 2
     assert engine.isProducer("test.dat", producers[1])

     engine.addConsumer("test.dat", consumers[0])
     assert len(engine.getConsumers("test.dat")) == 1
     assert engine.isConsumer("test.dat", consumers[0])

     engine.addConsumer("test.dat", consumers[1])
     assert len(engine.getConsumers("test.dat")) == 2
     assert engine.isConsumer("test.dat", consumers[1])

     assert engine.getCommitOnFileDependencies("test.dat") == []
     engine.addFileDependency("test.dat", file_dependencies[0])
     assert len(engine.getCommitOnFileDependencies("test.dat")) == 1
     assert engine.getCommitOnFileDependencies("test.dat")[0] == PosixPath(file_dependencies[0])
     engine.addFileDependency("test.dat", file_dependencies[1])
     assert len(engine.getCommitOnFileDependencies("test.dat")) == 2
     assert engine.getCommitOnFileDependencies("test.dat")[1] == PosixPath(file_dependencies[1])


def test_producers_consumers_file_dependencies_glob():
     engine = py_capio_cl.Engine()
     assert engine.size() == 0
     producers = ["A", "B"]
     consumers = ["C", "D"]
     file_dependencies = ["E", "F"]

     engine.newFile("test.*")

     engine.addProducer("test.dat", producers[0])
     assert len(engine.getProducers("test.dat")) == 1
     assert engine.isProducer("test.dat", producers[0])

     engine.addProducer("test.dat", producers[1])
     assert len(engine.getProducers("test.dat")) == 2
     assert engine.isProducer("test.dat", producers[1])

     engine.addConsumer("test.dat", consumers[0])
     assert len(engine.getConsumers("test.dat")) == 1
     assert engine.isConsumer("test.dat", consumers[0])

     engine.addConsumer("test.dat", consumers[1])
     assert len(engine.getConsumers("test.dat")) == 2
     assert engine.isConsumer("test.dat", consumers[1])

     assert engine.getCommitOnFileDependencies("test.dat") == []
     engine.addFileDependency("test.dat", file_dependencies[0])
     assert len(engine.getCommitOnFileDependencies("test.dat")) == 1
     assert engine.getCommitOnFileDependencies("test.dat")[0] == PosixPath(file_dependencies[0])
     engine.addFileDependency("test.dat", file_dependencies[1])
     assert len(engine.getCommitOnFileDependencies("test.dat")) == 2
     assert engine.getCommitOnFileDependencies("test.dat")[1] == PosixPath(file_dependencies[1])