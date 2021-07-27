"""ObjectIdentity test cases."""

import pickle

import hypothesis

from snmp_stream._snmp_stream import ObjectIdentity
from .strategies import oids


@hypothesis.given(
    oid=oids()  # type: ignore
)
def test_pickle(
        oid: ObjectIdentity
) -> None:
    """Test pickling an ObjectIdentity."""
    assert isinstance(oid, ObjectIdentity)
    other: ObjectIdentity = pickle.loads(pickle.dumps(oid))
    assert oid == other


@hypothesis.given(
    oid=oids()  # type: ignore
)
def test_len(
        oid: ObjectIdentity
) -> None:
    """Test .__len__()."""
    assert len(list(oid)) == len(oid)
    assert len(list(iter(oid))) == len(oid)


@hypothesis.given(
    oid=oids()  # type: ignore
)
def test_str(
        oid: ObjectIdentity
) -> None:
    """Test .__str__()."""
    if len(oid) == 0:
        assert str(oid) == ''
    else:
        assert str(oid) == (
            '.'+'.'.join(map(str, list(oid)))
        )


@hypothesis.given(
    oid=oids()  # type: ignore
)
def test_repr(
        oid: ObjectIdentity
) -> None:
    """Test .__repr__()."""
    if len(oid) == 0:
        assert repr(oid) == "ObjectIdentity('')"
    else:
        assert repr(oid) == (
            "ObjectIdentity('."+'.'.join(map(str, list(oid))) + "')"
        )
