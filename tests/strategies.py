"""Hypothesis search strategies."""

from typing import Optional, TypeVar

import hypothesis.strategies as st

T = TypeVar('T')  # pylint: disable=invalid-name


def optionals(strategy: st.SearchStrategy[T]) -> st.SearchStrategy[Optional[T]]:
    """Generate an optional strategy."""
    return st.one_of(
        st.none(), strategy
    )


def int64s(
        min_value: int = -(2 ** 63),
        max_value: int = (2 ** 63) - 1
) -> st.SearchStrategy[int]:
    """Generate an int64."""
    if min_value > max_value:
        raise TypeError('min_value must be less than or equal max_value.')
    if min_value < -(2 ** 63):
        raise TypeError(f'min_value must be greater than or equal to {-(2 ** 63)}.')
    if min_value > (2 ** 63) - 1:
        raise TypeError(f'min_value must be less than or equal to {(2 ** 63) - 1}.')
    if max_value < -(2 ** 63):
        raise TypeError(f'max_value must be greater than or equal to {-(2 ** 63)}.')
    if max_value > (2 ** 63) - 1:
        raise TypeError(f'max_value must be less than or equal to {(2 ** 63) - 1}.')
    return st.integers(
        min_value,
        max_value
    )


def uint64s(
        min_value: int = 0,
        max_value: int = (2 ** 64) - 1
) -> st.SearchStrategy[int]:
    """Generate a uint64."""
    if min_value > max_value:
        raise TypeError('min_value must be less than or equal to max_value.')
    if min_value < 0:
        raise TypeError('min_value must be greater than or equal to 0.')
    if min_value > (2 ** 64) - 1:
        raise TypeError(f'min_value must be less than or equal to {(2 ** 64) - 1}.')
    if max_value < 0:
        raise TypeError('max_value must be greater than or equal to 0.')
    if max_value > (2 ** 64) - 1:
        raise TypeError(f'max_value must be less than or equal to {(2 ** 64) - 1}.')
    return st.integers(
        min_value,
        max_value
    )
