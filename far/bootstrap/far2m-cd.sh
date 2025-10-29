# source this file in order to run far2m tty mode and make
# shell change directory to last active far2m directory when it exit
# Example: add to you bash.rc:
#  alias f2m="source $(which far2m-cd.sh)"
# and later just run f2m
# Note that there is no much sense to just run this file, so use 'source' Luke

if [ -d /tmp ]; then
	export FAR2M_CWD=/tmp/far2m-cwd-$$
elif [ -d /var/tmp ]; then
	export FAR2M_CWD=/var/tmp/far2m-cwd-$$
else
	export FAR2M_CWD=~/.far2m-cwd-$$
fi

if echo '' > "$FAR2M_CWD"; then
	chmod 0600 "$FAR2M_CWD"
else
	unset FAR2M_CWD
fi

# echo Running far2m --tty "$@"
far2m --tty "$@"

if [ "$FAR2M_CWD" != '' ]; then
	cwd="$(cat "$FAR2M_CWD")"
	unlink "$FAR2M_CWD"
	unset FAR2M_CWD

	if [ "$cwd" != '' ] && [ -d "$cwd" ]; then
#		echo "Changing workdir to $cwd"
		cd "$cwd"
	fi
fi
