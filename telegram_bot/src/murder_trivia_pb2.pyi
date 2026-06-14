from google.protobuf import empty_pb2 as _empty_pb2
from google.protobuf.internal import containers as _containers
from google.protobuf.internal import enum_type_wrapper as _enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from collections.abc import Mapping as _Mapping
from typing import ClassVar as _ClassVar, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class Answer(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    A: _ClassVar[Answer]
    B: _ClassVar[Answer]
    C: _ClassVar[Answer]
    D: _ClassVar[Answer]
    UNSPECIFIED: _ClassVar[Answer]

class MinigameType(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    GAME_UNSPECIFIED: _ClassVar[MinigameType]
    GAME_WIRES: _ClassVar[MinigameType]
    GAME_POISONED_GLASS: _ClassVar[MinigameType]
    GAME_MINESWEEPER: _ClassVar[MinigameType]
A: Answer
B: Answer
C: Answer
D: Answer
UNSPECIFIED: Answer
GAME_UNSPECIFIED: MinigameType
GAME_WIRES: MinigameType
GAME_POISONED_GLASS: MinigameType
GAME_MINESWEEPER: MinigameType

class PlayerInfo(_message.Message):
    __slots__ = ("chat_id", "answer", "minigame")
    CHAT_ID_FIELD_NUMBER: _ClassVar[int]
    ANSWER_FIELD_NUMBER: _ClassVar[int]
    MINIGAME_FIELD_NUMBER: _ClassVar[int]
    chat_id: int
    answer: Answer
    minigame: MinigameType
    def __init__(self, chat_id: _Optional[int] = ..., answer: _Optional[_Union[Answer, str]] = ..., minigame: _Optional[_Union[MinigameType, str]] = ...) -> None: ...

class PlayerList(_message.Message):
    __slots__ = ("players",)
    class PlayersEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: int
        value: PlayerInfo
        def __init__(self, key: _Optional[int] = ..., value: _Optional[_Union[PlayerInfo, _Mapping]] = ...) -> None: ...
    PLAYERS_FIELD_NUMBER: _ClassVar[int]
    players: _containers.MessageMap[int, PlayerInfo]
    def __init__(self, players: _Optional[_Mapping[int, PlayerInfo]] = ...) -> None: ...

class CorrectAnswer(_message.Message):
    __slots__ = ("correct_answer",)
    CORRECT_ANSWER_FIELD_NUMBER: _ClassVar[int]
    correct_answer: Answer
    def __init__(self, correct_answer: _Optional[_Union[Answer, str]] = ...) -> None: ...

class MinigameRequest(_message.Message):
    __slots__ = ("chat_id", "game_type")
    CHAT_ID_FIELD_NUMBER: _ClassVar[int]
    GAME_TYPE_FIELD_NUMBER: _ClassVar[int]
    chat_id: int
    game_type: MinigameType
    def __init__(self, chat_id: _Optional[int] = ..., game_type: _Optional[_Union[MinigameType, str]] = ...) -> None: ...
