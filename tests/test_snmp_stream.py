"""Module test cases."""

from snmp_stream import __version__


def test_version() -> None:
    """Test the version."""
    assert __version__ == '0.1.0'
