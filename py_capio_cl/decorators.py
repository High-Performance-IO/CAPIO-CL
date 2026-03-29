from functools import wraps

import requests


def _capio_internal_coordinator(func):
    """
    Manages the execution of registered decorator tasks, ensuring that IF @CapioContext is being used, it is allways
    executed first so that a CAPIO server instance is present to receive the requests.

    It works by checking the priority assigned to each decorator tasks.
    - @CapioCLRule has a priority of 10
    - @CapioContext has a priority of 20
    Highest priority is executed first. This enables to give order also internally to which @CapioCLRule needs to
    be executed first
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        tasks = getattr(wrapper, "_exec_tasks", [])

        rules = [t for p, t in tasks if p == 10]  # CapioCLRules
        context_task = [t for p, t in tasks if p == 20]  # CapioContext

        if len(context_task) > 1:
            raise RuntimeError("ERROR: @CapioContext has been wrapped multiple times!")

        context_task = context_task[0] if len(context_task) > 0 else None

        # Execute all CapioClRules
        for rule_logic in rules:
            rule_logic()

        # if CapioContext exists, execute it otherwise return the original function
        if context_task:
            return context_task(func, *args, **kwargs)

        return func(*args, **kwargs)

    wrapper._exec_tasks = []
    return wrapper


def _ensure_internal_coordinator(func):
    # Avoid wrapping _capio_internal_coordinator multiple times
    if hasattr(func, "_exec_tasks"):
        return func
    return _capio_internal_coordinator(func)


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

    def _capiocl_rule(func):
        target = _ensure_internal_coordinator(func)

        # Wrap the CAPIO-CL request logic into a callback that is going to be called later by the internal coordinator
        def _capiocl_rule_callback():
            def _perform_request(endpoint, payload=None):
                response = requests.post(endpoint, json=payload, headers={"content-type": "application/json"})
                res_json = response.json()
                if "OK" not in res_json.get("status", ""):
                    print(f"ERR: {res_json['what']}" if "what" in res_json else "ERR: no error message!")

            if committed:
                _perform_request("http://localhost:5520/commit", {"path": path, "commit": committed})
            if fire:
                _perform_request("http://localhost:5520/fire", {"path": path, "fire": fire})
            if close_count:
                _perform_request("http://localhost:5520/commit/close-count", {"path": path, "count": close_count})
            if directory_n_file_expected:
                _perform_request("http://localhost:5520/commit/file-count",
                                 {"path": path, "count": directory_n_file_expected})
            if is_directory is not None:
                _perform_request("http://localhost:5520/directory", {"path": path, "directory": is_directory})
            if is_permanent is not None:
                _perform_request("http://localhost:5520/permanent", {"path": path, "permanent": is_permanent})
            if is_excluded is not None:
                _perform_request("http://localhost:5520/exclude", {"path": path, "excluded": is_excluded})

            if producers:
                for p in producers:
                    _perform_request("http://localhost:5520/producer", {"path": path, "producer": p})
            if consumers:
                for c in consumers:
                    _perform_request("http://localhost:5520/consumer", {"path": path, "consumer": c})
            if file_dependencies:
                for d in file_dependencies:
                    _perform_request("http://localhost:5520/dependency", {"path": path, "dependency": d})

        # Add the request callback with low priority
        target._exec_tasks.append((10, _capiocl_rule_callback))

        return target

    return _capiocl_rule
