#!/bin/sh

# This file may be also executed by vanir-change-keyboard-layout

VANIR_KEYMAP="`/usr/bin/qubesdb-read /vanir-keyboard`"
VANIR_KEYMAP="`/bin/echo -e $VANIR_KEYMAP`"
VANIR_USER_KEYMAP=`cat $HOME/.config/vanir-keyboard-layout.rc 2> /dev/null`

if [ -n "$VANIR_KEYMAP" ]; then
    echo "$VANIR_KEYMAP" | xkbcomp - $DISPLAY
fi

if [ -n "$VANIR_USER_KEYMAP" ]; then
    VANIR_USER_KEYMAP_LAYOUT=`echo $VANIR_USER_KEYMAP+ | cut -f 1 -d +`
    VANIR_USER_KEYMAP_VARIANT=`echo $VANIR_USER_KEYMAP+ | cut -f 2 -d +`
    if [ -n "$VANIR_USER_KEYMAP_VARIANT" ]; then
        VANIR_USER_KEYMAP_VARIANT="-variant $VANIR_USER_KEYMAP_VARIANT"
    fi
    setxkbmap $VANIR_USER_KEYMAP_LAYOUT $VANIR_USER_KEYMAP_VARIANT
fi
