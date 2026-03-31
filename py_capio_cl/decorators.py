import json
import socket
from functools import wraps

from py_capio_cl import CAPIO_CL_DEFAULT_WF_NAME, DEFAULT_MCAST_GROUP
from py_capio_cl import CapioCLEntry


def CapioCLRule(path: str,
                workflow_name: str = CAPIO_CL_DEFAULT_WF_NAME,
                committed: str | None = None,
                fire: str | None = None,
                close_count: int | None = None,
                directory_n_file_expected: int | None = None,
                is_directory: bool | None = None,
                is_permanent: bool | None = None,
                is_excluded: bool | None = None,
                producers: list[str] | None = None,
                consumers: list[str] | None = None,
                file_dependencies: list[str] | None = None,
                multicast_group: tuple[str, int] = DEFAULT_MCAST_GROUP
                ):
    if not path:
        raise RuntimeError("ERROR: cannot specify a CAPIO-CL rule without setting a path!")

    rule = CapioCLEntry()
    if committed:
        rule.commit_rule = committed
    if fire:
        rule.fire_rule = fire

    if close_count:
        rule.commit_on_close_count = close_count

    if directory_n_file_expected:
        rule.directory_children_count = directory_n_file_expected

    if is_directory:
        rule.is_file = not is_directory

    if is_permanent:
        rule.permanent = is_permanent

    if is_excluded:
        rule.excluded = is_excluded

    if producers:
        rule.producers = producers

    if consumers:
        rule.consumers = consumers

    if file_dependencies:
        rule.file_dependencies = file_dependencies

    request_body = dict()
    request_body["path"] = path
    request_body["workflow_name"] = workflow_name
    request_body["CapioClEntry"] = json.loads(rule.to_json())

    message = json.dumps(request_body).encode('utf-8')
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    ttl = 1  # no broadcast outside current subnet
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)

    try:
        sock.sendto(message, multicast_group)
        print(f"[[py_capio_cl]] Sent bcast message for path: {path}")
    finally:
        sock.close()

    def _capiocl_rule(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)

        return wrapper

    return _capiocl_rule
