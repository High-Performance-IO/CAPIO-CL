import socket
import time
from pathlib import PosixPath

import py_capio_cl


def test_instantiation():
    engine = py_capio_cl.Engine()
    assert engine.size() == 0
    engine.print()


def test_add_file_default():
    engine = py_capio_cl.Engine()
    assert engine.size() == 0
    engine.newFile("test.dat")
    assert engine.size() == 1
    assert engine.getCommitRule("test.dat") == py_capio_cl.commit_rules.ON_TERMINATION
    assert engine.getFireRule("test.dat") == py_capio_cl.fire_rules.UPDATE
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
    engine.newFile("test.*")
    assert engine.size() == 1
    assert engine.getCommitRule("test.dat") == py_capio_cl.commit_rules.ON_TERMINATION
    assert engine.getFireRule("test.dat") == py_capio_cl.fire_rules.UPDATE
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
    engine.newFile("test.?")
    assert engine.size() == 1
    assert engine.getCommitRule("test.1") == py_capio_cl.commit_rules.ON_TERMINATION
    assert engine.getFireRule("test.1") == py_capio_cl.fire_rules.UPDATE
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
    path = "test.dat"
    engine.add(path, [], [], py_capio_cl.commit_rules.ON_TERMINATION,
               py_capio_cl.fire_rules.UPDATE, False, False, [])
    assert engine.size() == 1
    assert engine.getCommitRule("test.dat") == py_capio_cl.commit_rules.ON_TERMINATION
    assert engine.getFireRule("test.dat") == py_capio_cl.fire_rules.UPDATE
    assert engine.getConsumers("test.dat") == []
    assert engine.getProducers("test.dat") == []
    assert not engine.isPermanent("test.dat")
    assert not engine.isExcluded("test.dat")
    assert engine.isFile("test.dat")
    assert not engine.isDirectory("test.dat")
    assert engine.getDirectoryFileCount("test.dat") == 0
    assert not engine.isStoredInMemory("test.dat")
    engine.setFile("test.txt")


def test_add_file_manually_glob():
    engine = py_capio_cl.Engine()
    path = "test.*"
    engine.add(path, [], [], py_capio_cl.commit_rules.ON_TERMINATION,
               py_capio_cl.fire_rules.UPDATE, False, False, [])
    assert engine.size() == 1
    assert engine.getCommitRule("test.dat") == py_capio_cl.commit_rules.ON_TERMINATION
    assert engine.getFireRule("test.dat") == py_capio_cl.fire_rules.UPDATE
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
    path = "test.?"
    engine.add(path, [], [], py_capio_cl.commit_rules.ON_CLOSE,
               py_capio_cl.fire_rules.NO_UPDATE, False, False, [])
    engine.setDirectory("test.?")
    engine.setDirectoryFileCount("test.?", 10)

    assert engine.size() == 1
    assert engine.getCommitRule("test.1") == py_capio_cl.commit_rules.ON_CLOSE
    assert engine.getFireRule("test.1") == py_capio_cl.fire_rules.NO_UPDATE
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

    engine.setDirectoryFileCount("myDir", 10)
    assert engine.getDirectoryFileCount("myDir") == 10


def test_add_file_manually_glob_explicit():
    engine = py_capio_cl.Engine()
    path = "test.[abc][abc][abc]"
    engine.add(path, [], [], py_capio_cl.commit_rules.ON_CLOSE,
               py_capio_cl.fire_rules.NO_UPDATE, False, False, [])
    engine.setDirectory(path)
    engine.setDirectoryFileCount(path, 10)
    assert engine.size() == 1
    assert engine.getCommitRule("test.cab") == py_capio_cl.commit_rules.ON_CLOSE
    assert engine.getFireRule("test.bac") == py_capio_cl.fire_rules.NO_UPDATE
    assert engine.isDirectory("test.bbb")


def test_producers_consumers_file_dependencies():
    engine = py_capio_cl.Engine()
    producers = ["A", "B"]
    consumers = ["C", "D"]
    deps = [PosixPath("E"), PosixPath("F")]

    engine.newFile("test.dat")
    engine.addProducer("test.dat", producers[0])
    assert engine.isProducer("test.dat", producers[0])
    engine.addProducer("test.dat", producers[1])
    assert engine.isProducer("test.dat", producers[1])

    engine.addConsumer("test.dat", consumers[0])
    assert engine.isConsumer("test.dat", consumers[0])
    engine.addConsumer("test.dat", consumers[1])
    assert engine.isConsumer("test.dat", consumers[1])

    assert engine.getCommitOnFileDependencies("test.dat") == []
    engine.addFileDependency("test.dat", deps[0])
    assert engine.getCommitOnFileDependencies("test.dat")[0] == deps[0]
    engine.addFileDependency("test.dat", deps[1])
    assert engine.getCommitOnFileDependencies("test.dat")[1] == deps[1]

    engine.addFileDependency("myFile.txt", deps[0])
    assert engine.getCommitRule("myFile.txt") == py_capio_cl.commit_rules.ON_FILE
    assert engine.getCommitOnFileDependencies("myFile.txt")[0] == deps[0]


