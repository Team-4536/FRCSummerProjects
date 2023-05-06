
from typing import Any




class widget:

    def __init__(self) -> None:
        raise NotImplementedError()

    def tick(self, data: dict[str, Any]) -> None:
        raise NotImplementedError()