from functools import wraps

import requests


def CapioCLRule(path: str,
                committed: str | None = None,
                fire: str | None = None,
                close_count: int | None = None,
                directory_n_file_expected: int | None = None,
                is_directory: bool | None = None,
                is_permanent: bool | None = None,
                is_excluded: bool | None = None,
                producers: list[str] | None = None,
                consumers: list[str] | None = None,
                file_dependencies: list[str] | None = None
                ):
    if not path:
        raise RuntimeError("ERROR: cannot specify a CAPIO-CL rule without setting a path!")

    def _perform_request(endpoint, payload=None):
        response = requests.post(endpoint, json=payload, headers={"content-type": "application/json"})
        json = response.json()
        if "OK" not in json["status"]:
            print(f"ERR: {json["what"]}")

    if committed:
        _perform_request(
            endpoint="http://localhost:5520/commit",
            payload={
                "path": path,
                "commit": committed
            })

    if fire:
        _perform_request(
            endpoint="http://localhost:5520/fire",
            payload={
                "path": path,
                "fire": fire
            })

    if close_count:
        _perform_request(
            endpoint="http://localhost:5520/commit/close-count",
            payload={
                "path": path,
                "count": close_count
            })

    if directory_n_file_expected:
        _perform_request(
            endpoint="http://localhost:5520/commit/file-count",
            payload={
                "path": path,
                "count": directory_n_file_expected
            })

    if is_directory is not None:
        _perform_request(
            endpoint="http://localhost:5520/directory",
            payload={
                "path": path,
                "directory": is_directory
            }
        )

    if is_permanent is not None:
        _perform_request(
            endpoint="http://localhost:5520/permanent",
            payload={
                "path": path,
                "permanent": is_permanent
            }
        )
    if is_excluded is not None:
        _perform_request(
            endpoint="http://localhost:5520/exclude",
            payload={
                "path": path,
                "excluded": is_excluded
            }
        )

    if producers:
        for producer in producers:
            _perform_request(
                endpoint="http://localhost:5520/producer",
                payload={
                    "path": path,
                    "producer": producer
                }
            )

    if consumers:
        for consumer in consumers:
            _perform_request(
                endpoint="http://localhost:5520/consumer",
                payload={
                    "path": path,
                    "consumer": consumer
                }
            )

    if file_dependencies:
        for dependency in file_dependencies:
            _perform_request(
                endpoint="http://localhost:5520/dependency",
                payload={
                    "path": path,
                    "dependency": dependency
                }
            )

    def _capiocl_rule(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        return wrapper

    return _capiocl_rule
