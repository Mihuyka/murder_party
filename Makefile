VENV_BIN = .venv/bin
PYTHON   = $(VENV_BIN)/python3
PROTOC   = $(VENV_BIN)/python3 -m grpc_tools.protoc

PROTO_GEN = telegram_bot/src/murder_trivia_pb2.py telegram_bot/src/murder_trivia_pb2_grpc.py

.PHONY: run

all: run

$(PROTO_GEN): proto/murder_trivia.proto
	$(PROTOC) -Iproto --python_out=telegram_bot/src --pyi_out=telegram_bot/src --grpc_python_out=telegram_bot/src proto/murder_trivia.proto

run: telegram_bot/src/server.py $(PROTO_GEN)
	$(PYTHON) telegram_bot/src/server.py