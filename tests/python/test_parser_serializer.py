import os
from pathlib import Path, PosixPath

import py_capio_cl


def test_serialize_parse_py_capio_cl_v1(tmp_path):
    path = tmp_path / "config.json"
    workflow_name = "demo"
    file1 = PosixPath("file1.txt")
    file2 = PosixPath("file2.txt")
    file3 = PosixPath("my_command_history.txt")
    file4 = PosixPath("/tmp")

    producer = "_first"
    consumer = "_last"
    intermediate = "_middle"

    engine = py_capio_cl.Engine()
    engine.setWorkflowName(workflow_name)

    # producer/consumer graph
    engine.addProducer(file1, producer)
    engine.addConsumer(file1, intermediate)
    engine.addProducer(file2, intermediate)
    engine.addConsumer(file2, consumer)
    engine.addConsumer(file1, consumer)

    # File 1: in-memory, ON_CLOSE, UPDATE, permanent
    engine.setStoreFileInMemory(file1)
    engine.setCommitRule(file1, py_capio_cl.commit_rules.ON_CLOSE)
    engine.setCommittedCloseNumber(file1, 3)
    engine.setFireRule(file1, py_capio_cl.fire_rules.UPDATE)
    engine.setPermanent(file1, True)

    # File 2: on filesystem, ON_TERMINATION, NO_UPDATE
    engine.setStoreFileInFileSystem(file2)
    engine.setCommitRule(file2, py_capio_cl.commit_rules.ON_TERMINATION)
    engine.setFireRule(file1, py_capio_cl.fire_rules.NO_UPDATE)

    # File 3: excluded, multiple producers
    for name in [producer, consumer, intermediate]:
        engine.addProducer(file3, name)
    engine.setExclude(file3, True)

    # File 4: directory with 10 files, ON_N_FILES commit rule
    engine.setCommitRule(file4, py_capio_cl.commit_rules.ON_N_FILES)
    engine.setFireRule(file4, py_capio_cl.fire_rules.NO_UPDATE)
    engine.setDirectoryFileCount(file4, 10)
    engine.addProducer(file4, intermediate)

    engine.print()

    # Serialize
    py_capio_cl.serialize(engine, path)

    # Parse back
    new_engine = py_capio_cl.Parser.parse(path)

    assert new_engine.getWorkflowName() == workflow_name

    # Parse with memory flag
    new_engine1 = py_capio_cl.Parser.parse(path, store_only_in_memory=True)
    assert len(new_engine1.getFileToStoreInMemory()) == engine.size()

    # cleanup
    os.remove(path)


def test_parser_resolve_absolute():
    json_path = "/tmp/capio_cl_jsons/V1.0/test0.json"

    engine = py_capio_cl.Parser.parse(str(json_path), "/tmp")
    assert engine.getWorkflowName() == "test"
    for f in ["/tmp/file", "/tmp/file1", "/tmp/file2", "/tmp/file3"]:
        assert engine.contains(f)


def test_parser_exception():
    json_dir = Path("/tmp/capio_cl_jsons")
    test_filenames = [
        "",
        "ANonExistingFile",
        *(str(json_dir / f"V1_test{i}.json") for i in range(1, 24)),
    ]

    for test_file in test_filenames:
        caught = False
        try:
            wf_name, engine = py_capio_cl.Parser.parse(test_file)
        except Exception as e:
            caught = True
            typename = type(e).__name__
            assert typename == "ParserException"
            assert len(str(e)) > 0
        assert caught
