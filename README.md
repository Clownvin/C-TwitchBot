# C-TwitchBot
It's a bot for twitch, written in C for Linux (POSIX)

To compile the bot, first replace DEFINEs pertaining to the bot account info (nickname, oath), and the default channel.

Once you've completed that, simply open a terminal, navigate to the source, and use the command: 
**gcc -o TwitchBot twitchbot.c -lpthread**

To run the bot,use ./TwitchBot 