def test_commit_fire_permanent_exclude_on_globs():
    engine = py_capio_cl.Engine()
    engine.newFile("test.*")
    engine.setFireRule("test.*", py_capio_cl.fire_rules.NO_UPDATE)
    assert engine.isFirable("test.a")
    assert not engine.isFirable("testb")

    engine.setCommitRule("testb", py_capio_cl.commit_rules.ON_FILE)
    engine.setFileDeps("testb", ["test.a"])

    engine.setPermanent("myFile", True)
    assert engine.isPermanent("myFile")
    assert not engine.isFirable("myFile")

    assert not engine.isExcluded("testb")
    engine.setExclude("testb", True)
    assert engine.isExcluded("testb")
    engine.setExclude("myFile.*", True)
    assert engine.isExcluded("myFile.txt")
    assert engine.isExcluded("myFile.dat")

    engine.setFireRule("test.c", py_capio_cl.fire_rules.NO_UPDATE)
    assert engine.isFirable("test.c")

    engine.setCommittedCloseNumber("test.e", 100)


def test_is_file_is_directory_glob():
    engine = py_capio_cl.Engine()
    engine.newFile("test.*")
    engine.setDirectory("test.d/")
    engine.setDirectory("test.d/bin/lib")
    assert engine.isDirectory("test.d/")
    assert engine.isDirectory("test.d/bin/lib")
    assert not engine.isDirectory("test.*")


def test_add_remove_file():
    engine = py_capio_cl.Engine()
    engine.newFile("test.*")
    assert engine.contains("test.*")
    assert engine.contains("test.txt")
    engine.remove("test.*")
    assert not engine.contains("test.*")
    engine.remove("data")
    assert not engine.contains("data")


def test_commit_close_count():
    engine = py_capio_cl.Engine()
    engine.newFile("test.*")
    engine.setCommitRule("test.*", py_capio_cl.commit_rules.ON_CLOSE)
    engine.setCommittedCloseNumber("test.e", 100)
    assert engine.getCommitCloseCount("test.e") == 100
    engine.setCommittedCloseNumber("test.*", 30)
    assert engine.getCommitCloseCount("test.f") == 30
    engine.setCommitRule("myFile", py_capio_cl.commit_rules.ON_FILE)
    assert engine.getCommitRule("myFile") == py_capio_cl.commit_rules.ON_FILE


def test_storage_options(tmp_path):
    engine = py_capio_cl.Engine()
    for f in ["A", "B"]:
        engine.newFile(f)

    engine.setStoreFileInMemory("A")
    assert engine.isStoredInMemory("A")
    assert not engine.isStoredInMemory("B")

    engine.setStoreFileInMemory("B")
    assert engine.isStoredInMemory("A")
    assert engine.isStoredInMemory("B")

    engine.setStoreFileInMemory("C")
    assert engine.isStoredInMemory("C")

    engine.newFile("D")
    assert not engine.isStoredInMemory("D")

    engine.setAllStoreInMemory()
    for f in ["A", "B", "C", "D"]:
        assert engine.isStoredInMemory(f)

    assert len(engine.getFileToStoreInMemory()) == 4

    for f in ["A", "B", "C", "D"]:
        engine.setStoreFileInFileSystem(f)
        assert not engine.isStoredInMemory(f)

    engine.setStoreFileInFileSystem("F")
    assert not engine.isStoredInMemory("F")


def test_home_node():
    """
    This test is skipped on macOS runners due to the issue described here:
    https://github.com/actions/runner-images/issues/10924.
    The test will only be executed on Linux-based runners, not macOS.
    """
    this_node_name = socket.gethostname()
    engine = py_capio_cl.Engine()
    engine.newFile("./A")
    assert this_node_name not in engine.getHomeNode("B")
    assert this_node_name not in engine.getHomeNode("./A")
    engine1 = py_capio_cl.Engine()
    engine1.setHomeNode("./A")
    time.sleep(1)
    assert this_node_name in engine.getHomeNode("./A")


def test_commit_status():
    """
    This test is skipped on macOS runners due to the issue described here:
    https://github.com/actions/runner-images/issues/10924.
    The test will only be executed on Linux-based runners, not macOS.
    """
    engine = py_capio_cl.Engine()
    engine.setCommitted("./C")
    assert engine.isCommitted("./C")

    engine1 = py_capio_cl.Engine()
    assert engine1.isCommitted("./C")


def test_insert_file_dependencies():
    engine = py_capio_cl.Engine()
    engine.setFileDeps("myFile.txt", [])
    engine.setFileDeps("test.txt", ["a", "b", "c"])
    deps = engine.getCommitOnFileDependencies("test.txt")
    assert deps == [PosixPath("a"), PosixPath("b"), PosixPath("c")]
