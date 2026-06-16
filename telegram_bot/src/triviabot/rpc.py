import murder_trivia_pb2_grpc
import murder_trivia_pb2
from google.protobuf import empty_pb2
import telebot
import triviabot.game as game
from triviabot.triviabot import bot

class BotRPC(murder_trivia_pb2_grpc.BotRPCServicer):
    def startRegistration(self, request, context):
        keyboard = telebot.types.InlineKeyboardMarkup()
        button_participating = telebot.types.InlineKeyboardButton(text="Участвую", callback_data='participating')
        button_not_participating = telebot.types.InlineKeyboardButton(text="Не участвую", callback_data='not_participating')
        keyboard.row(button_participating, button_not_participating)

        game.game_started = False
        game.registration_started = True

        for chat_id in game.users:
            bot.send_message(chat_id, "Готовы ли вы обменять свою душу?", reply_markup=keyboard)
        return empty_pb2.Empty()
            
    def receiveRegisteredPlayers(self, request, context):
        return game.players

    def startGame(self):
        game.game_started = True
        game.registration_started = True
        player_list = "Геноцид начался! Далее жертвы: " 
        for chat_id in game.players.players:
            player_list += f"@{bot.get_chat(chat_id).username} "
        for chat_id in game.players.players:       
            bot.send_message(chat_id, player_list)
        return empty_pb2.Empty()

    def sendNewQuestion(self, request, context):
        if game.game_started == False:
            self.startGame()
        keyboard = telebot.types.InlineKeyboardMarkup()
        Abtn = telebot.types.InlineKeyboardButton(text="A", callback_data='answerA')
        Bbtn = telebot.types.InlineKeyboardButton(text="B", callback_data='answerB')
        Cbtn = telebot.types.InlineKeyboardButton(text="C", callback_data='answerC')
        Dbtn = telebot.types.InlineKeyboardButton(text="D", callback_data='answerD')
        keyboard.row(Abtn, Bbtn, Cbtn, Dbtn)

        for chat_id in game.players.players:
            bot.send_message(chat_id, "Выберите один из варинатов ответа: ", reply_markup=keyboard)
        return empty_pb2.Empty()
    
    def receivePlayerAnswers(self, request, context):
        return game.players

    def correctAnswerWas(self, request, context):
        for chat_id in game.players.players:
            if game.players.players[chat_id] == request.correct_answer:
                bot.send_message(chat_id, "Вы угадали :), правильный ответ был: ") # TODO: Поменять сообщение
            else:
                bot.send_message(chat_id, "Неверно :), правильный ответ был: ") 
        return empty_pb2.Empty()
    
    def assignMinigame(self, request, context):
        chat_id = request.chat_id
        game.players.players[chat_id].minigame = request.game_type
        match(request.game_type):
            case murder_trivia_pb2.GAME_UNSPECIFIED:  # TODO: Сделать миниигры
                pass
            case murder_trivia_pb2.GAME_WIRES:
                pass
            case murder_trivia_pb2.GAME_POISONED_GLASS:
                pass
            case murder_trivia_pb2.GAME_MINESWEEPER:
                pass
        return empty_pb2.Empty()
    
    def sendYoureDead(self, request, context):
        bot.send_message(request.chat_id, "Вы сдохли!") # TODO: Поменять сообщение
        return empty_pb2.Empty()
    
    def sendYoureAlive(self, request, context):
        bot.send_message(request.chat_id, "Вы живы!") # TODO: Поменять сообщение
        return empty_pb2.Empty()
    
    def showWinners(self, request, context):
        game.game_started = False
        game.registration_started = False
        winners_message = "Победители:\n"
        for i, chat_id in enumerate(request.players, start=1):
            chat = bot.get_chat(chat_id)
            if chat.username == None:
                winners_message += f"{i}. {chat.first_name}\n"
            else:
                winners_message += f"{i}. @{chat.username}\n"
        for chat_id in game.players.players:       
            bot.send_message(chat_id, winners_message)
        return empty_pb2.Empty()