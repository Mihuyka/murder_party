import telebot
import triviabot.game as game
from triviabot.game import players, addPlayer, addUser, removePlayer
from config import BOT_TOKEN
import murder_trivia_pb2
import logging

# Запуск Telegram бота
bot = telebot.TeleBot(BOT_TOKEN)

@bot.message_handler(commands=['start'])
def start(message):
    user_id = message.chat.id
    status = addUser(user_id)
    if status:
        bot.send_message(user_id, "Добро пожаловать в смертельную вечеринку! Данный бот позволяет вам боротся за свою душу в игре, посредством Телеграмм бота! Когда сбор в игру начнётся, вам обязательно придёт повестка!")
    else: 
        bot.send_message(user_id, "Думаешь я про тебя забыл? Ага конечно, я уже давно за тобой наблюдаю. А, ты забыл для чего я предназначен?")
        bot.send_message(user_id, "Я бот, который позволяет вам взаимодействовать с игрой, с помощью Телеграмма! Когда игра начнётся, я пришлю вам контракт. Так что ждите...")        

@bot.callback_query_handler(func=lambda call: call.data == 'participating')
def participating_btn(call):
    user_id = call.message.chat.id
    bot.answer_callback_query(call.id)
    try:
        if game.registration_started and not game.game_started:
            addPlayer(user_id)
            bot.send_message(user_id, 'Вы подписали контракт с @Mihuyka на участие в смертельной вечеринке :)')
        elif game.game_started:
            bot.send_message(user_id, 'Игра уже началась, не тыкай, а то пальцы отрежем')
        else:
            bot.send_message(user_id, 'Запись на верную смерть ещё не началась!')
    except Exception as e:
        logging.error(f"Ошибка в participating_btn для {user_id}: {e}")
        bot.send_message(user_id, "Произошла ошибка, попробуйте позже.")

@bot.callback_query_handler(func=lambda call: call.data == 'not_participating')
def not_participating_btn(call):
    user_id = call.message.chat.id
    bot.answer_callback_query(call.id)
    try:
        if game.registration_started and not game.game_started:
            if user_id in game.players.players:
                removePlayer(user_id)
                bot.send_message(user_id, 'Вы разорвали контракт смертельной вечеринки :(')
            else:
                bot.send_message(user_id, 'Вы и так не участвуете.')
        elif game.game_started:
            bot.send_message(user_id, 'Игра уже началась, не тыкай, а то пальцы отрежем')
        else:
            bot.send_message(user_id, 'Кто ты, странник, и что ты тут делаешь?')
    except Exception as e:
        logging.error(f"Ошибка в not_participating_btn для {user_id}: {e}")
        bot.send_message(user_id, "Произошла ошибка, попробуйте позже.")

@bot.callback_query_handler(func=lambda call: call.data.startswith('answer'))
def answer_handler(call):
    user_id = call.from_user.id
    answer_letter = call.data.replace('answer', '')
    bot.answer_callback_query(call.id)
    
    if not game.game_started:
        bot.send_message(user_id, "Веселье ещё впереди :)")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Убери свои руки, я тебя не знаю")
        return

    try:
        enum_value = murder_trivia_pb2.Answer.Value(answer_letter)
        game.players.players[user_id].answer = enum_value
        bot.send_message(user_id, f"Вариант ответа {answer_letter} учтён. Вы точно уверены в своём ответе?")
    except ValueError:
        bot.send_message(user_id, "Такого ответа не существует!")
    except Exception as e:
        logging.error(f"Ошибка в answer_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'red_wire')
def red_wire_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
        bot.send_message(user_id, "Вы выбрали 🔴 красный провод. Вы точно уверены, что это не \"фаза\"?")
    except Exception as e:
        logging.error(f"Ошибка в red_wire_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'blue_wire')
def blue_wire_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
        bot.send_message(user_id, "Вы выбрали 🟦 Синий провод. Вы думаете это \"земля\"?")
    except Exception as e:
        logging.error(f"Ошибка в blue_wire_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'left')
def left_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
        bot.send_message(user_id, "Вы решили пойти ⬅️ налево. Вы точно уверены в своем ответе?")
    except Exception as e:
        logging.error(f"Ошибка в left_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'right')
def right_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
        bot.send_message(user_id, "Вы решили пойти ➡️ направо. Там случайно нет мины?")
    except Exception as e:
        logging.error(f"Ошибка в right_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'glass1')
def glass1_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
        bot.send_message(user_id, "Вы собираетесь выпить из 🍷 подозрительно прозрачного бокала. Точно ли там вино?")
    except Exception as e:
        logging.error(f"Ошибка в glass1_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'glass2')
def glass2_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
        bot.send_message(user_id, "Вы собираетесь выпить из 🍸 светло-зелёного бокала. Он точно не отравлен?")
    except Exception as e:
        logging.error(f"Ошибка в glass2_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")

@bot.callback_query_handler(func=lambda call: call.data == 'glass3')
def glass3_handler(call):
    user_id = call.from_user.id
    bot.answer_callback_query(call.id)
    if not game.game_started:
        bot.send_message(user_id, "Игра не активна.")
        return
    if user_id not in game.players.players:
        bot.send_message(user_id, "Вы не участвуете в игре.")
        return
    try:
        game.players.players[user_id].answer = murder_trivia_pb2.Answer.C
        bot.send_message(user_id, "Вы собираетесь выпить из 🥃 небольшая рюмки. Не желаете сменить свой ответ?")
    except Exception as e:
        logging.error(f"Ошибка в glass3_handler для {user_id}: {e}")
        bot.send_message(user_id, "Ошибка при сохранении ответа.")