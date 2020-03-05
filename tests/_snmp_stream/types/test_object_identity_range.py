"""ObjectIdentity test cases."""

import pickle
from typing import Optional

import hypothesis
import pytest

from snmp_stream._snmp_stream import ObjectIdentity, ObjectIdentityRange
from tests.strategies import optionals
from .strategies import oids


@hypothesis.given(
    start=optionals(oids()),  # type: ignore
    stop=optionals(oids())  # type: ignore
)
def test_pickle(
        start: Optional[ObjectIdentity],
        stop: Optional[ObjectIdentity]
) -> None:
    """Test pickling an ObjectIdentityRange."""
    if start and stop and start > stop:
        with pytest.raises(ValueError):  # type: ignore
            ObjectIdentityRange(start, stop)
    else:
        oid_range = ObjectIdentityRange(start, stop)
        other: ObjectIdentityRange = pickle.loads(pickle.dumps(oid_range))
        assert oid_range == other

# @hypothesis.given(
#     oid=oids()  # type: ignore
# )
# def test_len(
#         oid: ObjectIdentity
# ) -> None:
#     """Test .__len__()."""
#     assert len(list(oid)) == len(oid)
#     assert len(list(iter(oid))) == len(oid)
#
#
# @hypothesis.given(
#     oid=oids()  # type: ignore
# )
# def test_str(
#         oid: ObjectIdentity
# ) -> None:
#     """Test .__str__()."""
#     if len(oid) == 0:
#         assert str(oid) == ''
#     else:
#         assert str(oid) == (
#             '.'+'.'.join(map(str, list(oid)))
#         )
#
#
# @hypothesis.given(
#     oid=oids()  # type: ignore
# )
# def test_repr(
#         oid: ObjectIdentity
# ) -> None:
#     """Test .__repr__()."""
#     if len(oid) == 0:
#         assert str(oid) == ''
#     else:
#         assert str(oid) == (
#             '.'+'.'.join(map(str, list(oid)))
#         )
