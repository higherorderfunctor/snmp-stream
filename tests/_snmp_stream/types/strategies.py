"""Hypothesis search strategies."""

from functools import partial
from typing import Optional, Sequence, Text, TypeVar, Union

import hypothesis.strategies as st

from snmp_stream._snmp_stream import (
    Community, Config, ObjectIdentity, ObjectIdentityRange, SnmpError, SnmpRequest,
    test_ambiguous_root_oids
)
from tests.strategies import int64s, optionals, uint64s

T = TypeVar('T')


def oids(
        min_size: int = 0,
        max_size: int = 128,
        prefix: Optional[Sequence[int]] = None
) -> st.SearchStrategy[ObjectIdentity]:
    """Generate an ObjectIdentity."""
    def create_object_identity(
            oids: Sequence[int],
            text: Optional[Text] = None
    ) -> ObjectIdentity:
        _oids: Union[Sequence[int], Text]
        if prefix is not None:
            _oids = [*prefix, *oids]
        else:
            _oids = oids
        if text:
            _oids = text+'.'.join(map(str, _oids))
        return ObjectIdentity(oids)

    prefix_len = 0
    if prefix is not None:
        prefix_len = len(prefix)

    return st.one_of([
        st.builds(  # type: ignore
            create_object_identity,
            st.lists(  # type: ignore
                uint64s(),
                min_size=min_size, max_size=max_size - prefix_len
            )
        ),
        st.builds(  # type: ignore
            partial(create_object_identity, text=''),  # type: ignore
            st.lists(  # type: ignore
                uint64s(),
                min_size=min_size, max_size=max_size - prefix_len
            )
        ),
        st.builds(  # type: ignore
            partial(create_object_identity, text='.'),  # type: ignore
            st.lists(  # type: ignore
                uint64s(),
                min_size=min_size, max_size=max_size - prefix_len
            )
        )
    ])


def oid_ranges(
    start: st.SearchStrategy[Optional[ObjectIdentity]] = optionals(oids()),
    stop: st.SearchStrategy[Optional[ObjectIdentity]] = optionals(oids())
) -> st.SearchStrategy[ObjectIdentityRange]:
    """Generate an ObjectIdentityRange."""
    def create_object_identity_range(
            start: Optional[ObjectIdentity],
            stop: Optional[ObjectIdentity]
    ) -> ObjectIdentityRange:
        if start and stop and stop < start:
            return ObjectIdentityRange(stop, start)
        return ObjectIdentityRange(start, stop)
    return st.builds(create_object_identity_range, start, stop)  # type: ignore


def versions() -> st.SearchStrategy[Community.Version]:
    """Generate a Version."""
    return st.one_of([  # type: ignore
        st.just(Community.Version.V1),  # type: ignore
        st.just(Community.Version.V2C)  # type: ignore
    ])


def communities(
    string: st.SearchStrategy[Text] = st.text(),
    version: st.SearchStrategy[Community.Version] = versions()
) -> st.SearchStrategy[Community]:
    """Generate a Community."""
    return st.builds(  # type: ignore
        Community, string, version
    )


def configs(
    retries: st.SearchStrategy[Optional[int]] = optionals(int64s(min_value=0)),
    timeout: st.SearchStrategy[Optional[int]] = optionals(int64s(min_value=0)),
    max_response_var_binds_per_pdu: st.SearchStrategy[Optional[int]] = optionals(uint64s()),
    max_async_sessions: st.SearchStrategy[Optional[int]] = optionals(uint64s(min_value=1))
) -> st.SearchStrategy[Config]:
    """Generate a Config."""
    return st.builds(  # type: ignore
        Config, retries, timeout, max_response_var_binds_per_pdu, max_async_sessions
    )


def snmp_request_types() -> st.SearchStrategy[SnmpRequest.SnmpRequestType]:
    """Generate an SnmpRequestType."""
    return st.one_of([  # type: ignore
        st.just(SnmpRequest.SnmpRequestType.GET_REQUEST),  # type: ignore
        st.just(SnmpRequest.SnmpRequestType.WALK_REQUEST)  # type: ignore
    ])


def snmp_requests(
        type: st.SearchStrategy[SnmpRequest.SnmpRequestType] = snmp_request_types(),
        host: st.SearchStrategy[Text] = st.text(),
        communities: st.SearchStrategy[Sequence[Community]] = st.lists(communities(), min_size=1),
        oids: st.SearchStrategy[Sequence[ObjectIdentity]] = (
            st.lists(
                oids(), min_size=1
            )
            .filter(lambda x: test_ambiguous_root_oids(x) is None)
        ),
        ranges: st.SearchStrategy[Optional[Sequence[ObjectIdentityRange]]] = optionals(
            st.lists(oid_ranges())
        ),
        config: st.SearchStrategy[Optional[Config]] = optionals(configs()),
) -> st.SearchStrategy[SnmpRequest]:
    """Generate an SnmpRequest."""
    return st.builds(  # type: ignore
        SnmpRequest, type, host, communities, oids, ranges, config
    )


def snmp_error_types() -> st.SearchStrategy[SnmpError.SnmpErrorType]:
    """Generate an SnmpErrorType."""
    return st.one_of([  # type: ignore
        st.just(SnmpError.SnmpErrorType.SESSION_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.CREATE_REQUEST_PDU_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.SEND_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.BAD_RESPONSE_PDU_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.TIMEOUT_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.ASYNC_PROBE_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.TRANSPORT_DISCONNECT_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.CREATE_RESPONSE_PDU_ERROR),  # type: ignore
        st.just(SnmpError.SnmpErrorType.VALUE_WARNING)  # type: ignore
    ])


def snmp_errors(
        type:  st.SearchStrategy[SnmpError.SnmpErrorType] = snmp_error_types(),
        request:  st.SearchStrategy[SnmpRequest] = snmp_requests(),
        sys_errno:  st.SearchStrategy[Optional[int]] = optionals(int64s()),
        snmp_errno:  st.SearchStrategy[Optional[int]] = optionals(int64s()),
        err_stat:  st.SearchStrategy[Optional[int]] = optionals(int64s()),
        err_index:  st.SearchStrategy[Optional[int]] = optionals(int64s()),
        err_oid: st.SearchStrategy[Optional[ObjectIdentity]] = optionals(oids()),
        message:    st.SearchStrategy[Optional[Text]] = optionals(st.text())
) -> st.SearchStrategy[SnmpError]:
    """Generate an SnmpError."""
    return st.builds(  # type: ignore
        SnmpError, type, request, sys_errno, snmp_errno, err_stat, err_index, err_oid, message
    )
