#!/bin/sh

# Register already present XDG_SESSION_COOKIE in session bus

[ -n "$XDG_SESSION_COOKIE" ] && ck-xinit-session-vanir /bin/true
