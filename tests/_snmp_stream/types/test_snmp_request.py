"""SnmpRequest test cases."""

import pickle

import hypothesis

from snmp_stream._snmp_stream import SnmpRequest
from .strategies import snmp_requests


@hypothesis.given(
    snmp_request=snmp_requests()  # type: ignore
)
def test_pickle(
        snmp_request: SnmpRequest
) -> None:
    """Test pickling an SnmpRequest."""
    assert isinstance(snmp_request, SnmpRequest)
    other: SnmpRequest = pickle.loads(pickle.dumps(snmp_request))
    test = snmp_request == other
    assert test
