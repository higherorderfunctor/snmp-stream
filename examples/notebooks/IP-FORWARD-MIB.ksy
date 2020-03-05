meta:
  id: ip_forward_mib
seq:
  - id: header
    type: info
    size: 16
  - id: body
    type: body
types:
  info:
    seq:
      - id: sys_size
        type: u1
      - id: suboid_size
        type: u1
      - id: endianess
        type: u1
  body:
    meta:
      endian:
        switch-on: _root.header.endianess
        cases:
          0: le
          1: be
    seq:
      - id: metadata
        type: metadata
      - id: root_oids
        type: root_oids
      #- id: var_binds
      #  type: var_binds
    types:
      size_t:
        seq:
          - id: as_int
            type: 
              switch-on: _root.header.sys_size
              cases:
                2: u2
                4: u4
                8: u8
      oid_t:
        seq:
          - id: as_int
            type: 
              switch-on: _root.header.suboid_size
              cases:
                2: u2
                4: u4
                8: u8
      aligned:
        seq:
          - id: padding
            size: (_root.header.sys_size - _io.pos) % _root.header.sys_size      
      metadata:
        seq:
          - id: len
            type: size_t
          - id: value
            type: str
            encoding: utf-8
            size: len.as_int
          - id: padding
            type: aligned
      suboids:
        seq:
          - id: suboid
            type: oid_t
            repeat: eos
      oid:
        seq:
          - id: len
            type: size_t
          - id: suboids
            type: suboids
            size: len.as_int / _root.header.suboid_size
          - id: padding
            type: aligned
      root_oids:
        seq:
          - id: len
            type: size_t
          - id: oid
            type: oid
            repeat: expr
            repeat-expr: len.as_int
          - id: padding
            type: aligned
      #var_bind:
      #  seq:
      #    - id: len
      #      type: size_t
      #    - id: data
      #      size: len.as_int
      #var_binds:
      #   seq:
      #     - id: var_bind
      #       type: var_bind
      #       repeat: eos