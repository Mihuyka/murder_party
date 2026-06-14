import telebot
import game
from game import players, addPlayer, addUser, removePlayer
from config import BOT_TOKEN
import murder_trivia_pb2
# TODO: добавить исключения для всего
# Запуск Telegram бота
bot = telebot.TeleBot(BOT_TOKEN)

@bot.message_handler(commands=['start'])
def start(message):
    user_id = message.chat.id
    status = addUser(user_id)
    if status:
        bot.send_message(user_id, "Вы успешно зарегистрированы!") # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, "Вы уже зарегистрированы!") # TODO: Поменять сообщение

@bot.callback_query_handler(func=lambda call: call.data == 'participating')
def participating_btn(call):
    user_id = call.message.chat.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        addPlayer(user_id)
        bot.send_message(user_id, 'Вы подписали контракт с @Mihuyka на участие в смертельной вечеринке :)') # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, 'Игра уже началассь. Что поделаешь...') # TODO: Поменять сообщение

@bot.callback_query_handler(func=lambda call: call.data == 'not_participating')
def not_participating_btn(call):
    user_id = call.message.chat.id

    bot.answer_callback_query(call.id)
    if not game.game_started:
        removePlayer(user_id)
        bot.send_message(user_id, 'Вы разорвали контракт смертельной вечеринки :(') # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, 'Игра уже началась. Что поделаешь...') # TODO: Поменять сообщение

def set_player_answer(user_id, answer_letter):
    if user_id in game.players.players:
        enum_value = getattr(murder_trivia_pb2.Answer, answer_letter, murder_trivia_pb2.Answer.UNSPECIFIED)
        game.players.players[user_id].answer = enum_value
        bot.send_message(user_id, f"Вариант ответа {answer_letter} учтен") # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, "Вы не в игре!") # TODO: Поменять сообщение

@bot.callback_query_handler(func=lambda call: call.data == 'answerA')
def btn_answer_A(call):
    set_player_answer(call.message.chat.id, 'A')

@bot.callback_query_handler(func=lambda call: call.data == 'answerB')
def btn_answer_B(call):
    set_player_answer(call.message.chat.id, 'B')

@bot.callback_query_handler(func=lambda call: call.data == 'answerC')
def btn_answer_C(call):
    set_player_answer(call.message.chat.id, 'C')

@bot.callback_query_handler(func=lambda call: call.data == 'answerD')
def btn_answer_D(call):
    set_player_answer(call.message.chat.id, 'D')