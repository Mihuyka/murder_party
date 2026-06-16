import murder_trivia_pb2_grpc
import murder_trivia_pb2
from google.protobuf import empty_pb2
import telebot
import triviabot.game as game
from triviabot.triviabot import bot
from triviabot.game import IMAGE_PATH
import logging

class BotRPC(murder_trivia_pb2_grpc.BotRPCServicer):

    def startRegistration(self, request, context):
        keyboard = telebot.types.InlineKeyboardMarkup()
        button_participating = telebot.types.InlineKeyboardButton(text="Участвую", callback_data='participating')
        button_not_participating = telebot.types.InlineKeyboardButton(text="Не участвую", callback_data='not_participating')
        keyboard.row(button_participating, button_not_participating)

        game.game_started = False
        game.registration_started = True

        for chat_id in game.users:
            try:
                bot.send_message(chat_id, "Готовы ли вы обменять свою душу?", reply_markup=keyboard)
            except Exception as e:
                logging.error(f"Не удалось отправить сообщение {chat_id}: {e}")
        return empty_pb2.Empty()

    def receiveRegisteredPlayers(self, request, context):
        return game.players

    def startGame(self):
        game.game_started = True
        game.registration_started = True
        player_list = "Геноцид начался! Далее жертвы: " 
        for chat_id in game.players.players:
            try:
                username = bot.get_chat(chat_id).username
                player_list += f"@{username} " if username else f"{chat_id} "
            except Exception:
                player_list += f"{chat_id} "
        for chat_id in game.players.players:
            try:
                bot.send_message(chat_id, player_list)
            except Exception as e:
                logging.error(f"Ошибка отправки сообщения о начале игры {chat_id}: {e}")
        return empty_pb2.Empty()

    def sendNewQuestion(self, request, context):
        if not game.game_started:
            self.startGame()
        keyboard = telebot.types.InlineKeyboardMarkup()
        Abtn = telebot.types.InlineKeyboardButton(text="🅰️", callback_data='answerA')
        Bbtn = telebot.types.InlineKeyboardButton(text="🅱️", callback_data='answerB')
        Cbtn = telebot.types.InlineKeyboardButton(text="C", callback_data='answerC')
        Dbtn = telebot.types.InlineKeyboardButton(text="D", callback_data='answerD')
        keyboard.row(Abtn, Bbtn, Cbtn, Dbtn)

        question = f"Выберите один из вариантов ответа:\n A) {request.A}\nB) {request.B}\nC) {request.C}\nD) {request.D}"
        for chat_id in game.players.players:
            game.players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
            try:
                bot.send_message(chat_id, question, reply_markup=keyboard)
            except Exception as e:
                logging.error(f"Ошибка отправки вопроса {chat_id}: {e}")
        return empty_pb2.Empty()

    def receivePlayerAnswers(self, request, context):
        return game.players

    def correctAnswerWas(self, request, context):
        correct = request.correct_answer
        for chat_id, player in game.players.players.items():
            if player.answer == correct:
                try:
                    bot.send_message(chat_id, "Вы угадали :)")
                except Exception as e:
                    logging.error(f"Ошибка отправки поздравления {chat_id}: {e}")
            else:
                try:
                    bot.send_message(chat_id, f"Неверно :(, правильный ответ был: {murder_trivia_pb2.Answer.Name(correct)}")
                except Exception as e:
                    logging.error(f"Ошибка отправки уведомления {chat_id}: {e}")
        return empty_pb2.Empty()

    def assignMinigame(self, request, context):
        chat_id = request.chat_id
        if chat_id not in game.players.players:
            return empty_pb2.Empty()  # игрок не участвует

        game.players.players[chat_id].minigame = request.game_type
        game.players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED

        match request.game_type:
            case murder_trivia_pb2.GAME_UNSPECIFIED:
                try:
                    bot.send_message(chat_id, "Молодец... Маленький, молодец. Так уж и быть, пока живи, наблюдай как остальные умирают. НО НЕ РАССЛАБЛЯЙСЯ!1!!! До тебя очередь ещё дойдёт... ахпвахахахаххах")
                except Exception as e:
                    logging.error(f"Ошибка отправки сообщения GAME_UNSPECIFIED {chat_id}: {e}")
            case murder_trivia_pb2.GAME_WIRES:
                self.wiresMinigame(chat_id)
            case murder_trivia_pb2.GAME_POISONED_GLASS:
                self.poisonedGlassMinigame(chat_id)
            case murder_trivia_pb2.GAME_MINESWEEPER:
                self.minesweeperMinigame(chat_id)
        return empty_pb2.Empty()

    def wiresMinigame(self, chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            blue_wire = telebot.types.InlineKeyboardButton(text="🟦 Синий провод", callback_data='blue_wire')
            red_wire = telebot.types.InlineKeyboardButton(text="🔴 Красный провод", callback_data='red_wire')
            keyboard.row(red_wire, blue_wire)

            with open(IMAGE_PATH / "wires.webp", 'rb') as wires:
                bot.send_photo(chat_id, wires, caption="Ну что поиграем) Вы работаете электриком в небольшой деревушке под Соколом. Перед вами 2 кабеля: фаза и земля, но какой из них фаза, а какой земля вы не знаете. Бабок и никого рядом нету( Выберите кабель:")
            # После фото отправляем сообщение с клавиатурой (иначе клавиатура не прикрепится)
            bot.send_message(chat_id, "Выберите провод:", reply_markup=keyboard)
        except FileNotFoundError:
            logging.error(f"Файл wires.webp не найден по пути {IMAGE_PATH / 'wires.webp'}")
            bot.send_message(chat_id, "Изображение не найдено, но игра продолжается. Выберите провод:", reply_markup=keyboard)
        except Exception as e:
            logging.error(f"Ошибка в wiresMinigame для {chat_id}: {e}")
            bot.send_message(chat_id, "Произошла ошибка при загрузке мини-игры. Попробуйте позже.")

    def poisonedGlassMinigame(self, chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            glass1 = telebot.types.InlineKeyboardButton(text="🍷 Подозрительно прозрачный бокал", callback_data='glass1')
            glass2 = telebot.types.InlineKeyboardButton(text="🍸 Светло-зелёный бокал", callback_data='glass2')
            glass3 = telebot.types.InlineKeyboardButton(text="🥃 Небольшая рюмка", callback_data='glass3')
            keyboard.row(glass1, glass2, glass3)

            with open(IMAGE_PATH / "poisoned_glass.jpg", 'rb') as f:
                bot.send_photo(chat_id, f, caption="Ну что поиграем) В гостях у белочки вам предложили выпить, но есть нюанс, в один из бокалов белочка налила снотворное. Пейте, на здоровье!")
            bot.send_message(chat_id, "Выберите бокал:", reply_markup=keyboard)
        except FileNotFoundError:
            logging.error(f"Файл poisoned_glass.jpg не найден по пути {IMAGE_PATH / 'poisoned_glass.jpg'}")
            bot.send_message(chat_id, "Изображение не найдено, но игра продолжается. Выберите бокал:", reply_markup=keyboard)
        except Exception as e:
            logging.error(f"Ошибка в poisonedGlassMinigame для {chat_id}: {e}")
            bot.send_message(chat_id, "Произошла ошибка при загрузке мини-игры.")

    def minesweeperMinigame(self, chat_id):
        try:
            keyboard = telebot.types.InlineKeyboardMarkup()
            left = telebot.types.InlineKeyboardButton(text="⬅️ Налево", callback_data='left')
            right = telebot.types.InlineKeyboardButton(text="➡️ Направо", callback_data='right')
            keyboard.row(left, right)

            with open(IMAGE_PATH / "minesweeper.jpg", 'rb') as f:
                bot.send_photo(chat_id, f, caption="Ну что поиграем) Вам предложили неплохую подработку на лето: разминировать целое минное поле. Пока вы живы, у вас есть выбор сначала пройтись по правой или левой тропинке. Выбирайте с умом!")
            bot.send_message(chat_id, "Куда пойдёте?", reply_markup=keyboard)
        except FileNotFoundError:
            logging.error(f"Файл minesweeper.jpg не найден по пути {IMAGE_PATH / 'minesweeper.jpg'}")
            bot.send_message(chat_id, "Изображение не найдено, но игра продолжается. Выберите направление:", reply_markup=keyboard)
        except Exception as e:
            logging.error(f"Ошибка в minesweeperMinigame для {chat_id}: {e}")
            bot.send_message(chat_id, "Произошла ошибка при загрузке мини-игры.")

    def sendYoureDead(self, request, context):
        try:
            bot.send_message(request.chat_id, "Вы сдохли! Но на этом игра не заканчивается")
        except Exception as e:
            logging.error(f"Ошибка sendYoureDead для {request.chat_id}: {e}")
        return empty_pb2.Empty()

    def sendYoureAlive(self, request, context):
        try:
            bot.send_message(request.chat_id, "Блин, вы выжили :(")
        except Exception as e:
            logging.error(f"Ошибка sendYoureAlive для {request.chat_id}: {e}")
        return empty_pb2.Empty()

    def showWinners(self, request, context):
        game.game_started = False
        game.registration_started = False
        winners_message = "Победители:\n"
        for i, chat_id in enumerate(request.players, start=1):
            try:
                chat = bot.get_chat(chat_id)
                name = f"@{chat.username}" if chat.username else chat.first_name
                winners_message += f"{i}. {name}\n"
            except Exception:
                winners_message += f"{i}. {chat_id}\n"
        for chat_id in game.players.players:
            try:
                bot.send_message(chat_id, winners_message)
            except Exception as e:
                logging.error(f"Ошибка отправки победителей {chat_id}: {e}")
        return empty_pb2.Empty()