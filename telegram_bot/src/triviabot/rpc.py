import murder_trivia_pb2_grpc
import murder_trivia_pb2
from google.protobuf import empty_pb2
import telebot
import triviabot.game as game
from triviabot.triviabot import bot
from triviabot.game import IMAGE_PATH

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
        Abtn = telebot.types.InlineKeyboardButton(text="🅰️", callback_data='answerA')
        Bbtn = telebot.types.InlineKeyboardButton(text="🅱️", callback_data='answerB')
        Cbtn = telebot.types.InlineKeyboardButton(text="C", callback_data='answerC')
        Dbtn = telebot.types.InlineKeyboardButton(text="D", callback_data='answerD')
        keyboard.row(Abtn, Bbtn, Cbtn, Dbtn)

        for chat_id in game.players.players:
            game.players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
            bot.send_message(chat_id, "Выберите один из варинатов ответа: ", reply_markup=keyboard)
        return empty_pb2.Empty()
    
    def receivePlayerAnswers(self, request, context):
        return game.players

    def correctAnswerWas(self, request, context):
        for chat_id in game.players.players:
            if game.players.players[chat_id] == request.correct_answer:
                bot.send_message(chat_id, "Вы угадали :)")
            else:
                bot.send_message(chat_id, "Неверно :), правильный ответ был: ") 
        return empty_pb2.Empty()
    
    def assignMinigame(self, request, context):
        chat_id = request.chat_id
        game.players.players[chat_id].minigame = request.game_type
        game.players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
        match(request.game_type):
            case murder_trivia_pb2.GAME_UNSPECIFIED:
                bot.send_message(chat_id, "Молодец... Маленький, молодец. Так уж и быть, пока живи, наблюдай как остальные умирают. НО НЕ РАССЛАБЛЯЙСЯ!1!!! До тебя очередь ещё дойдёт очередь... ахпвахахахаххах", reply_markup=keyboard)
            case murder_trivia_pb2.GAME_WIRES:
                self.wiresMinigame(chat_id)
            case murder_trivia_pb2.GAME_POISONED_GLASS:
                self.poisonedGlassMinigame(chat_id)
            case murder_trivia_pb2.GAME_MINESWEEPER:
                self.minesweeperMinigame(chat_id)
        return empty_pb2.Empty()
    
    def wiresMinigame(chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            blue_wire = telebot.types.InlineKeyboardButton(text="🟦 Синий провод", callback_data='blue_wire')
            red_wire = telebot.types.InlineKeyboardButton(text="🔴 Красный провод", callback_data='red_wire')
            keyboard.row(red_wire, blue_wire)
            with open(IMAGE_PATH / "wires.webp", 'rb') as wires:
                bot.send_photo(chat_id, wires, caption="Ну что поиграем) Вы работате электриком в небольшой деревушке под Соколом. Перед вами 2 кабеля: фаза и земля, но какой из них фаза, а какой земля вы не знаете. Бабки и не кого рядом нету( Выберите кабель:")
        except FileNotFoundError:
            print(f"Ошибка: Файл не найден по пути:\n{IMAGE_PATH.resolve() / "wires.webp"}")
    
    def poisonedGlassMinigame(chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            glass1 = telebot.types.InlineKeyboardButton(text="🍷 Подозрительно прозрачный бокал", callback_data='glass1')
            glass2 = telebot.types.InlineKeyboardButton(text="🍸 Светло-зелёный бокал", callback_data='glass2')
            glass3 = telebot.types.InlineKeyboardButton(text="🥃 Небольшая рюмка", callback_data='glass3')
            keyboard.row(glass1, glass2, glass3)
            with open(IMAGE_PATH / "poisoned_glass.jpg", 'rb') as wires:
                bot.send_photo(chat_id, wires, caption="Ну что поиграем) В гостях у белочки вам предложили выпить, но есть нюанс, в один из бокалов белочка налила снатворное. Пейте, на здоровье!")
        except FileNotFoundError:
            print(f"Ошибка: Файл не найден по пути:\n{IMAGE_PATH.resolve() / "wires.webp"}")

    def minesweeperMinigame(chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            left = telebot.types.InlineKeyboardButton(text="⬅️ Налево", callback_data='left')
            right = telebot.types.InlineKeyboardButton(text="➡️ Направо", callback_data='right')
            keyboard.row(left, right)
            with open(IMAGE_PATH / "wires.webp", 'rb') as wires:
                bot.send_photo(chat_id, wires, caption="Ну что поиграем) Вам предложили неплохую подработку на лето: разминировать целое минное поле. Пока вы живи, у вас есть выбор сначла пройтись по правой или левой тропинке. Выбирайте с умом!")
        except FileNotFoundError:
            print(f"Ошибка: Файл не найден по пути:\n{IMAGE_PATH.resolve() / "wires.webp"}")
    
    def sendYoureDead(self, request, context):
        bot.send_message(request.chat_id, "Вы сдохли! Но на этом игра не заканчивается") 
        return empty_pb2.Empty()
    
    def sendYoureAlive(self, request, context):
        bot.send_message(request.chat_id, "Блин, вы выжили :(") 
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