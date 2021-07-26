"""SnmpError test cases."""

import pickle

import hypothesis

from snmp_stream._snmp_stream import SnmpError
from .strategies import snmp_errors


@hypothesis.given(
    snmp_error=snmp_errors()  # type: ignore
)
def test_pickle(
        snmp_error: SnmpError
) -> None:
    """Test pickling an SnmpError."""
    assert isinstance(snmp_error, SnmpError)
    other: SnmpError = pickle.loads(pickle.dumps(snmp_error))
    assert snmp_error == other
