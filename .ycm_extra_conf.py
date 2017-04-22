import sys
import ycm_sys_conf as conf
from os.path import expanduser

homeDir = expanduser("~")

#Default for project is C files
conf.flags += ['-std=c++11', '-x', 'c++']
conf.flags += [
'-Wall',
'-Wextra',
'-I.',
'-Iinclude',
'-I../include',
'-I../lib/yaml-cpp/include',
'-Ilib/yaml-cpp/include',
]

FlagsForFile = conf.FlagsForFile
