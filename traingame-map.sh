#!/bin/sh

echo ""
echo -n "Which Train Game Map would You use?"
echo ""
ls /usr/lib/traingame/maps
echo ""
read MAP

traingame play $MAP
