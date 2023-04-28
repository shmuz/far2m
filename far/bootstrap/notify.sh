#!/bin/sh
############################################################
#This script used by FAR to display desktop notifcations
############################################################
#$1= action
#$2= object
############################################################
#For per user customization - create:
#~/.config/far2m/notify.sh
############################################################

action=$1
object=$2 

if [ -x ~/.config/far2m/notify.sh ]; then
. ~/.config/far2m/notify.sh
fi

if command -v notify-send >/dev/null 2>&1; then #GNOME
	notify-send --app-name=far2m --icon=far2m "$1" "$2"

elif command -v osascript >/dev/null 2>&1; then #OSX
	osascript -e "display notification \"$2\" with title \"$1\""
fi
