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
        bot.send_message(user_id, "Добро пожаловать в смертельную вечеринку! Данный бот позволяет вам боротся за свою душу в игре, посредством Телеграмм бота! Когда сбор в игру начнётся, вам обязательно придёт повестка!")
    else: 
        bot.send_message(user_id, "Думаешь я про тебя забыл? Ага конечно, я уже давно за тобой наблюдаю. А, ты забыл для чего я предназначен?")
        bot.send_message(user_id, "Я бот, который позволяет вам взаимодействовать с игрой, с помощью Телеграмма! Когда игра начнётся, я пришлю вам контракт. Так что ждите...")        

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
                bot.send_message(user_id, f"Вариант ответа {answer_letter} учтен. Вы точно уверены в своем ответе?") # TODO: Поменять сообщение
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

@bot.callback_query_handler(func=lambda call: call.data.startswith('red_wire'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
    bot.send_message(user_id, f"Вы выбрали 🔴 красный провод. Вы точно уверены, что это не \"фаза\"?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('blue_wire'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
    bot.send_message(user_id, f"Вы выбрали 🟦 Синий провод. Вы думаете это \"земля\"?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('left'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
    bot.send_message(user_id, f"Вы решили пойти ⬅️ налево. Вы точно уверены в своем ответе?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('right'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
    bot.send_message(user_id, f"Вы решили пойти ➡️ направо. Там случайно нет мины?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('glass1'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.A
    bot.send_message(user_id, f"Вы собираетесь выпить из 🍷 подозрительно прозрачного бокала. Точно ли там вино?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('glass2'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.B
    bot.send_message(user_id, f"Вы собираетесь выпить из 🍸 светло-зелёного бокала. Он точно не отравлен?")
    bot.answer_callback_query(call.id)

@bot.callback_query_handler(func=lambda call: call.data.startswith('glass3'))
def btn_answer_handler(call):
    user_id = call.from_user.id
    game.players.players[user_id].answer = murder_trivia_pb2.Answer.C
    bot.send_message(user_id, f"Вы собираетесь выпить из 🥃 небольшая рюмки. Не желаете сменить свой ответ?")
    bot.answer_callback_query(call.id)