# Murder Trivia

Murder Trivia is a group project made by @kelianor, @Mihuyka and others for the communications techniques college subject. The idea came from a popular Steam game [Trivia Murder Party](https://www.jackboxgames.com/games/trivia-murder-party) created by the [jackbox games](https://www.jackboxgames.com/) studio.


## Usage

Install gRPC and pyTelegramBotAPI python packages:
```
pip install grpcio grpcio-tools pyTelegramBotAPI
```
Supply your Telegram bot token with the token you have received from [@BotFather](tg://resolve?domain=BotFather) into `src/server/config.py` like showcased bellow:
```
BOT_TOKEN='1234567890:ABCDEFGHIJKLMnopqrstuvwxyz'
```
Then, you can run the Telegram bot:
```
make server
```
To play the game you will need to start the conversation with your Telegram bot. After that you can launch the game with:
```
make client
```
The bot should ask if you would like to join, answer yes. That's all, enjoy the trivia!
