import telebot
import triviabot.game as game
from triviabot.game import players, addPlayer, addUser, removePlayer
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
    if game.registration_started:
        if not game.game_started:
            addPlayer(user_id)
            bot.send_message(user_id, 'Вы подписали контракт с @Mihuyka на участие в смертельной вечеринке :)') 
        else:
            bot.send_message(user_id, 'Игра уже началась, не тыкай, а то пальцы отрежем') 
    else:
        bot.send_message(user_id, 'Запись на верную смерть ещё не началась!') 

@bot.callback_query_handler(func=lambda call: call.data == 'not_participating')
def not_participating_btn(call):
    user_id = call.message.chat.id

    bot.answer_callback_query(call.id)
    if game.registration_started:
        if not game.game_started:
            removePlayer(user_id)
            bot.send_message(user_id, 'Вы разорвали контракт смертельной вечеринки :(') 
        else:
            bot.send_message(user_id, 'Игра уже началась, не тыкай, а то пальцы отрежем') 
    else:
        bot.send_message(user_id, 'Кто ты, странник, и что ты тут делаешь?')

def set_player_answer(user_id, answer_letter):
    if game.game_started:
        if user_id in game.players.players:
            try:
                enum_value = murder_trivia_pb2.Answer.Value(answer_letter)
                game.players.players[user_id].answer = enum_value
                bot.send_message(user_id, f"Вариант ответа {answer_letter} учтен") # TODO: Поменять сообщение
            except ValueError:
                enum_value = murder_trivia_pb2.Answer.UNSPECIFIED
                bot.send_message(user_id, "Такого ответа не существует!") # TODO: Поменять сообщение
        else:
            bot.send_message(user_id, "Убери свои руки, я тебя не знаю") 
    else:
        bot.send_message(user_id, "Веселье ещё впереди :)") 

@bot.callback_query_handler(func=lambda call: call.data.startswith('answer'))
def btn_answer_handler(call):
    answer_letter = call.data.replace('answer', '')
    user_id = call.from_user.id
    set_player_answer(user_id, answer_letter)
    bot.answer_callback_query(call.id)