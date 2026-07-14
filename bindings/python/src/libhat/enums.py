from enum import Enum, Flag, auto


class Protection(Flag):
    Read = auto()
    Write = auto()
    Execute = auto()


class ScanAlignment(Enum):
    X1 = 1
    X4 = 4
    X16 = 16


class ScanHint(Flag):
    X86_64 = auto()
    PAIR0 = auto()
    AARCH64 = auto()
