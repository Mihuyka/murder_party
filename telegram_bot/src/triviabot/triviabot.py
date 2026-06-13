import telebot
from game import players, game_started, addPlayer, addUser, removePlayer
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
    if not game_started:
        addPlayer(user_id)
        bot.send_message(user_id, 'Вы подписали контракт с @Mihuyka на участие в смертельной вечеринке :)') # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, 'Игра уже началассь. Что поделаешь...') # TODO: Поменять сообщение

@bot.callback_query_handler(func=lambda call: call.data == 'not_participating')
def not_participating_btn(call):
    user_id = call.message.chat.id

    bot.answer_callback_query(call.id)
    if not game_started:
        removePlayer(user_id)
        bot.send_message(user_id, 'Вы разорвали контракт смертельной вечеринки :(') # TODO: Поменять сообщение
    else:
        bot.send_message(user_id, 'Игра уже началассь. Что поделаешь...') # TODO: Поменять сообщение

def start_game(): # Проверить
    game_started = True
    for user_id in players.players:
        playerList = "Игра началась :) В вечеринке участвуют: "
        for user_id in players.players:
            playerList += "@" + bot.get_chat(user_id).username + " "

        bot.send_message(user_id, playerList)

def get_answers():
    keyboard = telebot.types.InlineKeyboardMarkup()
    Abtn = telebot.types.InlineKeyboardButton(text="A", callback_data='answerA')
    Bbtn = telebot.types.InlineKeyboardButton(text="B", callback_data='answerB')
    Cbtn = telebot.types.InlineKeyboardButton(text="C", callback_data='answerC')
    Dbtn = telebot.types.InlineKeyboardButton(text="D", callback_data='answerD')
    keyboard.row(Abtn, Bbtn, Cbtn, Dbtn)

    for user_id in players.players:
        try:
            bot.send_message(user_id, "Выберите один из варинатов ответа: ", reply_markup=keyboard)
        except Exception as e:
            print(f"Ошибка отправки пользователю {user_id}: {e}")

@bot.callback_query_handler(func=lambda call: call.data == 'answerA')
def Abtn(call):
    userID = call.message.chat.id
    if userID in players.keys():
        players[userID] = 'A'
        bot.send_message(userID, "Вариант ответа A учитан")
    else:
        bot.send_message(userID, "Вы не в игре!")

@bot.callback_query_handler(func=lambda call: call.data == 'answerB')
def Bbtn(call):
    userID = call.message.chat.id
    if userID in players.keys():
        players[userID] = 'C'
        bot.send_message(userID, "Вариант ответа B учитан")
    else:
        bot.send_message(userID, "Вы не в игре!")

@bot.callback_query_handler(func=lambda call: call.data == 'answerC')
def Cbtn(call):
    userID = call.message.chat.id
    if userID in players.keys():
        players[userID] = 'C'
        bot.send_message(userID, "Вариант ответа C учитан")
    else:
        bot.send_message(userID, "Вы не в игре!")

@bot.callback_query_handler(func=lambda call: call.data == 'answerD')
def Dbtn(call):
    userID = call.message.chat.id
    if userID in players.keys():
        players[userID] = 'D'
        bot.send_message(userID, "Вариант ответа D учитан")
    else:
        bot.send_message(userID, "Вы не в игре!")