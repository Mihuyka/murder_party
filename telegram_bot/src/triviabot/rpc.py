import telebot
import murder_trivia_pb2_grpc
import murder_trivia_pb2
from triviabot import bot
from game import users, game_started, players

class BotRPC(murder_trivia_pb2_grpc.BotRPCServicer):
    def startRegistration(self, request, context):
        keyboard = telebot.types.InlineKeyboardMarkup()
        button_participating = telebot.types.InlineKeyboardButton(text="Участвую", callback_data='participating')
        button_not_participating = telebot.types.InlineKeyboardButton(text="Не участвую", callback_data='not_participating')
        keyboard.row(button_participating, button_not_participating)

        game_started = False

        for userID in users:
            bot.send_message(userID, "Желаете принять участие в игре?", reply_markup=keyboard) # TODO: Поменять сообщение
            
    def receiveRegisteredPlayers(self, request, context):
        pass

    def startGame():
        game_started = True
        for uid in players.keys():
            playerList = "Игра началась :) В вечеринке участвуют: " # TODO: Поменять сообщение
            for uid in players.keys():
                layerList += "@" + bot.get_chat(uid).username + " "

        bot.send_message(uid, playerList)
    def sendNewQuestion(self, request, context):
        if game_started == False:
            super().startGame()
        keyboard = telebot.types.InlineKeyboardMarkup()
        Abtn = telebot.types.InlineKeyboardButton(text="A", callback_data='answerA')
        Bbtn = telebot.types.InlineKeyboardButton(text="B", callback_data='answerB')
        Cbtn = telebot.types.InlineKeyboardButton(text="C", callback_data='answerC')
        Dbtn = telebot.types.InlineKeyboardButton(text="D", callback_data='answerD')
        keyboard.row(Abtn, Bbtn, Cbtn, Dbtn)

        for userID in users:
            bot.send_message(userID, "Выберите один из варинатов ответа: ", reply_markup=keyboard)
    
    def receivePlayerAnswers(self, request, context):
        response = murder_trivia_pb2.PlayerList
        response[:] = players
            

    def correctAnswerWas(self, request, context):
        for uid in players.keys():
            if players[uid] == request.correct_answer:
                bot.send_message(uid, "Вы угадали :), правильный ответ был: ") # TODO: Поменять сообщение
            else:
                bot.send_message(uid, "Вы не угадали :(, правильный ответ был: ") # TODO: Поменять сообщение
    
    def assignMinigame(self, request, context):
        userID = request.chat_id
        match(request.game_type):
            case murder_trivia_pb2.GAME_UNSPECIFIED:
                pass
            case murder_trivia_pb2.GAME_WIRES:
                pass
            case murder_trivia_pb2.GAME_POISONED_GLASS:
                pass
            case murder_trivia_pb2.GAME_MINESWEEPER:
                pass
    
    def sendYoureDead(self, request, context):
        bot.send_message(request.chat_id, "Вы сдохли!") # TODO: Поменять сообщение
    
    def sendYoureAlive(self, request, context):
        bot.send_message(request.chat_id, "Вы живы!") # TODO: Поменять сообщение
    
    def showWinners(self, request, context):
        pass