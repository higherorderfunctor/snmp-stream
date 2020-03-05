"""Build script for the C extension."""

import os
import platform
import subprocess
import sys
from typing import Any, MutableMapping, Text

from setuptools import Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    # pylint: disable=too-few-public-methods
    """Cmake extension."""

    def __init__(self, name: Text, sourcedir: Text = '') -> None:
        """Init the extension."""
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


extensions = [  # pylint: disable=invalid-name
    CMakeExtension('snmp_stream._snmp_stream'),
]


class CMakeBuild(build_ext):  # type: ignore
    """Cmake builder."""

    def run(self) -> None:
        """Run the cmake build."""
        try:
            # out = subprocess.check_output(['cmake', '--version'])
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                'CMake must be installed to build the following extensions: ' +
                ', '.join(e.name for e in self.extensions)
            )

        if platform.system() == 'Windows':
            raise RuntimeError('Windows is not a supported platform')

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: Extension) -> None:
        """Build the extension."""
        #extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [
            #'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable
        ]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        #if platform.system() == 'Windows':
        #    cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
        #    if sys.maxsize > 2**32:
        #        cmake_args += ['-A', 'x64']
        #    build_args += ['--', '/m']
        #else:
        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            # build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''), self.distribution.get_version()
        )
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)
        subprocess.check_call(['cmake', '--install', '.'], cwd=self.build_temp)


def build(setup_kwargs: MutableMapping[Text, Any]) -> None:
    """Build the cmake extension."""
    setup_kwargs.update({
        'ext_modules': extensions,
        'cmdclass': {
            'build_ext': CMakeBuild
        }
    })
