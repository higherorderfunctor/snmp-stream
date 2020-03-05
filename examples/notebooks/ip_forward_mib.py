# This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

from pkg_resources import parse_version
import kaitaistruct
from kaitaistruct import KaitaiStruct, KaitaiStream, BytesIO


if parse_version(kaitaistruct.__version__) < parse_version('0.9'):
    raise Exception("Incompatible Kaitai Struct Python API: 0.9 or later is required, but you have %s" % (kaitaistruct.__version__))

class IpForwardMib(KaitaiStruct):
    def __init__(self, _io, _parent=None, _root=None):
        self._io = _io
        self._parent = _parent
        self._root = _root if _root else self
        self._read()

    def _read(self):
        self._raw_header = self._io.read_bytes(16)
        _io__raw_header = KaitaiStream(BytesIO(self._raw_header))
        self.header = self._root.Info(_io__raw_header, self, self._root)
        self.body = self._root.Body(self._io, self, self._root)

    class Info(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            self.sys_size = self._io.read_u1()
            self.suboid_size = self._io.read_u1()
            self.endianess = self._io.read_u1()


    class Body(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            _on = self._root.header.endianess
            if _on == 0:
                self._is_le = True
            elif _on == 1:
                self._is_le = False
            if not hasattr(self, '_is_le'):
                raise kaitaistruct.UndecidedEndiannessError("/types/body")
            elif self._is_le == True:
                self._read_le()
            elif self._is_le == False:
                self._read_be()

        def _read_le(self):
            self.metadata = self._root.Body.Metadata(self._io, self, self._root, self._is_le)
            self.root_oids = self._root.Body.RootOids(self._io, self, self._root, self._is_le)
            self.var_binds = self._root.Body.VarBinds(self._io, self, self._root, self._is_le)

        def _read_be(self):
            self.metadata = self._root.Body.Metadata(self._io, self, self._root, self._is_le)
            self.root_oids = self._root.Body.RootOids(self._io, self, self._root, self._is_le)
            self.var_binds = self._root.Body.VarBinds(self._io, self, self._root, self._is_le)

        class VarBind(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/var_bind")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.data = self._io.read_bytes(self.len.as_int)

            def _read_be(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.data = self._io.read_bytes(self.len.as_int)


        class SizeT(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/size_t")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                _on = self._root.header.sys_size
                if _on == 2:
                    self.as_int = self._io.read_u2le()
                elif _on == 4:
                    self.as_int = self._io.read_u4le()
                elif _on == 8:
                    self.as_int = self._io.read_u8le()

            def _read_be(self):
                _on = self._root.header.sys_size
                if _on == 2:
                    self.as_int = self._io.read_u2be()
                elif _on == 4:
                    self.as_int = self._io.read_u4be()
                elif _on == 8:
                    self.as_int = self._io.read_u8be()


        class Oid(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/oid")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self._raw_suboids = self._io.read_bytes(self.len.as_int // self._root.header.suboid_size)
                _io__raw_suboids = KaitaiStream(BytesIO(self._raw_suboids))
                self.suboids = self._root.Body.Suboids(_io__raw_suboids, self, self._root, self._is_le)
                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)

            def _read_be(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self._raw_suboids = self._io.read_bytes(self.len.as_int // self._root.header.suboid_size)
                _io__raw_suboids = KaitaiStream(BytesIO(self._raw_suboids))
                self.suboids = self._root.Body.Suboids(_io__raw_suboids, self, self._root, self._is_le)
                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)


        class RootOids(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/root_oids")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.oid = [None] * (self.len.as_int)
                for i in range(self.len.as_int):
                    self.oid[i] = self._root.Body.Oid(self._io, self, self._root, self._is_le)

                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)

            def _read_be(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.oid = [None] * (self.len.as_int)
                for i in range(self.len.as_int):
                    self.oid[i] = self._root.Body.Oid(self._io, self, self._root, self._is_le)

                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)


        class Suboids(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/suboids")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.suboid = []
                i = 0
                while not self._io.is_eof():
                    self.suboid.append(self._root.Body.OidT(self._io, self, self._root, self._is_le))
                    i += 1


            def _read_be(self):
                self.suboid = []
                i = 0
                while not self._io.is_eof():
                    self.suboid.append(self._root.Body.OidT(self._io, self, self._root, self._is_le))
                    i += 1



        class VarBinds(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/var_binds")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.var_bind = []
                i = 0
                while not self._io.is_eof():
                    self.var_bind.append(self._root.Body.VarBind(self._io, self, self._root, self._is_le))
                    i += 1


            def _read_be(self):
                self.var_bind = []
                i = 0
                while not self._io.is_eof():
                    self.var_bind.append(self._root.Body.VarBind(self._io, self, self._root, self._is_le))
                    i += 1



        class Metadata(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/metadata")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.value = (self._io.read_bytes(self.len.as_int)).decode(u"utf-8")
                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)

            def _read_be(self):
                self.len = self._root.Body.SizeT(self._io, self, self._root, self._is_le)
                self.value = (self._io.read_bytes(self.len.as_int)).decode(u"utf-8")
                self.padding = self._root.Body.Aligned(self._io, self, self._root, self._is_le)


        class OidT(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/oid_t")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                _on = self._root.header.suboid_size
                if _on == 2:
                    self.as_int = self._io.read_u2le()
                elif _on == 4:
                    self.as_int = self._io.read_u4le()
                elif _on == 8:
                    self.as_int = self._io.read_u8le()

            def _read_be(self):
                _on = self._root.header.suboid_size
                if _on == 2:
                    self.as_int = self._io.read_u2be()
                elif _on == 4:
                    self.as_int = self._io.read_u4be()
                elif _on == 8:
                    self.as_int = self._io.read_u8be()


        class Aligned(KaitaiStruct):
            def __init__(self, _io, _parent=None, _root=None, _is_le=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._is_le = _is_le
                self._read()

            def _read(self):
                if not hasattr(self, '_is_le'):
                    raise kaitaistruct.UndecidedEndiannessError("/types/body/types/aligned")
                elif self._is_le == True:
                    self._read_le()
                elif self._is_le == False:
                    self._read_be()

            def _read_le(self):
                self.padding = self._io.read_bytes(((self._root.header.sys_size - self._io.pos()) % self._root.header.sys_size))

            def _read_be(self):
                self.padding = self._io.read_bytes(((self._root.header.sys_size - self._io.pos()) % self._root.header.sys_size))




