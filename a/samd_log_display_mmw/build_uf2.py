# Build a UF2 file for dropping on the USB disk
#
# from https://docs.platformio.org/en/latest/scripting/actions.html
#

Import("env")

#print("Current CLI targets", COMMAND_LINE_TARGETS)
#print("Current Build targets", BUILD_TARGETS)

# Custom UF2 from bin
env.AddPostAction(
    "$BUILD_DIR/firmware.bin",
    env.VerboseAction(" ".join([
        "python ../../tools/build/uf2conv.py -c -b 0x4000 -o $BUILD_DIR/${PROGNAME}.uf2 $BUILD_DIR/firmware.bin "
    ]), "Build output at $BUILD_DIR/${PROGNAME}.uf2")
)