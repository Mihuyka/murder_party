from concurrent import futures # Нужно почитать что это
import grpc
import murder_trivia_pb2_grpc
import triviabot.rpc as rpc
import triviabot

game_started = False

def serve():
    # Запуск gRPC сервера
    port = "50051"
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    murder_trivia_pb2_grpc.add_BotRPCServicer_to_server(rpc.BotRPC(), server)
    server.add_insecure_port("[::]:" + port)
    server.start()
    print("gRPC Сервер запущен, слушаю на " + port)
    server.wait_for_termination()

    # Блокирует поток Main, до тех пор пока пользоватлеь не завершит программу самостоятельно
    triviabot.bot.infinity_polling() 

if __name__ == "__main__":
    serve()