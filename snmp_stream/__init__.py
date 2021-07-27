"""Snmp-stream."""

from typing import Mapping, Optional, Sequence, Text, Tuple, TypedDict, Union

import snmp_stream._snmp_stream as snmp

__version__ = '0.1.0'


ObjectIdentityType = Union['ObjectIdentity', snmp.ObjectIdentity, Sequence[int], Text]


class ObjectIdentity(snmp.ObjectIdentity):
    # pylint: disable=too-few-public-methods
    """Object identity.

    :param oid: Optional OID supporting multiple formats.
    """

    def __init__(self, oid: Optional[ObjectIdentityType] = None) -> None:
        """Initialize the object identity.

        :param oid: Optional OID supporting multiple formats.
        """
        if isinstance(oid, Text):
            super().__init__([int(x) for x in (oid[1:] if oid[0] == '.' else oid).split('.')])
        elif oid is None:
            super().__init__()
        else:
            super().__init__(list(oid))


def to_object_identity(oid: Optional[ObjectIdentityType]) -> ObjectIdentity:
    """Map various OID type representations to an :class:ObjectIdentity."""
    if isinstance(oid, ObjectIdentity):
        return oid
    if isinstance(oid, Text):
        return ObjectIdentity([int(suboid) for suboid in oid.split('.') if suboid != ''])
    return ObjectIdentity(oid if oid is not None else [])


ObjectIdentityRangeType = Union[
    snmp.ObjectIdentityRange,
    Optional[ObjectIdentityType],
    Tuple[Optional[ObjectIdentityType], Optional[ObjectIdentityType]]
]


def to_object_identity_range(oid_range: ObjectIdentityRangeType) -> snmp.ObjectIdentityRange:
    """Map various OID range type representations to an :class:ObjectIdentity."""
    if isinstance(oid_range, snmp.ObjectIdentityRange):
        return oid_range
    if isinstance(oid_range, tuple):
        return snmp.ObjectIdentityRange(
            to_object_identity(oid_range[0]), to_object_identity(oid_range[1])
        )
    return snmp.ObjectIdentityRange(
        to_object_identity(oid_range), to_object_identity(oid_range)
    )


VersionType = Union[Tuple[Text, Union[snmp.Community.Version, Text]]]


def to_version(version: Union[snmp.Community.Version, Text]) -> snmp.Community.Version:
    """Map various SNMP version type representations to an :class:Version."""
    if isinstance(version, snmp.Community.Version):
        return version
    versions: Mapping[Text, snmp.Community.Version] = {
        'V1': snmp.Community.Version.V1,
        'V2C': snmp.Community.Version.V2C
    }
    return versions[version]


CommunityType = Union[snmp.Community, VersionType]

ConfigMapping = TypedDict('ConfigMapping', {
    'retries': Optional[int],
    'timeout': Optional[int],
    'max_response_var_binds_per_pdu': Optional[int],
    'max_async_sessions': Optional[int]
}, total=False)

ConfigType = Union[
    snmp.Config, ConfigMapping
]

SnmpRequestMapping = TypedDict('SnmpRequestMapping', {
    'SnmpRequest': snmp.SnmpRequest.SnmpRequestType,
    'host': Text,
    'community': CommunityType,
    'oids': Sequence[ObjectIdentityType],
    'ranges': Optional[Sequence[snmp.ObjectIdentityRange]],
    'req_id': Optional[Text],
    'config': Optional[ConfigType]
}, total=False)

SnmpRequestType = Union[
    snmp.SnmpRequest, Mapping[
        Text, Union[snmp.SnmpRequest.SnmpRequestType]]
]


def get(
        host: Text,
        community: Union[snmp.Community, Tuple[Text, Union[snmp.Community.Version, Text]]],
        oids: Sequence[ObjectIdentityType],
        ranges: Optional[Sequence[ObjectIdentityRangeType]] = None,
        req_id: Optional[Text] = None,
        config: Optional[Union[snmp.Config, Mapping[Text, Optional[int]]]] = None
) -> Optional[snmp.SnmpResponse]:
    # pylint: disable=too-many-arguments
    """Perform SNMP get request."""
    session = snmp.SessionManager()
    session.add_request(snmp.SnmpRequest(
        snmp.SnmpRequest.SnmpRequestType.GET_REQUEST,
        host,
        community if isinstance(community, snmp.Community)
        else snmp.Community(community[0], to_version(community[1])),
        [to_object_identity(oid) for oid in oids],
        [to_object_identity_range(oid_range) for oid_range in ranges] if ranges is not None
        else None,
        req_id,
        config if isinstance(config, snmp.Config) or config is None else snmp.Config(**config)
    ))
    response = session.run()
    return response[0] if response is not None else None


def walk(
        host: Text,
        community: Union[snmp.Community, Tuple[Text, Union[snmp.Community.Version, Text]]],
        oids: Sequence[ObjectIdentityType],
        ranges: Optional[Sequence[ObjectIdentityRangeType]] = None,
        req_id: Optional[Text] = None,
        config: Optional[Union[snmp.Config, Mapping[Text, Optional[int]]]] = None
) -> Optional[snmp.SnmpResponse]:
    # pylint: disable=too-many-arguments
    """Perform SNMP walk request."""
    session = snmp.SessionManager()
    session.add_request(snmp.SnmpRequest(
        snmp.SnmpRequest.SnmpRequestType.WALK_REQUEST,
        host,
        community if isinstance(community, snmp.Community)
        else snmp.Community(community[0], to_version(community[1])),
        [to_object_identity(oid) for oid in oids],
        [to_object_identity_range(oid_range) for oid_range in ranges] if ranges is not None
        else None,
        req_id,
        config if isinstance(config, snmp.Config) or config is None else snmp.Config(**config)
    ))
    response = session.run()
    return response[0] if response is not None else None
