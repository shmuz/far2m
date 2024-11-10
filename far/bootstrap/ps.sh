#!/bin/sh
############################################################
# This script used by FAR to show processes list
############################################################
############################################################
# For per user customization - create:
#~/.config/far2m/ps.sh
# note that per-user script must do 'exit 0' at the end if
# no need to continue default implementation in main ps.sh
############################################################

if [ -x ~/.config/far2m/ps.sh ]; then
. ~/.config/far2m/ps.sh
fi

if command -v htop >/dev/null 2>&1; then #GNOME
	htop

else
	if [ ! -f ~/.config/far2m/ps.warned ] ; then
		B=$(printf '\e[1;91m')
		N=$(printf '\e[0m')
		echo "It is recommended to install <${B}htop${N}> utility."
		echo "Without <${B}htop${N}> installed far2m will use <${B}top${N}>."
		echo "Press Enter to continue..."
		read k
		touch ~/.config/far2m/ps.warned
	fi
	top
fi
