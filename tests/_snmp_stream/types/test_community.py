"""Community test cases."""

import pickle

import hypothesis

from snmp_stream._snmp_stream import Community
from .strategies import communities


@hypothesis.given(
    community=communities()  # type: ignore
)
def test_pickle(
        community: Community
) -> None:
    """Test pickling a Community."""
    assert isinstance(community, Community)
    other: Community = pickle.loads(pickle.dumps(community))
    assert community == other
