This is a web implementation of The Game of Ur that does not use JavaScript. The
goal was to make a complete implementation in as few lines of code as possible,
but then I got sidetracked and added an extra feature or two.

Deployment is easy. You compile it with `./compile.sh`. You should get a
"game\_of\_ur" executable file, which you can run. You specify the port it will
run on as the only argument, e.g.  `./picross 8080`. If you run it as root, it
will drop privileges to group 32766, user 32767, which is the user and group for
"nobody" on OpenBSD. That might not be the case for your system, so you can
change DROP\_TO\_GROUP and DROP\_TO\_USER in cwebserv.h to something with
minimal (or no) permissions, then compile again. You can also run this picross
server as a user that is not root, and it will keep the permissions of that
user.

To play it, you can visit http://localhost:port/ Make sure to replace
"localhost:port" with the address and port of your server (For example, if you
are running it on your local machine on port 8080, it would be "localhost:8080".
People might not start the game properly, make moves slowly or just randomly
stop playing, so it's a good idea to play multiple games at once.

For debugging, recompile it with `./compile.sh -g -DDEBUG`, then repeat the
circumstances you used to get the program to break. Submit a bug report with
git's built in bug reporting system, making sure to include the circumstances of
the bug and the output of the program. If you can't get the whole output, the
output near when the program crashed is fine too.
