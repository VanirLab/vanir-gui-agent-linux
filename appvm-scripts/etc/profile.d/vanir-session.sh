if [ -O /tmp/vanir-session-env -a -z "$VANIR_ENV_SOURCED" ]; then
    . /tmp/vanir-session-env
fi
