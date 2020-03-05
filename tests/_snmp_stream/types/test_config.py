"""Config test cases."""

import pickle

import hypothesis

from snmp_stream._snmp_stream import Config
from .strategies import configs


@hypothesis.given(
    config=configs()  # type: ignore
)
def test_pickle(
        config: Config
) -> None:
    """Test pickling a Config."""
    assert isinstance(config, Config)
    other: Config = pickle.loads(pickle.dumps(config))
    assert config == other
