import murder_trivia_pb2
from pathlib import Path
import logging

BASE_DIR = Path(__file__).resolve().parent
DB_FILE = BASE_DIR / ".." / ".." / "data" / "users.txt"
DB_FILE.parent.mkdir(parents=True, exist_ok=True)
IMAGE_PATH = BASE_DIR / ".." / ".." / ".." / "project"

users = set()
players = murder_trivia_pb2.PlayerList()
game_started = False
registration_started = False

try:
    with open(DB_FILE, "r", encoding="utf-8") as file:
        for line in file:
            cleaned = line.strip()
            if cleaned:
                users.add(int(cleaned))
except FileNotFoundError:
    logging.warning("Файл data/users.txt не найден, набор пользователей пуст")
except Exception as e:
    logging.error(f"Ошибка при чтении users.txt: {e}")


def addUser(chat_id):
    try:
        chat_id = int(chat_id)
    except (ValueError, TypeError):
        return False

    if chat_id not in users:
        try:
            with open(DB_FILE, "a+", encoding="utf-8") as file:
                users.add(chat_id)
                file.write(str(chat_id) + '\n')
            return True
        except Exception as e:
            logging.error(f"Не удалось записать пользователя {chat_id}: {e}")
            return False
    return False


def addPlayer(chat_id):
    try:
        chat_id = int(chat_id)
    except (ValueError, TypeError):
        return

    addUser(chat_id)  # гарантируем, что пользователь есть в users
    if chat_id not in players.players:
        players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
        players.players[chat_id].minigame = murder_trivia_pb2.MinigameType.GAME_UNSPECIFIED
    else:
        # Если уже есть – сбрасываем ответы
        players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
        players.players[chat_id].minigame = murder_trivia_pb2.MinigameType.GAME_UNSPECIFIED


def removePlayer(chat_id):
    try:
        chat_id = int(chat_id)
    except (ValueError, TypeError):
        return

    if chat_id in players.players:
        del players.players[chat_id]