from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

env.Replace(
    UPLOADERFLAGS=[
        "-p", "$UPLOAD_PORT",
        "write_flash",
        "-fm", "dio",
        "-fs", "32m",
        "0x00000"
    ],
    UPLOADCMD='/usr/local/bin/esptool.py $UPLOADERFLAGS $SOURCE'
)
