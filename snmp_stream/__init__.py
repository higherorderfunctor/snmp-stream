"""Snmp-stream."""

import json
from collections import abc
from typing import Mapping, Optional, Sequence, Text, Tuple, TypedDict, Union

import snmp_stream._snmp_stream as snmp

__version__ = '0.1.0'


ObjectIdentityBaseType = Union['ObjectIdentity', snmp.ObjectIdentity, Sequence[int], Text]
ObjectIdentityMapping = TypedDict('ObjectIdentityMapping', {
    'oid': ObjectIdentityBaseType
})
ObjectIdentityMapping.__doc__ = """:class:`ObjectIdentity` encoded as a :class:`TypedDict`.

.. highlight:: python
.. code-block:: python

   ObjectIdentityBaseType = Union['ObjectIdentity', snmp.ObjectIdentity, Sequence[int], Text]
   ObjectIdentityMapping = TypedDict('ObjectIdentityMapping', {
       'oid': ObjectIdentityBaseType
   })
"""
ObjectIdentityType = Union[ObjectIdentityBaseType, ObjectIdentityMapping]


class ObjectIdentity(snmp.ObjectIdentity):
    """Object identity.

    :param oid: Optional OID supporting multiple formats.
    """

    def __init__(self, oid: Optional[ObjectIdentityType] = None) -> None:
        """Initialize the object identity.

        :param oid: Optional OID supporting multiple formats.
        """
        if isinstance(oid, Text):
            super().__init__([int(x) for x in (oid[1:] if oid[0] == '.' else oid).split('.')])
        elif isinstance(oid, abc.Mapping):
            super().__init__(list(ObjectIdentity(oid['oid'])))
        elif oid is None:
            super().__init__()
        else:
            super().__init__(list(oid))


class ObjectIdentityEncoder(json.JSONEncoder):
    """OID JSON encoder."""

    def default(self, obj: ObjectIdentityType) -> Text:
        """Encode an OID."""
        return {  # type: ignore
            'oid': str(ObjectIdentity(obj))
        }


class ObjectIdentityDecoder(json.JSONDecoder):
    """OID JSON encoder."""
    def __init__(self, *args, **kwargs):
        json.JSONDecoder.__init__(self, object_hook=self.object_hook, *args, **kwargs)

    def object_hook(self, obj: ObjectIdentityType) -> ObjectIdentity:
        """Encode an OID."""
        return ObjectIdentity(obj)








class ObjectIdentityRangePoint(snmp.ObjectIdentityRange):
    def __init__(self, point: ObjectIdentityType):
        pass

ObjectIdentityRangeMapping = TypedDict('ObjectIdentityRangeMapping', {
    'start': Optional[ObjectIdentityType],
    'stop': Optional[ObjectIdentityType]
})
ObjectIdentityRangeType = Union[
    snmp.ObjectIdentityRange,
    ObjectIdentityRangeMapping,
    Optional[ObjectIdentityType],
    Tuple[Optional[ObjectIdentityType], Optional[ObjectIdentityType]]
]
VersionType = Union[Tuple[Text, Union[snmp.Community.Version, Text]]]
# community mapping
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
        Text,Union[snmp.SnmpRequest.SnmpRequestType]]
]

def to_object_identity(oid: Optional[ObjectIdentityType]) -> ObjectIdentity:
    """Map various OID type representations to an :class:ObjectIdentity."""
    if isinstance(oid, ObjectIdentity):
        return oid
    if isinstance(oid, Text):
        return ObjectIdentity([int(suboid) for suboid in oid.split('.') if suboid != ''])
    return ObjectIdentity(oid if oid is not None else [])


def to_object_identity_range(oid_range: ObjectIdentityRangeType) -> snmp.ObjectIdentityRange:
    """Map various OID range type representations to an :class:ObjectIdentity."""
    if isinstance(oid_range, snmp.ObjectIdentityRange):
        return oid_range
    if (
        isinstance(oid_range, ObjectIdentity) or
        isinstance(oid_range, Text) or
        isinstance(oid_range, abc.Sequence) or
        oid_range is None
    ):
        return snmp.ObjectIdentityRange(to_object_identity(oid_range), to_object_identity(oid_range))
    return snmp.ObjectIdentityRange(to_object_identity(oid_range[0]), to_object_identity(oid_range[1]))

def to_version(version: Union[snmp.Community.Version, Text]) -> snmp.Community.Version:
    if isinstance(version, snmp.Community.Version):
        return version
    versions = {
        'V1': snmp.Community.V1,
        'V2C': snmp.Community.V2C
    }
    return versions[version]

def to_community(community: CommunityType) -> snmp.Community:
    pass


def to_oid(oid: Optional[ObjectIdentityType]) -> snmp.ObjectIdentity:
    if isinstance(oid, snmp.ObjectIdentity):
        return oid
    return ObjectIdentity(oid)

def to_range(oid_range: ObjectIdentityType) -> snmp.ObjectIdentityRange:
    if isinstance(oid_range, snmp.ObjectIdentityRange):
        return oid_range
    if isinstance(oid_range, snmp.ObjectIdentity) or isinstance(oid_range, snmp.ObjectIdentity) or isinstance(oid_range, Text) or oid is None:
        return snmp.ObjectIdentityRange(to_oid(oid_range), to_oid(oid_range))
    return snmp.ObjectIdentityRange(to_oid(oid_range[0]), to_oid(oid_range[1]))

def get(
        host: Text,
        community: Union[snmp.Community, Tuple[Text, Union[snmp.Community.Version, Text]]],
        oids: Sequence[ObjectIdentityType],
        ranges: Sequence[Union[snmp.ObjectIdentityRange, ObjectIdentityType, Tuple[ObjectIdentityType, ObjectIdentityType]]] = None,
        req_id: Optional[Text] = None,
        config: Optional[Union[snmp.Config, Mapping[Text, Optional[int]]]] = None
) -> snmp.SnmpResponse:
    sm = snmp.SessionManager()
    sm.add_request(snmp.SnmpRequest(
        snmp.SnmpRequest.GET_REQUEST,
        host,
        community if isinstance(community, snmp.Community) else snmp.Community(community[0], to_version(community[1])),
        [to_oid(oid) for oid in oids],
        [to_range(oid_range) for oid_range in ranges] if ranges is not None else None,
        req_id,
        config if isinstance(config, snmp.Config) or config is None else snmp.Config(**config)
    ))
    return sm.run()[0]

def walk(
        host: Text,
        community: Union[snmp.Community, Tuple[Text, Union[snmp.Community.Version, Text]]],
        oids: Sequence[ObjectIdentityType],
        ranges: Sequence[Union[snmp.ObjectIdentityRange, ObjectIdentityType, Tuple[ObjectIdentityType, ObjectIdentityType]]] = None,
        req_id: Optional[Text] = None,
        config: Optional[Union[snmp.Config, Mapping[Text, Optional[int]]]] = None
) -> snmp.SnmpResponse:
    sm = snmp.SessionManager()
    sm.add_request(snmp.SnmpRequest(
        snmp.SnmpRequest.WALK_REQUEST,
        host,
        community if isinstance(community, snmp.Community) else snmp.Community(community[0], to_version(community[1])),
        [to_oid(oid) for oid in oids],
        [to_range(oid_range) for oid_range in ranges] if ranges is not None else None,
        req_id,
        config if isinstance(config, snmp.Config) or config is None else snmp.Config(**config)
    ))
    return sm.run()[0]
