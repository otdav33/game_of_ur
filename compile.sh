#/bin/sh

git clone https://github.com/otdav33/cwebserv.git
cp cwebserv/cwebserv.{c,h} .
gcc $* -o game_of_ur *.c *.h
