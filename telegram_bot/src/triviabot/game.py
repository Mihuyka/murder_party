import murder_trivia_pb2

DB_FILE = "../data/users.txt"

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
    print("Файл data/users.txt не сущетсвует, ничего не записываем")


def addUser(userID): # True - пользователь добавлен в список, False - пользователь уже сущетсвует
    if userID not in users:
        with open(DB_FILE, "a+") as file:
            users.add(int(userID))
            file.write(str(userID) + '\n')
            return True
    else:
         return False
    
def addPlayer(userID):
    addUser(userID)
    players.players[userID].answer = murder_trivia_pb2.Answer.UNSPECIFIED

def removePlayer(userID):
    del players.players[userID]