from typing import Callable




PROC_FIRST: int =       0
PROC_PROF: int =        1
PROC_CTRL: int =        2
PROC_HARDWARE: int =    3
PROC_LAST: int =        4

Process = tuple[int, Callable[[], None]]


