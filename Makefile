proto:
	python -m grpc_tools.protoc -Iproto --python_out=telegram_bot/src --pyi_out=telegram_bot/src --grpc_python_out=telegram_bot/src 'proto/murder_trivia.proto'
