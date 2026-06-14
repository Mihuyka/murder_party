import murder_trivia_pb2
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
DB_FILE = BASE_DIR / ".." / ".." / "data" / "users.txt"
DB_FILE.parent.mkdir(parents=True, exist_ok=True)

users = set()
players = murder_trivia_pb2.PlayerList()
game_started = False

try:
    with open(DB_FILE, "r") as file:
        for line in file:
            cleaned = line.strip()
            if cleaned:
                users.add(int(cleaned))
except FileNotFoundError:
    print("Файл data/users.txt не существует, набор пользователей не записываем")


def addUser(chat_id): # True - пользователь добавлен в список, False - пользователь уже сущетсвует
    if chat_id not in users:
        with open(DB_FILE, "a+") as file:
            users.add(int(chat_id))
            file.write(str(chat_id) + '\n')
            return True
    else:
         return False
    
def addPlayer(chat_id):
    addUser(chat_id)
    players.players[chat_id].answer = murder_trivia_pb2.Answer.UNSPECIFIED
    players.players[chat_id].minigame = murder_trivia_pb2.MinigameType.GAME_UNSPECIFIED

def removePlayer(chat_id):
    del players.players[chat_id]